// SPDX-License-Identifier: GPL-3.0-or-later
// Pure bounded sprite-cache policy. No filesystem, ROM or 3DS dependency.
#pragma once
#include <array>
#include <cstdint>
namespace phase20 { namespace gui {
enum class SpriteState : uint8_t { Unknown, Present, Missing, Invalid };
constexpr unsigned SpriteCacheSlots=10u;
struct SpriteCacheEntry {
    unsigned species=0;
    SpriteState state=SpriteState::Unknown;
    uint64_t lastUsed=0;
};
struct SpriteCachePolicy {
    std::array<SpriteCacheEntry,SpriteCacheSlots> index{};
    uint64_t clock=0;
    unsigned hits=0,misses=0,evictions=0;
    void reset(){index={};clock=0;hits=misses=evictions=0;}
    unsigned locate(unsigned species,bool& cached){
        cached=false;
        if(species==0u || species>9999u)return SpriteCacheSlots;
        // Logical counter wrap is not a stale-cache eviction proof.
        if(clock==UINT64_MAX){
            // Wrap happens only after 2^64 lookups; cache remains correct and
            // we conservatively restart its bounded eviction chronology.
            for(auto& e:index)e.lastUsed=1;
            clock=1;
        }
        for(unsigned i=0;i<SpriteCacheSlots;i++)
            if(index[i].species==species && index[i].state!=SpriteState::Unknown){
                index[i].lastUsed=++clock;++hits;cached=true;return i;
            }
        ++misses;
        unsigned victim=SpriteCacheSlots;
        for(unsigned i=0;i<SpriteCacheSlots;i++)if(index[i].species==0u){
            victim=i;break;
        }
        if(victim==SpriteCacheSlots){
            victim=0;
            for(unsigned i=1;i<SpriteCacheSlots;i++)
                if(index[i].lastUsed<index[victim].lastUsed)victim=i;
            ++evictions;
        }
        index[victim]={species,SpriteState::Unknown,++clock};
        return victim;
    }
};
}} // namespace phase20::gui
