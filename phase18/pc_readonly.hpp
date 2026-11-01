// Phase18 experimental *read-only* Emerald Rogue PC-storage candidate.
// Source model: Pokabbie/pokeemerald-rogue EX-v2.1:
// src/save.c (sections 5..13), include/pokemon_storage_system.h
// (14x30 BoxPokemon, 80 bytes, aligned offset 4).
// The exact privately installed H168 v2.1.2 ROM has NOT been byte-matched
// to that source. This is a candidate inspector, NOT a verified PC reader.
// Never use this output to approve PC edits or restoration.
#pragma once
#include "offline_engine.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace rogue18 {
constexpr size_t PcSectionData=0xff4;
constexpr size_t PcSections=9;
constexpr unsigned PcBoxes=14;
constexpr unsigned PcSlotsPerBox=30;
constexpr size_t PcBoxStride=80;
constexpr size_t PcBoxArrayOffset=4; // alignment of boxes after currentBox
constexpr size_t PcBoxNamesOffset=0x8344;
constexpr size_t PcWallpapersOffset=0x83c2;
constexpr size_t PcBytes=PcSections*PcSectionData;
static_assert(PcBoxArrayOffset+PcBoxes*PcSlotsPerBox*PcBoxStride==PcBoxNamesOffset,
              "PC geometry mismatch");
static_assert(PcWallpapersOffset+PcBoxes<PcBytes,"PC section stream too small");

enum class PcStatus : uint8_t {
    Unsupported, Corrupt, UnconfirmedEmpty, StructuralCandidate
};
struct PcSlot {
    unsigned box=0,index=0;
    uint32_t pid=0,ot=0;
    uint16_t species=0,heldItem=0,checksum=0;
};
struct PcInspection {
    PcStatus status=PcStatus::Unsupported;
    unsigned currentBox=0,occupied=0;
    std::vector<PcSlot> slots{};
};
// This source candidate requires at least one independently checksummed
// occupied BoxPokemon; empty storage alone cannot prove the PC offset.
// It never claims ROM-verified compatibility.
inline PcInspection inspectPcCandidate(const uint8_t* raw,size_t n){
    PcInspection out{};
    h168::Info info{};
    if(!validate(raw,n,info))return out;
    std::array<uint8_t,PcBytes> store{};
    const auto& bank=info.banks[info.active];
    for(unsigned i=0;i<PcSections;i++){
        const size_t offset=bank.sector[5+i];
        if(offset>=n || n-offset<PcSectionData)return out;
        std::memcpy(store.data()+i*PcSectionData,raw+offset,PcSectionData);
    }
    out.status=PcStatus::Corrupt;
    out.currentBox=store[0];
    if(out.currentBox>=PcBoxes)return out;
    for(unsigned box=0;box<PcBoxes;box++)for(unsigned slot=0;slot<PcSlotsPerBox;slot++){
        const size_t at=PcBoxArrayOffset+(box*PcSlotsPerBox+slot)*PcBoxStride;
        const auto* p=store.data()+at;
        const bool hasSpecies=(p[19]&2u)!=0;
        if(!hasSpecies){
            // Do not treat arbitrary stale bytes or a shifted structure as
            // empty; a normal cleared BoxPokemon is all zero.
            for(unsigned j=0;j<PcBoxStride;j++)if(p[j]!=0)return out;
            continue;
        }
        // Full party encoder also parses the first 80-byte BoxPokemon header.
        std::array<uint8_t,104> scratch{};
        std::memcpy(scratch.data(),p,PcBoxStride);
        Mon m{};
        if(!decodeMon(scratch.data(),scratch.size(),0,m))return out;
        if(!m.species || m.species>2047)return out;
        out.slots.push_back({box,slot,m.pid,m.ot,m.species,m.heldItem,m.checksum});
        out.occupied++;
    }
    out.status=out.occupied?PcStatus::StructuralCandidate:PcStatus::UnconfirmedEmpty;
    return out;
}
} // namespace rogue18
