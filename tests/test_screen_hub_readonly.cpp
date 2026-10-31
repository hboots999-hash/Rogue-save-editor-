#include "RogueInspectScreen.hpp"
#include "3ds.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <unistd.h>
namespace fs=std::filesystem;
static std::vector<unsigned char> bytes(const fs::path& p){
 std::ifstream s(p,std::ios::binary);
 return {(std::istreambuf_iterator<char>(s)),{}};
}
static void press(RogueInspectScreen& s,u32 key){simulatedKeysDown=key;s.update(nullptr);}
int main(int argc,char**argv){
 assert(argc==2);
 const auto initial=bytes(argv[1]);assert(initial.size()==131072);
 char temp[]="/tmp/pksm-rogue-hub-page-XXXXXX";
 const auto root=mkdtemp(temp);assert(root);
 const auto cwd=fs::current_path();fs::current_path(root);
 const fs::path base="sdmc:/3ds/PKSM-Rogue";
 fs::create_directories(base/"imports");
 {std::ofstream f(base/"imports/00000001.sav",std::ios::binary);
 f.write((const char*)initial.data(),initial.size());}
 {
  RogueInspectScreen screen;
  press(screen,KEY_R);press(screen,KEY_R);press(screen,KEY_R); // Hub page
  press(screen,KEY_A);
  press(screen,KEY_X);
  press(screen,KEY_Y);
  press(screen,KEY_START);
  press(screen,KEY_DOWN);
  press(screen,KEY_START);
  assert(bytes(base/"imports/00000001.sav")==initial);
  assert(!fs::exists(base/"drafts")||fs::is_empty(base/"drafts"));
  assert(!fs::exists(base/"originals")||fs::is_empty(base/"originals"));
  press(screen,KEY_B);
 }
 fs::current_path(cwd);fs::remove_all(root);
}
