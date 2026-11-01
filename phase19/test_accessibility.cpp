// Phase19 host accessibility and source-isolation regression, synthetic only.
#include "ui.hpp"
#include "rosalina_input.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace phase19;
static void gotoHome(State& s,unsigned row){
    assert(s.screen==Screen::Home);
    while(s.homeRow<row)press(s,Key::Down);
    while(s.homeRow>row)press(s,Key::Up);
    press(s,Key::A);
}
int main(){
    static Demo snapshot{};populateMock(snapshot);
    State s(snapshot);
    // Every main screen can be opened and backed out with A/B and D-pad.
    for(unsigned i=0;i<7;i++){
        gotoHome(s,i);
        assert(s.screen!=Screen::Home);
        press(s,Key::B);
        assert(s.screen==Screen::Home);
    }
    gotoHome(s,1);assert(s.screen==Screen::PC);
    // Neither navigation nor selection needs L: R, X and D-pad are enough.
    press(s,Key::R);assert(s.box==1);
    press(s,Key::X);assert(s.box==0);
    press(s,Key::Right);assert(s.box==1);
    press(s,Key::Left);assert(s.box==0);
    press(s,Key::A);assert(s.screen==Screen::Pokemon);
    press(s,Key::A);assert(s.screen==Screen::Notice);
    press(s,Key::B);press(s,Key::B);press(s,Key::B);
    gotoHome(s,0);press(s,Key::A);press(s,Key::A);
    assert(s.screen==Screen::Edit);
    press(s,Key::Right);assert(s.group==Group::EV);
    press(s,Key::X);assert(s.group==Group::IV);
    press(s,Key::R);assert(s.group==Group::EV);
    press(s,Key::Left);assert(s.group==Group::IV);
    press(s,Key::B);press(s,Key::B);press(s,Key::B);
    gotoHome(s,2);
    // Fill a 19-stack fictional bag to exercise multi-page R/X/Left/Right.
    for(unsigned i=5;i<19;i++)snapshot.bag[i]={"Item",300u+i,1};
    snapshot.bagCount=19;
    assert(count(s)==19);
    press(s,Key::R);assert(s.selected==6);
    press(s,Key::R);assert(s.selected==12);
    press(s,Key::R);assert(s.selected==18);
    press(s,Key::R);assert(s.selected==18);
    press(s,Key::X);assert(s.selected==12);
    press(s,Key::Left);assert(s.selected==6);
    press(s,Key::Right);assert(s.selected==12);
    press(s,Key::B);
    // Explicit source swap must reset all navigation and proposals.
    gotoHome(s,0);press(s,Key::A);press(s,Key::A);press(s,Key::A);
    assert(s.screen==Screen::Proposal);
    switchView(s,false);
    assert(!s.demoPreviewEnabled && s.screen==Screen::Home);
    assert(s.homeRow==0 && s.selected==0 && s.box==0 && s.mon==0);
    // Even if a later bad adapter accidentally tags imported data as synthetic,
    // the independently latched session guard still rejects every edit.
    snapshot.synthetic=true;
    gotoHome(s,0);press(s,Key::A);press(s,Key::A);
    assert(s.screen==Screen::Notice);
    assert(std::strstr(s.notice,"READ ONLY"));
    press(s,Key::B);press(s,Key::B);press(s,Key::B);
    gotoHome(s,2);press(s,Key::A);
    assert(s.screen==Screen::Notice);press(s,Key::B);press(s,Key::B);
    gotoHome(s,3);press(s,Key::A);
    assert(s.screen==Screen::Notice);press(s,Key::B);press(s,Key::B);
    assert(s.screen==Screen::Home);
    // Switching into mock mode is explicit and restores demonstrations only.
    populateMock(snapshot);
    switchView(s,true);
    gotoHome(s,0);press(s,Key::A);press(s,Key::A);
    assert(s.screen==Screen::Edit);
    // START+SELECT Rosalina exit gate: do not exit on either key order.
    StartExitGate a{};
    assert(!a.tick(true,false,false,false));
    assert(!a.tick(false,false,true,true));
    assert(!a.tick(false,true,false,true));
    StartExitGate b{};
    assert(!b.tick(false,false,true,true));
    assert(!b.tick(true,false,false,true));
    assert(!b.tick(false,true,false,false));
    StartExitGate stand{};
    assert(!stand.tick(true,false,false,false));
    assert(stand.tick(false,true,false,false));
    // Maximum supported copied inventory: 450 stacks; no broken L needed.
    static Demo full{};
    populateMock(full);
    full.bagCount=450;
    for(unsigned i=5;i<450;i++)full.bag[i]={"Item",1000u+i,1};
    State huge(full);
    gotoHome(huge,2);
    for(unsigned i=0;i<100;i++)press(huge,Key::R);
    assert(huge.selected==444 && firstVisible(huge.selected)==444);
    for(unsigned i=0;i<10;i++)press(huge,Key::Down);
    assert(huge.selected==449 && lastVisible(huge.selected,450)==450);
    press(huge,Key::X);
    assert(huge.selected==438); // previous page's first visible stack
    press(huge,Key::Right);assert(huge.selected==444);
    press(huge,Key::Left);assert(huge.selected==438);
    press(huge,Key::A);assert(huge.screen==Screen::Proposal);
    press(huge,Key::B);assert(huge.screen==Screen::Bag);
    press(huge,Key::B);assert(huge.screen==Screen::Home);
    std::cout<<"PASS: L-free navigation, 19-stack pagination, isolated copy gate, Rosalina input\n";
}
