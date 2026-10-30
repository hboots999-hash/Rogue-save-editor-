#pragma once
#include "sector.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace rogue {
// This is a source-derived/observed *candidate*, not identification of a ROM.
constexpr std::size_t ObservedSaveBytes=0x20000;
struct ObservedSector { uint16_t id=0,stored=0,computed=0,candidateComputed=0; bool candidateLengthKnown=false; uint32_t signature=0,counter=0; };
struct ObservedMon {
 uint16_t species=0,heldItem=0,checksum=0;
 uint8_t nature=0,abilitySlot=0; bool egg=false;
 std::array<uint8_t,6> iv{},ev{};
 std::array<uint16_t,4> moves{};
 std::array<uint8_t,4> pp{};
};
struct ObservedItem { uint16_t slot=0,id=0,quantity=0; };
enum class ObservedStatus { BadSize, NoValidCopy, CorruptOtherCopy, AmbiguousCopies,
 WrongSlotParity, UnsupportedMarker, InvalidPartyCount, InvalidPokemon, Candidate, ChecksumProfileConflict };
struct ObservedView {
 ObservedStatus status=ObservedStatus::BadSize;
 int selected=-1; uint8_t saveVersion=0; uint32_t counter=0, money=0;
 bool validCopy[2]={false,false};
 std::array<ObservedSector,28> sectors{};
 std::vector<ObservedMon> party;
 std::vector<ObservedItem> items;
};
// Pure read-only decode. Candidate does NOT prove the installed ROM identity.
// Game fields are based on the pinned 2.2.1 candidate source, not generic Gen 3.
ObservedView inspectObserved(const uint8_t* save,std::size_t length);
}
