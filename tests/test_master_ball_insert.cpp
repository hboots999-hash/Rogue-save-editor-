#include "rogue_ball_insert.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

static void w16(uint8_t* p,uint16_t x){p[0]=uint8_t(x);p[1]=uint8_t(x>>8);}
static void updateSector1(std::vector<uint8_t>& v){
    w16(v.data()+0x1000+0xff6,rogue::checksum(v.data()+0x1000,0xff4));
}
static std::vector<uint8_t> load(const char* path){
    std::ifstream f(path,std::ios::binary);
    return {(std::istreambuf_iterator<char>(f)),{}};
}
int main(int argc,char**argv){
    assert(argc==2);
    auto src=load(argv[1]);
    assert(src.size()==131072);
    src[0x1000+0x4ac]=0; // pinned default type sort
    src[0x1000+0x4ad]=37; // maximum, 450 slots
    updateSector1(src);
    const auto before=rogue::inspectObserved(src.data(),src.size());
    assert(before.status==rogue::ObservedStatus::Candidate);
    assert(before.items.size()==2 && before.items[0].id==1 && before.items[1].id==2);
    std::vector<uint8_t> edited(src.size(),0xa5);
    std::size_t changed=0;
    assert(rogue_make_master_ball_draft(src.data(),src.size(),edited.data(),edited.size(),&changed)
           ==ROGUE_BALL_INSERT_OK);
    assert(changed>0 && changed<12);
    const auto after=rogue::inspectObserved(edited.data(),edited.size());
    assert(after.status==rogue::ObservedStatus::Candidate && after.items.size()==3);
    assert(after.items[0].id==1 && after.items[0].quantity==50);
    assert(after.items[1].id==2 && after.items[1].quantity==75);
    assert(after.items[2].id==4 && after.items[2].quantity==999);
    assert(before.money==after.money && before.party.size()==after.party.size());
    const auto original=src;
    assert(rogue_make_master_ball_draft(edited.data(),edited.size(),src.data(),src.size(),&changed)
           ==ROGUE_BALL_INSERT_ALREADY_OWNED);
    assert(src==original && changed==0);
    auto refuse=[&](std::vector<uint8_t> v,rogue_ball_insert_status expected){
        std::vector<uint8_t> out(v.size(),0xb3);
        std::size_t n=98;
        auto code=rogue_make_master_ball_draft(v.data(),v.size(),out.data(),out.size(),&n);
        assert(code==expected);
        assert(n==0 && std::all_of(out.begin(),out.end(),[](uint8_t x){return x==0xb3;}));
    };
    // Refuse non-default sort, holes, unavailable capacity, and invalid checksum.
    auto mutated=original;
    mutated[0x1000+0x4ac]=1;updateSector1(mutated);
    refuse(mutated,ROGUE_BALL_INSERT_BAG_UNSAFE);
    mutated=original;
    w16(mutated.data()+0x1000+0x578,0);updateSector1(mutated);
    refuse(mutated,ROGUE_BALL_INSERT_BAG_UNSAFE);
    mutated=original;
    mutated[0x1000+0x4ad]=0;
    for(unsigned i=2;i<30;i++)w16(mutated.data()+0x1000+0x578+4*i,uint16_t(113+i));
    updateSector1(mutated);
    refuse(mutated,ROGUE_BALL_INSERT_NO_CAPACITY);
    mutated=original;
    mutated[0x1000+0x578]^=0x4f;
    refuse(mutated,ROGUE_BALL_INSERT_BAD_SOURCE);
    mutated=original;
    mutated[10*0x1000+0xe00]=8;
    w16(mutated.data()+10*0x1000+0xff6,rogue::checksum(mutated.data()+10*0x1000,0xff4));
    refuse(mutated,ROGUE_BALL_INSERT_UNSUPPORTED_VERSION);
    assert(rogue_make_master_ball_draft(original.data(),original.size(),
        const_cast<uint8_t*>(original.data()),original.size(),&changed)
        ==ROGUE_BALL_INSERT_BAD_ARGUMENT);
    std::puts("PASS: source-derived Master Ball insertion, exact copies, limits, marker, checksum and fail-closed refusal");
}
