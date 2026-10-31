#include "RogueInspectScreen.hpp"
#include "3ds.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <unistd.h>
namespace fs=std::filesystem;
static std::vector<uint8_t> read(const fs::path& file){
 std::ifstream input(file,std::ios::binary);
 return {(std::istreambuf_iterator<char>(input)),{}};
}
static void w16(uint8_t* p,uint16_t value){p[0]=uint8_t(value);p[1]=uint8_t(value>>8);}
static void press(RogueInspectScreen& screen,u32 key){
 simulatedKeysDown=key;screen.update(nullptr);
}
int main(int argc,char**argv) {
 assert(argc==2);
 auto initial=read(argv[1]);
 assert(initial.size()==131072);
 // Synthetic candidate save's sector1 is physical sector1; add full capacity.
 initial[0x1000+0x4ac]=0;
 initial[0x1000+0x4ad]=37;
 w16(initial.data()+0x1000+0xff6,rogue::checksum(initial.data()+0x1000,0xff4));
 const auto before=rogue::inspectObserved(initial.data(),initial.size());
 assert(before.status==rogue::ObservedStatus::Candidate);
 assert(before.items.size()==2);
 char temp[]="/tmp/pksm-rogue-ball-XXXXXX";
 auto root=mkdtemp(temp);assert(root);
 const auto previous=fs::current_path();fs::current_path(root);
 const fs::path base="sdmc:/3ds/PKSM-Rogue";
 fs::create_directories(base/"imports");
 {std::ofstream f(base/"imports/00000001.sav",std::ios::binary);
  f.write(reinterpret_cast<const char*>(initial.data()),initial.size());}
 {
  RogueInspectScreen screen;
  press(screen,KEY_R);press(screen,KEY_R); // Bag
  press(screen,KEY_Y); // stage 999 Master Balls, no installed game access
  press(screen,KEY_START); // prepare
  assert(!fs::exists(base/"drafts") || fs::is_empty(base/"drafts"));
  press(screen,KEY_START); // export
  assert(fs::is_directory(base/"originals")&&fs::is_directory(base/"drafts"));
  auto originals=fs::directory_iterator(base/"originals");
  auto drafts=fs::directory_iterator(base/"drafts");
  assert(originals!=fs::directory_iterator()&&drafts!=fs::directory_iterator());
  const auto orig=read(originals->path()),edited=read(drafts->path());
  assert(orig==initial&&read(base/"imports/00000001.sav")==initial);
  assert(orig.size()==edited.size());
  const auto after=rogue::inspectObserved(edited.data(),edited.size());
  assert(after.status==rogue::ObservedStatus::Candidate);
  assert(after.party.size()==before.party.size()&&after.money==before.money);
  assert(after.items.size()==before.items.size()+1);
  assert(after.items[0].id==1&&after.items[1].id==2);
  assert(after.items[2].id==4&&after.items[2].quantity==999);
  ++originals;++drafts;
  assert(originals==fs::directory_iterator()&&drafts==fs::directory_iterator());
  press(screen,KEY_B);
 }
 fs::current_path(previous);fs::remove_all(root);
}
