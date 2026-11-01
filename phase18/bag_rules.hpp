// Phase18 source-derived *geometry* and conservative insertion evidence.
// EX-v2.1 source: include/constants/{global,item,rogue}.h;
// src/{item,rogue_controller}.c. This is not a verified H168 ROM ID map.
#pragma once
#include <cstdint>
#include <vector>
#include <utility>

namespace rogue18 {
constexpr unsigned BagSlotsPhysical=450;
constexpr unsigned BagReservedKeySlots=50;
constexpr unsigned BagBaseSlots=80;          // 450 - 37 upgrades * 10
constexpr unsigned BagSlotsPerUpgrade=10;
constexpr unsigned BagMaxUpgrades=37;
constexpr unsigned BagStackMaximum=999;
// EX-v2.1 ROGUE_EXPANDED_POCKETS uses pocket 1..9; key items are pocket 9.
constexpr uint8_t KeyItemsPocket=9;
inline unsigned capacityFromUpgrades(unsigned upgrades){
    if(upgrades>BagMaxUpgrades)upgrades=BagMaxUpgrades;
    return BagBaseSlots+upgrades*BagSlotsPerUpgrade;
}
inline unsigned normalCapacityFromUpgrades(unsigned upgrades){
    return capacityFromUpgrades(upgrades)-BagReservedKeySlots;
}
// An external, version-matched ROM decoder supplies pocket classifications,
// including *all* existing items. The file alone contains no pocket ID.
// Never guess an item's pocket from its numeric ID or current position.
inline bool knownPocket(const std::vector<std::pair<uint16_t,uint8_t>>& mapping,
                        uint16_t id,uint8_t& pocket){
    bool found=false;
    for(const auto& entry:mapping){
        if(entry.first!=id)continue;
        if(found || entry.second<1 || entry.second>9)return false;
        found=true;pocket=entry.second;
    }
    return found;
}
inline bool insertionCompatible(
    const std::vector<std::pair<uint16_t,uint8_t>>& verifiedMapping,
    const std::vector<uint16_t>& existingIds,uint16_t insertId,
    unsigned verifiedUpgradeCount){
    if(verifiedUpgradeCount>BagMaxUpgrades ||
       existingIds.size()>=BagSlotsPhysical)return false;
    uint8_t previous=0,newPocket=0;
    unsigned keyUsed=0,normalUsed=0;
    for(uint16_t id:existingIds){
        uint8_t p=0;
        if(!knownPocket(verifiedMapping,id,p) || p<previous)return false;
        previous=p;
        if(p==KeyItemsPocket)++keyUsed;
        else ++normalUsed;
    }
    if(!knownPocket(verifiedMapping,insertId,newPocket) ||
       newPocket<previous)return false; // append must preserve pocket ordering
    if(newPocket==KeyItemsPocket)++keyUsed;
    else ++normalUsed;
    return keyUsed<=BagReservedKeySlots &&
           normalUsed<=normalCapacityFromUpgrades(verifiedUpgradeCount);
}
} // namespace rogue18
