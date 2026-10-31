// Phase 10 native-safe offline copied-save regression; GPL-3.0-or-later.
#include "offline.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
namespace fs=std::filesystem;
using Bytes=std::vector<uint8_t>;
Bytes read(const fs::path& p){
    std::ifstream in(p,std::ios::binary);
    assert(in);
    return Bytes(std::istreambuf_iterator<char>(in),{});
}
void put(const fs::path& p,const Bytes& b){
    std::ofstream out(p,std::ios::binary|std::ios::trunc);
    assert(out);out.write(reinterpret_cast<const char*>(b.data()),b.size());
    assert(out.good());
}
int main(int argc,char** argv){
    assert(argc==2);
    Bytes original=read(argv[1]);
    assert(original.size()==h168::Size);
    // Toy fixture bank 0 is valid; make second toy bank by duplicating its
    // 14 sectors and changing counter to 1 (not a game-playable save).
    constexpr size_t bankSize=14*h168::Sector;
    std::memcpy(original.data()+bankSize,original.data(),bankSize);
    for(unsigned j=0;j<14;j++){
        h168::w32(original.data(),bankSize+j*h168::Sector+0xffc,1);
    }
    h168::Info info{};
    assert(h168::inspect(original.data(),original.size(),info));
    assert(info.active==1 && info.money==40000 && info.partyCount==1 &&
           info.items==3 && h168::species(original.data(),info,0)==25);
    unsigned id=0,qty=0;
    assert(h168::item(original.data(),info,0,id,qty) && id==113 && qty==10);
    fs::path tmp=fs::temp_directory_path()/"pksm_h168_preview_test";
    fs::remove_all(tmp);
    fs::create_directories(tmp/"imports");
    const auto source=tmp/"imports/00000001.sav";
    put(source,original);
    auto money=h168::exportIncrement(original.data(),source.c_str(),tmp.c_str(),true);
    assert(money.ok&&money.changed>0&&money.changed<=6);
    assert(read(source)==original);
    assert(read(tmp/"backups/import_001.sav")==original);
    auto draft=read(tmp/"drafts/h168_001.sav");
    h168::Info after{};
    assert(h168::inspect(draft.data(),draft.size(),after) && after.money==40001);
    assert(std::memcmp(draft.data(),original.data(),bankSize)==0);
    assert(h168::item(draft.data(),after,0,id,qty)&&qty==10);
    auto itemEdit=h168::exportIncrement(original.data(),source.c_str(),tmp.c_str(),false,0);
    assert(itemEdit.ok);
    assert(read(tmp/"backups/import_002.sav")==original);
    auto itemDraft=read(tmp/"drafts/h168_002.sav");
    assert(h168::inspect(itemDraft.data(),itemDraft.size(),after) && after.money==40000);
    assert(h168::item(itemDraft.data(),after,0,id,qty)&&id==113&&qty==11);
    assert(std::memcmp(itemDraft.data(),original.data(),bankSize)==0);
    // Never replace existing filenames, never write when imported bytes are stale.
    auto changed=original;changed[bankSize+4096+0x4a8]^=1;
    assert(!h168::exportIncrement(changed.data(),source.c_str(),tmp.c_str(),true).ok);
    assert(!fs::exists(tmp/"drafts/h168_003.sav"));
    changed=original;changed[bankSize+4096+0x4a8]^=1;
    assert(!h168::inspect(changed.data(),changed.size(),after));
    assert(!h168::exportIncrement(changed.data(),source.c_str(),tmp.c_str(),true).ok);
    assert(!h168::exportIncrement(original.data(),source.c_str(),tmp.c_str(),false,3).ok);
    assert(read(source)==original);
    fs::remove_all(tmp);
    std::cout<<"PASS H168: two banks, Pokémon validation, +1 money/item, byte-exact backup, stale rejection, untouched import\n";
}
