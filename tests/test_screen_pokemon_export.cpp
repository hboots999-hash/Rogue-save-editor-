#include "RogueInspectScreen.hpp"
#include "3ds.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <unistd.h>
namespace fs=std::filesystem;
static uint16_t r16(const uint8_t*p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
static uint32_t r32(const uint8_t*p){return uint32_t(r16(p))|uint32_t(r16(p+2))<<16;}
static void w16(uint8_t*p,uint16_t x){p[0]=uint8_t(x);p[1]=uint8_t(x>>8);}
static void w32(uint8_t*p,uint32_t x){w16(p,uint16_t(x));w16(p+2,uint16_t(x>>16));}
static std::vector<uint8_t> read(const fs::path& p){
 std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};
}
static void press(RogueInspectScreen& s,u32 k){simulatedKeysDown=k;s.update(nullptr);}
static std::vector<fs::path> list(const fs::path& p){
 std::vector<fs::path> files;for(const auto& f:fs::directory_iterator(p))files.push_back(f.path());
 return files;
}
int main(int argc,char**argv){
 assert(argc==2);
 auto src=read(argv[1]);assert(src.size()==131072);
 // Synthetic Pokemon record in sector 1, personality 24 = first permutation.
 auto* s1=src.data()+0x1000;
 w32(s1+0x234,1);
 auto* p=s1+0x238;
 const uint32_t pid=24,ot=0x11225566,key=pid^ot;
 w32(p,pid);w32(p+4,ot);p[0x13]=2;
 std::array<uint8_t,48> plain{};
 w16(plain.data(),25);
 plain[24]=2;plain[25]=3;
 w32(plain.data()+36+4,1u | (2u<<5));
 uint32_t sum=0;for(int i=0;i<48;i+=2)sum+=r16(plain.data()+i);
 w16(p+28,uint16_t(sum));
 for(int i=0;i<48;i+=4)w32(p+32+i,r32(plain.data()+i)^key);
 w16(s1+0xff6,rogue::checksum(s1,0xff4));
 const auto initial=rogue::inspectObserved(src.data(),src.size());
 assert(initial.status==rogue::ObservedStatus::Candidate && initial.party.size()==1);
 char tmp[]="/tmp/pksm-rogue-mon-XXXXXX";
 const auto root=mkdtemp(tmp);assert(root);
 const auto cwd=fs::current_path();fs::current_path(root);
 const fs::path base="sdmc:/3ds/PKSM-Rogue";
 fs::create_directories(base/"imports");
 {std::ofstream out(base/"imports/00000001.sav",std::ios::binary);
  out.write(reinterpret_cast<const char*>(src.data()),src.size());}
 {
  RogueInspectScreen screen;
  press(screen,KEY_R); // Party tab
  press(screen,KEY_X); // 31 IVs
  press(screen,KEY_START);press(screen,KEY_START);
  press(screen,KEY_Y); // 252/252/4 EVs on original imported Pokemon
  press(screen,KEY_START);press(screen,KEY_START);
  const auto originals=list(base/"originals"), drafts=list(base/"drafts");
  assert(originals.size()==2 && drafts.size()==2);
  for(const auto& o:originals)assert(read(o)==src);
  assert(read(base/"imports/00000001.sav")==src);
  bool foundIV=false,foundEV=false;
  for(const auto& d:drafts){
   auto bytes=read(d);auto view=rogue::inspectObserved(bytes.data(),bytes.size());
   assert(view.status==rogue::ObservedStatus::Candidate);
   assert(view.party.size()==1 && view.party[0].species==25);
   if(view.party[0].iv[0]==31 && view.party[0].iv[5]==31)foundIV=true;
   if(view.party[0].ev[0]==4 && view.party[0].ev[1]==252 && view.party[0].ev[3]==252)foundEV=true;
   assert(view.money==initial.money);
  }
  assert(foundIV && foundEV);
 }
 fs::current_path(cwd);fs::remove_all(root);
 std::cout<<"PASS: UI X/Y staged independent Pokemon IV/EV offline drafts with originals intact\n";
}
