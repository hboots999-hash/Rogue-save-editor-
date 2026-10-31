#include "rogue_pokemon_edit.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
static uint16_t r16(const uint8_t*p){return uint16_t(p[0]) | uint16_t(p[1])<<8;}
static uint32_t r32(const uint8_t*p){return uint32_t(r16(p)) | uint32_t(r16(p+2))<<16;}
static void w16(uint8_t*p,uint16_t x){p[0]=uint8_t(x);p[1]=uint8_t(x>>8);}
static void w32(uint8_t*p,uint32_t x){w16(p,uint16_t(x));w16(p+2,uint16_t(x>>16));}
static constexpr uint8_t offsets[24][4]={
 {0,1,2,3},{0,1,3,2},{0,2,1,3},{0,3,1,2},{0,2,3,1},{0,3,2,1},
 {1,0,2,3},{1,0,3,2},{2,0,1,3},{3,0,1,2},{2,0,3,1},{3,0,2,1},
 {1,2,0,3},{1,3,0,2},{2,1,0,3},{3,1,0,2},{2,3,0,1},{3,2,0,1},
 {1,2,3,0},{1,3,2,0},{2,1,3,0},{3,1,2,0},{2,3,1,0},{3,2,1,0}
};
int main(int argc,char**argv){
 assert(argc==2);std::ifstream f(argv[1],std::ios::binary);
 const std::vector<uint8_t> base((std::istreambuf_iterator<char>(f)),{});
 assert(base.size()==rogue::ObservedSaveBytes);
 for(unsigned k=0;k<24;k++){
  auto src=base;
  auto* s1=src.data()+0x1000;
  w32(s1+0x234,1);
  auto* mon=s1+0x238;
  uint32_t pid=2400+k,ot=0x44556677;
  w32(mon,pid);w32(mon+4,ot);mon[0x13]=2;
  std::array<uint8_t,48> plain{};
  auto* growth=plain.data()+12*offsets[k][0];
  w16(growth,25);
  auto* ev=plain.data()+12*offsets[k][2];
  for(int i=0;i<6;i++)ev[i]=uint8_t(i+1);
  auto* misc=plain.data()+12*offsets[k][3];
  w32(misc+4,1u | (2u<<5) | (3u<<10));
  unsigned sum=0;for(int i=0;i<48;i+=2)sum+=r16(plain.data()+i);
  w16(mon+28,uint16_t(sum));
  for(int i=0;i<48;i+=4)w32(mon+32+i,r32(plain.data()+i)^(pid^ot));
  w16(s1+0xff6,rogue::checksum(s1,0xff4));
  auto before=rogue::inspectObserved(src.data(),src.size());
  assert(before.status==rogue::ObservedStatus::Candidate);
  assert(before.party.size()==1 && before.party[0].species==25);
  rogue_pokemon_stats_request request{};
  request.party_index=0;request.expected_species=25;request.expected_personality=pid;
  request.change_ivs=request.change_evs=1;
  for(auto& x:request.ivs)x=31;
  request.evs[0]=4;request.evs[1]=252;request.evs[3]=252;
  std::vector<uint8_t> out(src.size(),0xad);size_t changed=0;
  assert(rogue_make_pokemon_stats_draft(src.data(),src.size(),&request,out.data(),out.size(),&changed)==ROGUE_MON_EDIT_OK);
  assert(changed>0 && changed<60);
  const auto after=rogue::inspectObserved(out.data(),out.size());
  assert(after.status==rogue::ObservedStatus::Candidate);
  assert(after.party[0].species==25 && after.party[0].iv[0]==31 && after.party[0].iv[5]==31);
  assert(after.party[0].ev[0]==4 && after.party[0].ev[1]==252 && after.party[0].ev[3]==252);
  assert(before.money==after.money && before.items.size()==after.items.size());
  request.expected_personality++;
  assert(rogue_make_pokemon_stats_draft(src.data(),src.size(),&request,out.data(),out.size(),&changed)==ROGUE_MON_EDIT_STALE_PARTY);
  request.expected_personality--;
  request.evs[0]=255;
  assert(rogue_make_pokemon_stats_draft(src.data(),src.size(),&request,out.data(),out.size(),&changed)==ROGUE_MON_EDIT_BAD_STATS);
 }
 std::cout<<"PASS: 24 Pokemon block permutations, checksums, limits and stale guards; synthetic only\n";
}
