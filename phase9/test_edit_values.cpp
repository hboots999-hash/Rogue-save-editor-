// Platform-neutral configurable editing widget contract.
#include "edit_values.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
using namespace rogue_sandbox;
int main() {
    Values v{};
    assert(fieldCount(Edit::Money)==1 && fieldCount(Edit::AllIV)==6);
    assert(fieldCount(Edit::OffensiveEV)==6 && fieldCount(Edit::FirstMove)==3);
    assert(valid(Edit::Money,v) && valid(Edit::OffensiveEV,v));
    assert(fieldValue(Edit::Money,v,0)==999999);
    assert(!setField(Edit::Money,v,0,1000000));
    assert(fieldValue(Edit::Money,v,0)==999999);
    assert(adjust(Edit::Money,v,0,-10000) && v.money==989999);
    assert(adjust(Edit::Money,v,0,-INT64_C(1000000000000)) && v.money==0);
    assert(adjust(Edit::Money,v,0,INT64_C(1000000000000)) && v.money==999999);
    assert(!setField(Edit::ExistingItem,v,0,0));
    assert(setField(Edit::ExistingItem,v,0,10) && v.quantity==10);
    assert(adjust(Edit::ExistingItem,v,0,-1000) && v.quantity==1);
    assert(adjust(Edit::ExistingItem,v,0,10000) && v.quantity==999);
    assert(adjust(Edit::AllIV,v,0,-11) && v.ivs[0]==20);
    assert(adjust(Edit::AllIV,v,0,-100) && v.ivs[0]==0);
    assert(adjust(Edit::AllIV,v,0,200) && v.ivs[0]==31);
    assert(!setField(Edit::AllIV,v,0,32));
    assert(!setField(Edit::AllIV,v,6,20));
    assert(!adjust(Edit::AllIV,v,6,1));
    assert(evTotal(v)==508);
    assert(setField(Edit::OffensiveEV,v,1,1) && evTotal(v)==509);
    assert(setField(Edit::OffensiveEV,v,1,3) && evTotal(v)==511);
    assert(!valid(Edit::OffensiveEV,v));
    assert(setField(Edit::OffensiveEV,v,1,2) && valid(Edit::OffensiveEV,v));
    assert(!setField(Edit::OffensiveEV,v,1,253));
    assert(!setField(Edit::FirstMove,v,0,4));
    assert(!setField(Edit::FirstMove,v,1,849));
    assert(!setField(Edit::FirstMove,v,2,0));
    assert(setField(Edit::FirstMove,v,0,3) && v.moveSlot==3);
    assert(setField(Edit::FirstMove,v,1,400) && v.moveId==400);
    assert(setField(Edit::FirstMove,v,2,40) && v.movePP==40);
    assert(valid(Edit::FirstMove,v));
    assert(adjust(Edit::FirstMove,v,1,10000) && v.moveId==848);
    assert(adjust(Edit::FirstMove,v,2,-10000) && v.movePP==1);
    Values insert{};
    assert(valid(Edit::InsertItem,insert));
    assert(insert.insertItemId==113 && insert.quantity==999);
    assert(fieldCount(Edit::InsertItem)==2);
    assert(!setField(Edit::InsertItem,insert,0,25));
    assert(!setField(Edit::InsertItem,insert,0,112));
    assert(setField(Edit::InsertItem,insert,0,1));
    assert(adjust(Edit::InsertItem,insert,0,-99) && insert.insertItemId==1);
    assert(adjust(Edit::InsertItem,insert,0,23) && insert.insertItemId==24);
    assert(adjust(Edit::InsertItem,insert,0,1) && insert.insertItemId==113);
    assert(adjust(Edit::InsertItem,insert,0,-1) && insert.insertItemId==24);
    assert(setField(Edit::InsertItem,insert,1,5) && insert.quantity==5);
    assert(!setField(Edit::InsertItem,insert,1,1000));
    assert(valid(Edit::InsertItem,insert));
    assert(largeStep(Edit::Money)==10000);
    assert(std::strcmp(fieldName(Edit::AllIV,5),"SpDef")==0);
    std::cout<<"PASS editable numeric fields: defaults, limits, 510 EV cap, move slot/ID/PP, saturated controls\n";
}
