#include "rogue_hub_probe.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
static void w16(uint8_t* p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
static std::vector<uint8_t> read(const char* path){
 std::ifstream stream(path,std::ios::binary);
 return {(std::istreambuf_iterator<char>(stream)),{}};
}
static void fixSector(std::vector<uint8_t>& v,unsigned id){
 w16(v.data()+id*4096+0xff6,rogue::checksum(v.data()+id*4096,rogue::kCandidateSectorSizes[id]));
}
int main(int argc,char**argv){
 assert(argc==2);
 const auto original=read(argv[1]);
 assert(original.size()==131072);
 rogue_hub_report hub{};
 assert(rogue_probe_hub(original.data(),original.size(),&hub)==ROGUE_HUB_OK);
 assert(hub.selected_copy==0 && hub.coordinate_stride==4);
 assert(hub.built_count==2 && hub.upgrades_count==2);
 assert(hub.area_built[0]==1 && hub.area_built[2]==1 && hub.area_built[10]==0);
 assert(hub.upgrade_owned[0]==1 && hub.upgrade_owned[2]==1 && hub.upgrade_owned[47]==0);
 const auto before=original;
 auto v=original;
 // Corrupt active sector checksum, even while hub header looks plausible.
 v[10*4096+0xe00]^=0x80;
 assert(rogue_probe_hub(v.data(),v.size(),&hub)==ROGUE_HUB_BAD_SOURCE);
 v=original;
 // Preserve checksum but change quest array count; strict layout must refuse.
 v[10*4096+0xe00+8]=0xdd;fixSector(v,10);
 assert(rogue_probe_hub(v.data(),v.size(),&hub)==ROGUE_HUB_UNRECOGNIZED_LAYOUT);
 v=original;
 // Padding discrepancy is only accepted if zero, not guessed.
 // This byte lives in logical sector 11, after 0xe00 start + 1822 + 2.
 const std::size_t logical=0xe00+1822+2;
 const std::size_t off=logical<0xff4?10*4096+logical:11*4096+(logical-0xff4);
 v[off]=0x7c;
 fixSector(v,logical<0xff4?10:11);
 assert(rogue_probe_hub(v.data(),v.size(),&hub)==ROGUE_HUB_UNRECOGNIZED_LAYOUT);
 // Invalid size or pointer fails before touching output.
 hub.built_count=99;
 assert(rogue_probe_hub(nullptr,131072,&hub)==ROGUE_HUB_BAD_ARGUMENT);
 assert(hub.built_count==99);
 assert(before==original);
 std::puts("PASS: synthetic hub bitfields, strict layout, untouched output on refusal");
}
