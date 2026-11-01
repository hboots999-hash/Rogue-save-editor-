// Phase 12 synthetic-only live backup regression; never accesses the installed title.
#include "live_backup.hpp"
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
    std::string stage="";
};
int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream in(argv[1],std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),{});
    assert(data.size()==h168::Size);
    constexpr size_t bankSize=14u*h168::Sector;
    std::memcpy(data.data()+bankSize,data.data(),bankSize);
    for(unsigned j=0;j<14;j++)
        h168::w32(data.data(),bankSize+j*h168::Sector+0xffc,1);
    h168::Info info{};
    assert(h168::inspect(data.data(),data.size(),info));
    const auto original=data;
    const fs::path root=fs::temp_directory_path()/"pksm_phase12_readonly_backup_test";
    fs::remove_all(root);
    auto reader=[&]{
        Snapshot s;
        s.ok=true;s.save=std::make_unique<uint8_t[]>(h168::Size);
        std::memcpy(s.save.get(),data.data(),h168::Size);
        return s;
    };
    auto a=rogue_live_backup::backupVerified(reader,root.c_str());
    assert(a.ok && a.name=="live_001.sav");
    auto b=rogue_live_backup::backupVerified(reader,root.c_str());
    assert(b.ok && b.name=="live_002.sav");
    for(const auto& name:{"live_001.sav","live_002.sav"}){
        const auto p=root/"backups"/name;
        std::ifstream f(p,std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
        assert(bytes==original);
    }
    assert(data==original);
    // A changed snapshot between live read #1 and #2 must refuse output.
    int calls=0;
    auto stale=[&]{
        auto s=reader();
        ++calls;
        if(calls==2)s.save[bankSize+4096+0x4a8]^=1;
        return s;
    };
    const auto fail=rogue_live_backup::backupVerified(stale,root.c_str());
    assert(!fail.ok);
    assert(!fs::exists(root/"backups/live_003.sav"));
    // A change after backup creates a warning; preserves the verified backup.
    calls=0;
    auto postStale=[&]{
        auto s=reader();
        ++calls;
        if(calls==3)s.save[bankSize+4096+0x4a8]^=1;
        return s;
    };
    auto warn=rogue_live_backup::backupVerified(postStale,root.c_str());
    assert(!warn.ok && warn.name=="live_003.sav");
    assert(fs::exists(root/"backups/live_003.sav"));
    assert(data==original);
    auto broken=[&]{Snapshot s;s.stage="No live service";return s;};
    auto failLive=rogue_live_backup::backupVerified(broken,root.c_str());
    assert(!failLive.ok);
    assert(!fs::exists(root/"backups/live_004.sav"));
    fs::remove_all(root);
    std::cout<<"PASS Phase 12: live snapshots, unique verified SD backups, no edits, stale refusals\n";
}
