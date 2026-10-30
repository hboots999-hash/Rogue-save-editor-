#include "rogue_edit.h"
#include "observed_profile.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
int main(int argc, char** argv) {
 if(argc!=2)return 2;
 std::ifstream in(argv[1],std::ios::binary);
 const std::vector<uint8_t> original((std::istreambuf_iterator<char>(in)),{});
 assert(original.size()==ROGUE_RAW_SAVE_SIZE);
 std::vector<uint8_t> output(original.size(),0xaa);
 rogue_draft_request r{};r.change_money=1;r.money=999999;r.change_quantity=1;r.item_slot=0;r.expected_item_id=1;r.quantity=999;
 size_t changed=0;
 auto code=rogue_make_offline_draft(original.data(),original.size(),&r,output.data(),output.size(),&changed);
 assert(code==ROGUE_EDIT_OK && changed>0 && changed<=10);
 auto before=rogue::inspectObserved(original.data(),original.size());
 auto after=rogue::inspectObserved(output.data(),output.size());
 assert(before.status==rogue::ObservedStatus::Candidate);
 assert(after.status==rogue::ObservedStatus::Candidate);
 assert(before.counter==after.counter&&before.saveVersion==after.saveVersion);
 assert(before.money==123456&&after.money==999999);
 assert(before.items.size()==2 && after.items.size()==2);
 assert(after.items[0].id==1 && after.items[0].quantity==999);
 assert(after.items[1].quantity==75);
 assert(before.party.size()==after.party.size());
 assert(rogue_make_offline_draft(original.data(),original.size(),&r,output.data(),original.size()-1,&changed)==ROGUE_EDIT_BAD_ARGUMENT);
 assert(changed==0);
 assert(rogue_make_offline_draft(original.data(),original.size(),&r,output.data(),output.size(),&changed)==ROGUE_EDIT_OK);
 r.money=1000000;
 assert(rogue_make_offline_draft(original.data(),original.size(),&r,output.data(),output.size(),&changed)==ROGUE_EDIT_BAD_MONEY);
 r.money=999999;r.expected_item_id=3;
 assert(rogue_make_offline_draft(original.data(),original.size(),&r,output.data(),output.size(),&changed)==ROGUE_EDIT_STALE_ITEM);
 r.expected_item_id=1;r.item_slot=2;
 assert(rogue_make_offline_draft(original.data(),original.size(),&r,output.data(),output.size(),&changed)==ROGUE_EDIT_STALE_ITEM);
 r.item_slot=0;r.quantity=0;
 assert(rogue_make_offline_draft(original.data(),original.size(),&r,output.data(),output.size(),&changed)==ROGUE_EDIT_BAD_QUANTITY);
 std::puts("PASS offline money/item draft: readback, selected sector, stale IDs, limits");
}