// Phase 13 fail-closed regression: only synthetic, nonplayable saves.
// No live game, Luma service, native restore, or privileged write.
#include "live_draft.hpp"
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
namespace fs=std::filesystem;
struct Snapshot {
    bool ok=false;
    std::unique_ptr<uint8_t[]> save{};
    std::string stage="injected read failure";
};
int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream input(argv[1],std::ios::binary);
    const std::vector<uint8_t> one((std::istreambuf_iterator<char>(input)),{});
    assert(one.size()==h168::Size);
    std::vector<uint8_t> raw=one;
    constexpr size_t bankSize=14*h168::Sector;
    std::memcpy(raw.data()+bankSize,raw.data(),bankSize);
    for(unsigned j=0;j<14;j++)
        h168::w32(raw.data(),bankSize+j*h168::Sector+0xffc,1);
    h168::Info info{};
    assert(h168::inspect(raw.data(),raw.size(),info));
    auto live=[&] {
        Snapshot s;
        s.ok=true;
        s.stage="ok";
        s.save=std::unique_ptr<uint8_t[]>(new uint8_t[h168::Size]);
        std::memcpy(s.save.get(),raw.data(),h168::Size);
        return s;
    };
    const auto top=fs::temp_directory_path()/"pksm_phase13_failclosed";
    fs::remove_all(top);
    fs::create_directories(top);
    // Stage sequence: 1/2 initial matching reads; 3 backup postcheck;
    // 4 pre-draft freshness; 5 final live reread after draft was written.
    // Every failure must be reported; no falsely successful receipt.
    for(unsigned failAt=1;failAt<=5;failAt++){
        const auto root=top/("fail_"+std::to_string(failAt));
        unsigned calls=0;
        auto failed=[&] {
            auto s=live();
            if(++calls==failAt){s.ok=false;s.save.reset();s.stage="fault";}
            return s;
        };
        const auto receipt=rogue_restore::stageFromLive(
            failed,root.c_str(),rogue_restore::Operation::MoneyPlusOne);
        assert(!receipt.ok);
        assert(calls==failAt);
        const bool hasBackup=fs::exists(root/"backups/live_001.sav");
        const bool hasDraft=fs::exists(root/"drafts/liveedit_001.sav");
        assert(hasBackup==(failAt>=3));
        assert(hasDraft==(failAt==5));
        if(hasBackup){
            std::ifstream b(root/"backups/live_001.sav",std::ios::binary);
            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(b)),{});
            assert(bytes==raw);
        }
        if(failAt==5)
            assert(receipt.message.find("DO NOT RESTORE")!=std::string::npos);
    }
    // Valid but different live save on read four: backup retained and
    // draft refused even if each individual snapshot passes checksums.
    rogue_restore::Validation p{};
    auto changed=rogue_restore::prepare(raw.data(),
                            rogue_restore::Operation::MoneyPlusOne,0,p);
    assert(changed && p.ok);
    {
        const auto root=top/"valid_stale";
        unsigned calls=0;
        auto stale=[&] {
            auto s=live();
            if(++calls==4)
                std::memcpy(s.save.get(),changed.get(),h168::Size);
            return s;
        };
        const auto receipt=rogue_restore::stageFromLive(
            stale,root.c_str(),rogue_restore::Operation::MoneyPlusOne);
        assert(!receipt.ok && calls==4);
        assert(fs::exists(root/"backups/live_001.sav"));
        assert(!fs::exists(root/"drafts/liveedit_001.sav"));
    }
    // An existing numbered backup must NEVER be overwritten, including
    // when a prior failed attempt left an incomplete file at that name.
    {
        const auto root=top/"preserve";
        fs::create_directories(root/"backups");
        const auto old=root/"backups/live_001.sav";
        {
            std::ofstream out(old,std::ios::binary);
            out<<"do-not-overwrite";
        }
        const auto receipt=rogue_restore::stageFromLive(
            live,root.c_str(),rogue_restore::Operation::MoneyPlusOne);
        assert(receipt.ok);
        assert(receipt.backupName=="live_002.sav");
        assert(receipt.draftName=="liveedit_001.sav");
        std::ifstream oldFile(old,std::ios::binary);
        const std::string previous((std::istreambuf_iterator<char>(oldFile)),{});
        assert(previous=="do-not-overwrite");
        assert(fs::file_size(root/"backups/live_002.sav")==h168::Size);
        assert(fs::file_size(root/"drafts/liveedit_001.sav")==h168::Size);
    }
    assert(one.size()==h168::Size);
    fs::remove_all(top);
    std::cout<<"PASS Phase 13 fail-closed: five read fault points, valid stale data, no overwrite\n";
}
