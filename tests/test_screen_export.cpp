#include "RogueInspectScreen.hpp"
#include "3ds.h"
#include "observed_profile.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <unistd.h>
namespace fs=std::filesystem;
static std::vector<uint8_t> read(const fs::path& path){
 std::ifstream f(path,std::ios::binary); return {(std::istreambuf_iterator<char>(f)),{}};
}
static void press(RogueInspectScreen& s, u32 key){
 simulatedKeysDown=key; s.update(nullptr);
}
int main(int argc,char**argv){
 assert(argc==2);
 auto original=read(argv[1]);assert(original.size()==ROGUE_RAW_SAVE_SIZE);
 char p[]="/tmp/rogue-pksm-edit-XXXXXX";auto* root=mkdtemp(p);assert(root);
 fs::path prev=fs::current_path();
 fs::current_path(root);
 fs::path base="sdmc:/3ds/PKSM-Rogue";
 fs::create_directories(base/"imports");
 auto source=base/"imports/00000001.sav";
 {std::ofstream f(source,std::ios::binary); f.write((const char*)original.data(),original.size());}
 {
 RogueInspectScreen screen;
 // Arm a money-only draft, preview, explicitly confirm.
 press(screen,KEY_A); press(screen,KEY_START);
 assert(!fs::exists(base/"drafts") || fs::is_empty(base/"drafts"));
 press(screen,KEY_START);
 assert(read(source)==original);
 assert(fs::exists(base/"originals") && fs::exists(base/"drafts"));
 auto listing=[](const fs::path& p){std::vector<fs::path> a;for(auto& e:fs::directory_iterator(p))a.push_back(e.path());return a;};
 auto origs=listing(base/"originals"),drafts=listing(base/"drafts");
 assert(origs.size()==1 && drafts.size()==1);
 assert(read(origs.front())==original);
 auto output=read(drafts.front());
 auto view=rogue::inspectObserved(output.data(),output.size());
 assert(view.status==rogue::ObservedStatus::Candidate && view.money==999999);
 assert(view.items.size()==2 && view.items[0].quantity==50);
 // Now stage an item quantity and require two-step confirmation again.
 press(screen,KEY_R);press(screen,KEY_R); // Summary -> Party -> Bag
 press(screen,KEY_X); press(screen,KEY_START); press(screen,KEY_START);
 origs=listing(base/"originals");drafts=listing(base/"drafts");
 assert(origs.size()==2 && drafts.size()==2);
 bool foundItem=false, foundMoney=false;
 for(const auto& file:drafts){
  auto bytes=read(file);auto v=rogue::inspectObserved(bytes.data(),bytes.size());
  assert(v.status==rogue::ObservedStatus::Candidate);
  if(v.money==999999 && v.items[0].quantity==50)foundMoney=true;
  if(v.money==123456 && v.items[0].quantity==999)foundItem=true;
 }
 assert(foundMoney && foundItem);
 for(const auto& file:origs)assert(read(file)==original);
 assert(read(source)==original);
 press(screen,KEY_B);
 }
 fs::current_path(prev);
 fs::remove_all(root);
 std::puts("PASS: simulated PKSM buttons export two distinct verified offline drafts; original never changed");
}