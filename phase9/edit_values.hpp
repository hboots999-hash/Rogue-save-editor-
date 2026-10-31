// Phase 9.4 numerical edit parameters and range-limited controller.
// No file access or game-save access. All values are toy-fixture proposals.
#pragma once
#include <cstdint>
#include <cstddef>

namespace rogue_sandbox {
enum class Edit { Money, ExistingItem, AllIV, OffensiveEV, FirstMove };

struct Values {
    uint32_t money = 999999;
    uint16_t quantity = 999;
    uint8_t ivs[6] = {31,31,31,31,31,31};
    uint8_t evs[6] = {4,0,0,252,252,0};
    uint8_t moveSlot = 0;
    uint16_t moveId = 85;
    uint8_t movePP = 15;
};
inline unsigned fieldCount(Edit k) {
    return k==Edit::AllIV || k==Edit::OffensiveEV ? 6u :
           k==Edit::FirstMove ? 3u : 1u;
}
inline const char* fieldName(Edit k,unsigned field) {
    static const char* names[6]={"HP","Attack","Defense","Speed","SpAtk","SpDef"};
    if(field>=fieldCount(k))return "Invalid field";
    if(k==Edit::AllIV || k==Edit::OffensiveEV)return names[field];
    if(k==Edit::Money)return "Money";
    if(k==Edit::ExistingItem)return "Quantity";
    return field==0 ? "Move slot (0-3)" : field==1 ? "Move ID" : "PP";
}
inline uint32_t fieldValue(Edit k,const Values& v,unsigned field) {
    if(field>=fieldCount(k))return 0;
    if(k==Edit::Money)return v.money;
    if(k==Edit::ExistingItem)return v.quantity;
    if(k==Edit::AllIV)return v.ivs[field];
    if(k==Edit::OffensiveEV)return v.evs[field];
    return field==0 ? v.moveSlot : field==1 ? v.moveId : v.movePP;
}
inline uint32_t minValue(Edit k,unsigned field) {
    if(k==Edit::ExistingItem)return 1;
    if(k==Edit::FirstMove && field!=0)return 1;
    return 0;
}
inline uint32_t maxValue(Edit k,unsigned field) {
    if(k==Edit::Money)return 999999;
    if(k==Edit::ExistingItem)return 999;
    if(k==Edit::AllIV)return 31;
    if(k==Edit::OffensiveEV)return 252;
    if(field==0)return 3;
    if(field==1)return 848;
    return 40;
}
inline bool setField(Edit k,Values& v,unsigned field,uint32_t n) {
    if(field>=fieldCount(k) || n<minValue(k,field) || n>maxValue(k,field))return false;
    if(k==Edit::Money)v.money=n;
    else if(k==Edit::ExistingItem)v.quantity=static_cast<uint16_t>(n);
    else if(k==Edit::AllIV)v.ivs[field]=static_cast<uint8_t>(n);
    else if(k==Edit::OffensiveEV)v.evs[field]=static_cast<uint8_t>(n);
    else if(field==0)v.moveSlot=static_cast<uint8_t>(n);
    else if(field==1)v.moveId=static_cast<uint16_t>(n);
    else v.movePP=static_cast<uint8_t>(n);
    return true;
}
inline bool adjust(Edit k,Values& v,unsigned field,int64_t delta) {
    if(field>=fieldCount(k))return false;
    int64_t value=static_cast<int64_t>(fieldValue(k,v,field))+delta;
    if(value<static_cast<int64_t>(minValue(k,field)))
        value=minValue(k,field);
    if(value>static_cast<int64_t>(maxValue(k,field)))
        value=maxValue(k,field);
    return setField(k,v,field,static_cast<uint32_t>(value));
}
inline unsigned evTotal(const Values& v) {
    unsigned sum=0;
    for(auto n:v.evs)sum+=n;
    return sum;
}
inline bool valid(Edit k,const Values& v) {
    for(unsigned i=0;i<fieldCount(k);++i){
        const uint32_t n=fieldValue(k,v,i);
        if(n<minValue(k,i) || n>maxValue(k,i))return false;
    }
    return k!=Edit::OffensiveEV || evTotal(v)<=510;
}
inline int64_t largeStep(Edit k) {
    return k==Edit::Money ? 10000 : k==Edit::FirstMove ? 10 : 10;
}
} // namespace rogue_sandbox
