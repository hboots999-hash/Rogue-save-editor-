#include "live_draft.hpp"
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>
namespace fs=std::filesystem;
struct Snapshot{
    bool ok=false;
    std::unique_ptr<uint8_t[]> save{};
    std::string stage="failed";
};
int main(int argc,char**argv){
    assert(argc==2);
    std::ifstream f(argv[1],std::ios::binary);
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)),{});
    assert(raw.size()==h168::Size);
    constexpr size_t bankSize=14*h168::Sector;
    std::memcpy(raw.data()+bankSize,raw.data(),bankSize);
    for(unsigned j=0;j<14;j++)
        h168::w32(raw.data(),bankSize+j*h168::Sector+0xffc,1);
    h168::Info info{};
    assert(h168::inspect(raw.data(),raw.size(),info));
    auto savedRaw=raw;
    auto live=[&]{
        Snapshot s;
        s.ok=true;
        s.save=std::unique_ptr<uint8_t[]>(new uint8_t[h168::Size]);
        std::memcpy(s.save.get(),raw.data(),h168::Size);
        return s;
    };
    const auto root=fs::temp_directory_path()/"pksm_phase13_liveedit_test";
    fs::remove_all(root);
    auto money=rogue_restore::stageFromLive(live,root.c_str(),
                        rogue_restore::Operation::MoneyPlusOne);
    assert(money.ok && money.editProof.oldValue==40000 &&
           money.editProof.newValue==40001 &&
           money.backupName=="live_001.sav" &&
           money.draftName=="liveedit_001.sav");
    auto get=[&](const fs::path& path){
        std::ifstream f(path,std::ios::binary);
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});
    };
    assert(get(root/"backups"/money.backupName)==savedRaw);
    const auto result=get(root/"drafts"/money.draftName);
    assert(result.size()==h168::Size);
    h168::Info edited{};
    assert(h168::inspect(result.data(),result.size(),edited));
    assert(edited.money==40001 && edited.partyCount==info.partyCount);
    assert(std::memcmp(result.data(),savedRaw.data(),bankSize)==0);
    assert(raw==savedRaw);
    auto item=rogue_restore::stageFromLive(live,root.c_str(),
                    rogue_restore::Operation::ItemQuantityPlusOne,0);
    assert(item.ok && item.backupName=="live_002.sav" &&
           item.draftName=="liveedit_002.sav");
    const auto itemBytes=get(root/"drafts"/item.draftName);
    assert(h168::inspect(itemBytes.data(),itemBytes.size(),edited));
    unsigned id=0,qty=0;
    assert(h168::item(itemBytes.data(),edited,0,id,qty) && id==113 && qty==11);
    assert(raw==savedRaw);
    // Fresh live source changes after the independently verified backup:
    // reject before drafting, even though the backup itself was successful.
    unsigned calls=0;
    auto stale=[&]{
        auto s=live();
        ++calls;
        if(calls>=4)s.save[0]^=1;
        return s;
    };
    auto blocked=rogue_restore::stageFromLive(stale,root.c_str(),
        rogue_restore::Operation::MoneyPlusOne);
    assert(!blocked.ok);
    assert(!fs::exists(root/"drafts/liveedit_003.sav"));
    assert(raw==savedRaw);
    fs::remove_all(root);
    std::cout<<"PASS Phase 13: live backup and real-save-derived offline money/item draft; stale source rejected\n";
}
