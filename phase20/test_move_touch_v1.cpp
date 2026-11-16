// SPDX-License-Identifier: GPL-3.0-or-later
#include "phase20/move_touch_v1.hpp"
#include <cassert>
#include <iostream>
using namespace phase20::gui;
using phase19::MovePage;
int main(){
    for(unsigned i=0;i<6;i++){
        auto r=moveTouchHit(MovePage::Party,(i%2)?175:20,59+(i/2)*54,6,0,0,true);
        assert(r.action==MoveTapAction::PartyRow && r.index==i);
    }
    assert(moveTouchHit(MovePage::Party,20,59,0,0,0,true).action==MoveTapAction::None);
    for(unsigned i=0;i<4;i++){
        auto r=moveTouchHit(MovePage::Slots,30,57+i*37,6,0,0,false);
        assert(r.action==MoveTapAction::SlotRow && r.index==i);
    }
    for(unsigned i=0;i<6;i++){
        auto r=moveTouchHit(MovePage::Catalog,30,50+i*26,6,13,8,false);
        assert(r.action==MoveTapAction::CatalogRow && r.index==6+i);
    }
    assert(moveTouchHit(MovePage::Catalog,30,182,6,7,6,false).action==MoveTapAction::None);
    assert(moveTouchHit(MovePage::Catalog,210,222,6,1,0,false).action==MoveTapAction::Search);
    assert(moveTouchHit(MovePage::Catalog,100,222,6,1,0,false).action==MoveTapAction::Previous);
    for(unsigned i=0;i<30;i++){
        auto r=moveTouchHit(MovePage::Keyboard,16+(i%6)*50,49+(i/6)*31,6,1,0,false);
        assert(r.action==MoveTapAction::KeyboardRow && r.index==i);
    }
    assert(moveTouchHit(MovePage::Keyboard,150,220,6,0,0,false).action==MoveTapAction::ApplySearch);
    assert(moveTouchHit(MovePage::Review,180,220,6,2,0,false).action==MoveTapAction::StartDraft);
    // Only B is permitted on all confirmations. No touch A/X on any page.
    for(auto page:{MovePage::ConfirmOne,MovePage::ConfirmTwo,MovePage::Notice}){
        assert(moveTouchHit(page,150,220,6,2,0,false).action==MoveTapAction::None);
        assert(moveTouchHit(page,20,220,6,2,0,false).action==MoveTapAction::Back);
    }
    for(unsigned i=0;i<8;i++){
        const auto page=static_cast<MovePage>(i);
        assert(moveTouchHit(page,320,48,6,3,0,false).action==MoveTapAction::None);
        assert(moveTouchHit(page,44,240,6,3,0,false).action==MoveTapAction::None);
    }
    std::cout<<"PASS: move touchscreen rows, keyboard, empty lists, "
             <<"no touch confirmation acceptance\n";
}
