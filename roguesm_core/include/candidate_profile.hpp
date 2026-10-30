#pragma once
#include "sector.hpp"
#include <cstddef>
#include <cstdint>
namespace rogue {
// Both candidate profiles have the same measured lengths under ARM APCS-GNU.
// 2.1.2 candidate: 518b7b4d; 2.2.1 candidate: a6adfcf.
// This profile is NOT certified for any released ROM or for live save access.
constexpr SectorSizes kCandidateSectorSizes = {{
 0x0ef4, 0x0ff4, 0x0ff4, 0x0ff4, 0x0480,
 0x0ff4, 0x0ff4, 0x0ff4, 0x0ff4, 0x0ff4,
 0x0ff4, 0x0ff4, 0x0ff4, 0x05d0
}};
constexpr std::size_t kFlashSize=0x20000;
enum class CandidateStatus { BadSize, NoValidSlot, CorruptOtherSlot, AmbiguousSlots, SlotParityMismatch, UnsupportedMarker, CandidateOnly };
struct CandidateResult {
 CandidateStatus status=CandidateStatus::BadSize;
 int selected=-1;
 uint32_t counter=0;
 bool slotValid[2]={false,false};
};
// Pure read-only analysis only. CandidateOnly does NOT authorize interpreting
// unrelated game data or writing: the marker is not a unique ROM fingerprint.
CandidateResult inspectCandidate(const uint8_t* flash, std::size_t length); // 2.2.1 candidate
CandidateResult inspectCandidate212(const uint8_t* flash, std::size_t length); // 2.1.2 candidate
}
