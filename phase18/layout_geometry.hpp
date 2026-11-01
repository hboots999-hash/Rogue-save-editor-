// Phase18: EX-v2.1 C struct geometry, derived from field sizes rather
// than stale handwritten offsets in include/global.h. No save I/O.
#pragma once
#include <cstddef>
#include <cstdint>

namespace rogue18 {
namespace layout {
constexpr size_t PartyCount=0x234;
constexpr size_t PartyStart=0x238;
constexpr size_t PartyMax=6;
constexpr size_t PartyStride=104; // EX struct Pokemon includes 4 extra bytes
constexpr size_t Money=PartyStart+PartyMax*PartyStride;
constexpr size_t BagSortMode=Money+4;
constexpr size_t BagUpgradeCandidate=BagSortMode+1;
constexpr size_t PCItemArray=Money+8;
constexpr size_t PCItemsCount=50;
constexpr size_t ItemSlotStride=4;
constexpr size_t BagArray=PCItemArray+PCItemsCount*ItemSlotStride;
constexpr size_t BagPhysicalSlots=450;
static_assert(Money==0x4a8,"EX source-derived money must match observed H168");
static_assert(BagUpgradeCandidate==0x4ad,"candidate offset differs from EX geometry");
static_assert(PCItemArray==0x4b0,"EX PC item array");
static_assert(BagArray==0x578,"EX source-derived bag must match observed H168");
static_assert(BagArray+BagPhysicalSlots*ItemSlotStride<0xff4,
              "bag must fit first SaveBlock1 flash sector");
// Candidate means source geometry only. It does not confirm the exact
// installed H168 build's bag-upgrade semantics.
inline uint8_t bagUpgradeCandidate(const uint8_t* source,size_t sector1){
    return source[sector1+BagUpgradeCandidate];
}
} // namespace layout
} // namespace rogue18
