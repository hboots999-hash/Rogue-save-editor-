// Offline synthetic-only exporter integration test. Never uses a real game save.
#include "offline_sandbox.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
namespace fs=std::filesystem;
using Bytes=std::vector<uint8_t>;
static Bytes read(const fs::path& p) {
    std::ifstream f(p,std::ios::binary); assert(f);
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
static void write(const fs::path& p,const Bytes& b) {
    std::ofstream f(p,std::ios::binary|std::ios::trunc);assert(f);
    f.write(reinterpret_cast<const char*>(b.data()),b.size());assert(f.good());
}
static fs::path setup(const fs::path& parent,const char* name,const Bytes& original){
    const fs::path root=parent/name;
    fs::create_directories(root/"imports");
    write(root/"imports/00000001.sav",original);
    return root;
}
static rogue_sandbox::Result run(const fs::path& root,const Bytes& initial,
                                 rogue_sandbox::Edit action,unsigned slot=0){
    return rogue_sandbox::exportOne(initial.data(),
             (root/"imports/00000001.sav").c_str(),root.c_str(),action,slot);
}
int main(int argc,char** argv){
    assert(argc==2);
    const Bytes initial=read(argv[1]);
    assert(initial.size()==ROGUE_RAW_SAVE_SIZE);
    assert(rogue_sandbox::exactSynthetic(initial.data(),initial.size()));
    auto altered=initial;altered[altered.size()-1]^=0x5a;
    assert(!rogue_sandbox::exactSynthetic(altered.data(),altered.size()));
    assert(!rogue_sandbox::exactSynthetic(initial.data(),initial.size()-1));
    const fs::path temp=fs::temp_directory_path()/
        ("pksm_phase9_2_"+std::to_string(::getpid()));
    assert(!fs::exists(temp));fs::create_directories(temp);

    const auto reject=setup(temp,"real_or_changed",initial);
    auto r=run(reject,altered,rogue_sandbox::Edit::Money);
    assert(!r.ok && !fs::exists(reject/"backups") && !fs::exists(reject/"drafts"));
    write(reject/"imports/00000001.sav",altered);
    r=run(reject,initial,rogue_sandbox::Edit::Money);
    assert(!r.ok && !fs::exists(reject/"backups") && !fs::exists(reject/"drafts"));

    const auto blocked=setup(temp,"backup_failure",initial);
    const Bytes sentinel={'N','O','T','_','A','_','D','I','R'};
    write(blocked/"backups",sentinel);
    r=run(blocked,initial,rogue_sandbox::Edit::Money);
    assert(!r.ok && read(blocked/"backups")==sentinel && !fs::exists(blocked/"drafts"));
    assert(read(blocked/"imports/00000001.sav")==initial);

    const auto good=setup(temp,"positive",initial);
    r=run(good,initial,rogue_sandbox::Edit::Money);
    assert(r.ok && r.changed>0);
    assert(read(good/"backups/import_001.sav")==initial);
    assert(read(good/"imports/00000001.sav")==initial);
    Bytes draft=read(good/"drafts/rogue_001.sav");
    assert(draft!=initial);
    rogue_preview p{};assert(rogue_preview_copy(draft.data(),draft.size(),&p));
    assert(p.money==999999);

    r=run(good,initial,rogue_sandbox::Edit::ExistingItem,0);
    assert(r.ok && r.changed>0);
    assert(read(good/"backups/import_002.sav")==initial);
    assert(read(good/"imports/00000001.sav")==initial);
    draft=read(good/"drafts/rogue_002.sav");
    rogue_item_candidate item{};
    std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
       rogue_snapshot_open(draft.data(),draft.size()),rogue_snapshot_close);
    assert(snap && rogue_snapshot_item(snap.get(),0,&item));
    assert(item.quantity==999 && item.item_id==113);
    r=run(good,initial,rogue_sandbox::Edit::ExistingItem,999);
    assert(!r.ok && !fs::exists(good/"drafts/rogue_003.sav"));

    // Pre-existing names cannot be replaced. Both exports skip occupied slots.
    const auto occupied=setup(temp,"occupied",initial);
    fs::create_directories(occupied/"backups");
    fs::create_directories(occupied/"drafts");
    write(occupied/"backups/import_001.sav",sentinel);
    write(occupied/"drafts/rogue_001.sav",sentinel);
    r=run(occupied,initial,rogue_sandbox::Edit::Money);
    assert(r.ok && read(occupied/"backups/import_001.sav")==sentinel);
    assert(read(occupied/"drafts/rogue_001.sav")==sentinel);
    assert(read(occupied/"backups/import_002.sav")==initial);
    assert(read(occupied/"drafts/rogue_002.sav")!=initial);

    // If draft directory cannot be created, no draft is written; backup remains.
    const auto draftBlocked=setup(temp,"draft_failure",initial);
    write(draftBlocked/"drafts",sentinel);
    r=run(draftBlocked,initial,rogue_sandbox::Edit::Money);
    assert(!r.ok);
    assert(read(draftBlocked/"drafts")==sentinel);
    assert(read(draftBlocked/"backups/import_001.sav")==initial);
    assert(read(draftBlocked/"imports/00000001.sav")==initial);

    fs::remove_all(temp);
    std::cout<<"PASS synthetic-only offline money/item export: byte-exact gate, stale input, backup failure, verified copies, exclusive names, draft I/O refusal\n";
}
