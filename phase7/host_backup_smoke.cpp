// Independent, synthetic-only smoke test for Phase 8 SD-copy backup gate.
#include "RogueInspectScreen.hpp"
#include "3ds.h"
#include "gui.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <unistd.h>
namespace fs=std::filesystem;
using Bytes=std::vector<uint8_t>;
static Bytes bytes(const fs::path& p){std::ifstream f(p,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
static void writeBytes(const fs::path& p,const Bytes& b){std::ofstream f(p,std::ios::binary|std::ios::trunc);assert(f);f.write(reinterpret_cast<const char*>(b.data()),b.size());assert(f.good());}
static void key(RogueInspectScreen& sc,u32 k){stub_key_state()=k;sc.update(nullptr);}
static bool shown(RogueInspectScreen& sc,const std::string& s){Gui::captured_text().clear();sc.drawTop();return std::any_of(Gui::captured_text().begin(),Gui::captured_text().end(),[&](auto&x){return x.find(s)!=std::string::npos;});}
static void exportMoney(RogueInspectScreen& sc){key(sc,KEY_X);key(sc,KEY_A);key(sc,KEY_A);}
int main(int argc,char**argv){
 assert(argc==2);
 const auto orig=bytes(argv[1]);assert(orig.size()==131072);
 const auto prev=fs::current_path();
 const auto dir=fs::temp_directory_path()/("rogue_ci_backup_gate_"+std::to_string(getpid()));
 assert(!fs::exists(dir));
 fs::create_directories(dir/"sdmc:/3ds/PKSM-Rogue/imports");
 fs::current_path(dir);
 const auto imp=dir/"sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
 const auto backups=dir/"sdmc:/3ds/PKSM-Rogue/backups";
 const auto drafts=dir/"sdmc:/3ds/PKSM-Rogue/drafts";
 writeBytes(imp,orig);
 {
   RogueInspectScreen screen;
   assert(shown(screen,"backup: required"));
   // Fail closed when the expected backup directory is actually a file.
   {std::ofstream f(backups);f<<"SENTINEL";}
   exportMoney(screen);
   assert(shown(screen,"backup FAILED: draft BLOCKED"));
   assert(!fs::exists(drafts));
   assert(bytes(imp)==orig);
   assert(bytes(backups)==Bytes({'S','E','N','T','I','N','E','L'}));
   fs::remove(backups);
   exportMoney(screen);
   assert(shown(screen,"Draft exported: rogue_001.sav"));
   assert(bytes(backups/"import_001.sav")==orig);
   assert(bytes(imp)==orig);
   assert(bytes(drafts/"rogue_001.sav")!=orig);
   assert(shown(screen,"Draft exported:"));
 }
 {
   RogueInspectScreen second;
   exportMoney(second);
   assert(bytes(backups/"import_002.sav")==orig);
   assert(bytes(imp)==orig);
 }
 {
   RogueInspectScreen stale;
   auto changed=orig;changed.back()^=0x5a;
   writeBytes(imp,changed);
   exportMoney(stale);
   assert(shown(stale,"SD import changed: draft BLOCKED"));
   assert(!fs::exists(backups/"import_003.sav"));
   assert(bytes(imp)==changed);
   writeBytes(imp,orig);
 }
 fs::current_path(prev);
 fs::remove_all(dir);
 std::cout<<"PASS synthetic backup gate: write failure, unchanged input, distinct backup, stale import refusal\n";
}
