// SPDX-License-Identifier: GPL-3.0-or-later
// All touchscreen targets are geometrically bounded; no save logic here.
#include "phase20/touch_layout_v1.hpp"
#include <cassert>
#include <iostream>
using namespace phase20::gui;
int main(){
    for(unsigned i=0;i<6;i++){
        const unsigned x=(i%2)?180:20, y=61+(i/2)*54;
        const Hit h=hitTest(Page::Home,x,y,6,false);
        assert(h.action==Action::HomeItem && h.index==i);
        const Hit p=hitTest(Page::Party,x,y,6,false);
        assert(p.action==Action::PartyItem && p.index==i);
    }
    assert(hitTest(Page::Home,20,223,6,false).action==Action::HomeItem);
    assert(hitTest(Page::Home,20,223,6,false).index==6);
    assert(hitTest(Page::Party,15,220,6,false).action==Action::Back);
    assert(hitTest(Page::Party,150,220,6,false).action==Action::PartyMoves);
    assert(hitTest(Page::Pokemon,20,218,6,false).action==Action::Back);
    assert(hitTest(Page::Pokemon,140,218,6,false).action==Action::PokemonEdit);
    assert(hitTest(Page::Pokemon,250,218,6,false).action==Action::PokemonMoves);
    assert(hitTest(Page::Pokemon,180,218,6,false).action==Action::PokemonAbility);
    for(unsigned i=0;i<3;i++){
        auto tab=hitTest(Page::Pokemon,20+i*103,115,6,false);
        assert(tab.action==Action::PokemonTab && tab.index==i);
        auto pcTab=hitTest(Page::Pokemon,20+i*103,115,6,true);
        assert(pcTab.action==Action::PokemonTab && pcTab.index==i);
    }
    assert(hitTest(Page::Pokemon,108,115,6,false).action==Action::None);
    assert(hitTest(Page::Pokemon,20,133,6,false).action==Action::None);

    assert(hitTest(Page::Pokemon,180,218,6,true).action==Action::None);
    assert(hitTest(Page::Pokemon,140,218,6,true).action==Action::None);
    assert(hitTest(Page::Pokemon,250,218,6,true).action==Action::None);
    assert(hitTest(Page::Party,20,61,0,false).action==Action::None);
    assert(hitTest(Page::Party,180,169,3,false).action==Action::None);
    for(unsigned page=0;page<4;page++) {
        assert(hitTest(static_cast<Page>(page),320,40,6,false).action==Action::None);
        assert(hitTest(static_cast<Page>(page),40,240,6,false).action==Action::None);
        assert(hitTest(static_cast<Page>(page),999,999,6,false).action==Action::None);
    }
    assert(hitTest(Page::Other,20,61,6,false).action==Action::None);
    assert(hitTest(Page::Home,160,61,6,false).action==Action::None); // card gutter
    std::cout<<"PASS touchscreen layout: 7 home, 6 party, controls, empty/PC and bounds\n";
}
