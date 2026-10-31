#include "rogue_bridge.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
int main() {
    rogue_preview r{};
    assert(!rogue_preview_copy(nullptr,0,&r));
    assert(r.profile == ROGUE_UNSUPPORTED);
    assert(!rogue_preview_copy(nullptr,0,nullptr));
    std::vector<std::uint8_t> garbage(ROGUE_RAW_SAVE_SIZE,0);
    const auto original = garbage;
    assert(!rogue_preview_copy(garbage.data(),garbage.size(),&r));
    assert(r.party_count == 0);
    assert(!rogue_snapshot_open(garbage.data(),garbage.size()));
    assert(!rogue_snapshot_open(garbage.data(),garbage.size()-1));
    rogue_snapshot_close(nullptr);
    assert(!rogue_snapshot_summary(nullptr,&r));
    assert(r.money == 0);
    rogue_party_candidate p{};
    rogue_item_candidate item{};
    assert(!rogue_get_party_candidate(garbage.data(),garbage.size(),0,&p));
    assert(!rogue_get_item_candidate(garbage.data(),garbage.size(),0,&item));
    assert(p.species_id == 0 && item.quantity == 0);
    assert(!rogue_snapshot_party(nullptr,0,&p));
    assert(!rogue_snapshot_item(nullptr,0,&item));
    assert(!rogue_snapshot_party(nullptr,0,nullptr));
    assert(!rogue_snapshot_item(nullptr,0,nullptr));
    assert(original == garbage);
    return 0;
}
