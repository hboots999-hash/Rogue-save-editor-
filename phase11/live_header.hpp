/* Phase 11 GBA VC container parser: read-only, no IO.
 * Original implementation based on documented GBA VC .SAV slot structure.
 * This is neither a writer nor proof of valid CMAC.
 * Inspired by GPLv3 Checkpoint GbaSave, 2026; original code here.
 */
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace rogue_live {
constexpr uint64_t ExpectedTitle = 0x0004000007D7BA00ULL;
constexpr uint32_t RawSize = 131072;
constexpr uint32_t HeaderSize = 0x200;
constexpr uint32_t SlotSize = HeaderSize + RawSize;
constexpr uint32_t ContainerMin = 2 * SlotSize;

inline uint32_t load32(const uint8_t* p,size_t k){
    return uint32_t(p[k])|(uint32_t(p[k+1])<<8)|
           (uint32_t(p[k+2])<<16)|(uint32_t(p[k+3])<<24);
}
inline uint64_t load64(const uint8_t* p,size_t k){
    return uint64_t(load32(p,k))|(uint64_t(load32(p,k+4))<<32);
}
struct Slot {
    bool valid=false;
    uint32_t offset=0;
    uint32_t timesSaved=0;
    uint64_t title=0;
    uint32_t saveSize=0;
};
inline Slot parseHeader(const uint8_t* header,size_t n,uint32_t offset){
    Slot slot{};
    if(!header || n<HeaderSize || std::memcmp(header,".SAV",4)!=0)return slot;
    const uint32_t start=load32(header,0x50);
    const uint32_t size=load32(header,0x54);
    if(start!=HeaderSize || size!=RawSize)return slot;
    slot.valid=true;
    slot.offset=offset;
    slot.timesSaved=load32(header,0x34);
    slot.title=load64(header,0x38);
    slot.saveSize=size;
    return slot;
}
// The global latest slot MUST match our Emerald Rogue title; never use
// a stale slot with matching title if another GBA VC was played last.
inline bool selectLatest(const Slot& top,const Slot& bottom,Slot& selected){
    if(!top.valid && !bottom.valid)return false;
    selected= !top.valid ? bottom :
              !bottom.valid ? top :
              (bottom.timesSaved>top.timesSaved?bottom:top);
    return selected.title==ExpectedTitle && selected.saveSize==RawSize;
}
} // namespace rogue_live
