// SPDX-License-Identifier: GPL-3.0-or-later
#include "phase20/profile_view_v1.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace phase20::profile;
int main(){
    assert(std::strcmp(natureName(0),"HARDY")==0);
    assert(std::strcmp(natureName(3),"ADAMANT")==0);
    assert(std::strcmp(natureName(12),"SERIOUS")==0);
    assert(std::strcmp(natureName(24),"QUIRKY")==0);
    assert(std::strcmp(natureName(25),"UNVERIFIED")==0);
    for(unsigned n=0;n<25;n++){
        assert(natureName(n)[0]);
        for(unsigned m=0;m<n;m++)assert(std::strcmp(natureName(m),natureName(n))!=0);
    }
    assert(std::strcmp(abilitySlotLabel(0),"SLOT 1")==0);
    assert(std::strcmp(abilitySlotLabel(2),"SLOT 3")==0);
    assert(std::strcmp(abilitySlotLabel(3),"INVALID SLOT")==0);
    assert(cycle(Tab::Overview,true)==Tab::Training);
    assert(cycle(Tab::Training,true)==Tab::Moves);
    assert(cycle(Tab::Moves,true)==Tab::Overview);
    assert(cycle(Tab::Overview,false)==Tab::Moves);
    assert(fromIndex(999u)==Tab::Overview);
    phase19::Pokemon p{};
    p.ev={252,252,6,0,0,0};
    assert(evTotal(p)==510);
    static phase19::MoveRecord moves[2]{};
    moves[0].id=15;std::strcpy(moves[0].name,"CUT");
    moves[1].id=85;std::strcpy(moves[1].name,"THUNDERBOLT");
    phase19::MoveCatalog catalog{};
    catalog.records=moves;catalog.count=2;
    catalog.origin=phase19::CatalogOrigin::Synthetic;
    assert(std::strcmp(knownMoveName(&catalog,true,85),"THUNDERBOLT")==0);
    assert(knownMoveName(&catalog,false,85)==nullptr);
    assert(knownMoveName(nullptr,true,85)==nullptr);
    assert(knownMoveName(&catalog,true,999)==nullptr);
    assert(knownMoveName(&catalog,true,0)==nullptr);
    catalog.origin=phase19::CatalogOrigin::ExactRomReviewed;
    assert(knownMoveName(&catalog,true,85)==nullptr);
    assert(std::strcmp(knownMoveName(&catalog,false,15),"CUT")==0);
    catalog.origin=phase19::CatalogOrigin::Unauthenticated;
    assert(knownMoveName(&catalog,false,85)==nullptr);
    catalog.count=1000;
    assert(knownMoveName(&catalog,false,85)==nullptr);
    phase19::Demo d{};
    d.synthetic=true;
    d.partyStatus=phase19::Availability::Ready;d.partyCount=2;
    d.party[0].species=25;d.party[0].nature=3;d.party[0].ability=0;
    d.party[1].species=149;d.party[1].nature=12;d.party[1].ability=2;
    phase19::State s(d);s.screen=phase19::Screen::Pokemon;s.mon=0;
    View view{};
    view.sync(s);
    assert(view.pinned && view.tab==Tab::Overview);
    view.tab=Tab::Moves;
    view.sync(s);
    assert(view.tab==Tab::Moves); // no spurious reset on redraw
    s.mon=1;view.sync(s);
    assert(view.tab==Tab::Overview);
    view.tab=Tab::Training;
    d.sourceSha256[0]=1;view.sync(s);
    assert(view.tab==Tab::Overview);
    view.tab=Tab::Moves;
    d.party[1].ability=1;view.sync(s);
    assert(view.tab==Tab::Overview);
    s.screen=phase19::Screen::Party;view.sync(s);
    assert(!view.pinned && view.tab==Tab::Overview);
    s.screen=phase19::Screen::Pokemon;s.fromPC=true;
    d.pcStatus=phase19::Availability::Ready;d.pcBoxes=1;d.pcCounts[0]=1;
    d.pc[0][0].species=25;s.mon=0;
    view.sync(s);assert(view.pinned && view.pc);
    assert(!DetailTabsWriteInstalledSave);
    std::cout<<"PASS detail tabs: nature map, ability labels, catalog source locks,"
             <<" stale selection resets, PC read-only\n";
}
