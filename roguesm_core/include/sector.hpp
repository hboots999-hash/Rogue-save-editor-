#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace rogue {
constexpr std::size_t SectorBytes=4096, SectorData=4084, SectorsPerSlot=14;
using SectorSizes=std::array<uint16_t,SectorsPerSlot>;
struct SlotStatus { bool valid=false; uint32_t counter=0; std::array<std::size_t,SectorsPerSlot> physical{}; };
uint16_t checksum(const uint8_t* data, std::size_t length);
SlotStatus validateSlot(const uint8_t* data, std::size_t length, const SectorSizes& sizes);
// Returns -1 on neither valid or on non-adjacent or ambiguous counter order.
int chooseSlot(const SlotStatus& a,const SlotStatus& b);
}
