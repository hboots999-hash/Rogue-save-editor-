// SPDX-License-Identifier: GPL-3.0-or-later
#include "phase20/sprite_cache_policy_v1.hpp"
#include <cassert>
#include <iostream>
using namespace phase20::gui;
int main(){
    SpriteCachePolicy p{};
    bool hit=true;
    assert(p.locate(0,hit)==SpriteCacheSlots && !hit);
    assert(p.locate(10000,hit)==SpriteCacheSlots && !hit);
    for(unsigned i=0;i<SpriteCacheSlots;i++){
        assert(p.locate(100+i,hit)==i && !hit);
        p.index[i].state=SpriteState::Present;
    }
    assert(p.locate(101,hit)==1 && hit);
    assert(p.locate(1000,hit)==0 && !hit); // oldest slot evicted
    p.index[0].state=SpriteState::Missing;
    assert(p.locate(1000,hit)==0 && hit); // negative result cached
    p.index[0].state=SpriteState::Invalid;
    assert(p.locate(1000,hit)==0 && hit); // invalid result cached
    assert(p.locate(102,hit)==2 && hit);
    for(unsigned i=0;i<1000;i++){
        const unsigned species=100+(i%18);
        const unsigned slot=p.locate(species,hit);
        assert(slot<SpriteCacheSlots);
        p.index[slot].state=SpriteState::Present;
        assert(p.index[slot].species==species);
    }
    assert(p.evictions>0 && p.hits>0);
    p.clock=UINT64_MAX;
    (void)p.locate(119,hit);
    p.reset();
    assert(p.hits==0 && p.misses==0 && p.evictions==0);
    for(const auto& e:p.index)assert(e.species==0 &&
                                      e.state==SpriteState::Unknown);
    std::cout<<"PASS sprite cache LRU, collision, negative caching, refresh, bounds\n";
}
