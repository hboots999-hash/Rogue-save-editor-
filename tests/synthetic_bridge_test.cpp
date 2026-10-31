#include "rogue_bridge.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
int main(int argc,char**argv) {
    if(argc!=2) return 2;
    std::ifstream in(argv[1],std::ios::binary);
    if(!in) return 3;
    const std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)),{});
    assert(data.size()==ROGUE_RAW_SAVE_SIZE);
    rogue_snapshot* snapshot=rogue_snapshot_open(data.data(),data.size());
    if(!snapshot) { std::fprintf(stderr,"fixture rejected\n");return 4; }
    rogue_preview v{};
    assert(rogue_snapshot_summary(snapshot,&v));
    assert(v.profile == ROGUE_CANDIDATE_EX_212);
    assert(v.serialized_marker==6);
    assert(v.money==123456);
    assert(v.party_count==0);
    assert(v.occupied_bag_slots==2);
    rogue_party_candidate p{};
    assert(!rogue_snapshot_party(snapshot,0,&p));
    rogue_item_candidate a{},b{};
    assert(rogue_snapshot_item(snapshot,0,&a));
    assert(rogue_snapshot_item(snapshot,1,&b));
    assert(a.slot_index==0 && a.item_id==1 && a.quantity==50);
    assert(b.slot_index==1 && b.item_id==2 && b.quantity==75);
    assert(!rogue_snapshot_item(snapshot,2,&a));
    rogue_snapshot_close(snapshot);
    std::puts("PASS: synthetic fixture money=123456 party=0 bag=1x50,2x75");
}
