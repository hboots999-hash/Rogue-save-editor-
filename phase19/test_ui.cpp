// Phase 19: hardware-button-equivalent host navigation tests, GPL-3.0-or-later.
#include "ui.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace phase19;
static void pressKey(State& s,Key k){press(s,k);}
static void home(State& s,unsigned row){
    assert(s.screen==Screen::Home);
    while(s.homeRow<row)pressKey(s,Key::Down);
    while(s.homeRow>row)pressKey(s,Key::Up);
    pressKey(s,Key::A);
}
static void acknowledgeBlocked(State& s, Screen from){
    assert(s.screen==Screen::Proposal || s.screen==Screen::ConfirmOne);
    if(s.screen==Screen::Proposal) pressKey(s,Key::A);
    assert(s.screen==Screen::ConfirmOne);
    pressKey(s,Key::A);assert(s.screen==Screen::ConfirmTwo);
    pressKey(s,Key::A);assert(s.screen==Screen::Notice);
    assert(std::strstr(s.notice,"BLOCKED")!=nullptr);
    pressKey(s,Key::B);assert(s.screen==from);
}
static void unchanged(const State& s,const Demo& original) {
    assert(s.demo.money==original.money);
    assert(s.demo.partyCount==original.partyCount);
    assert(s.demo.pcCounts==original.pcCounts);
    for(unsigned i=0;i<s.demo.party.size();i++) {
        const auto& a=s.demo.party[i];const auto& b=original.party[i];
        assert(a.species==b.species && a.iv==b.iv && a.ev==b.ev &&
               a.moves==b.moves && a.pp==b.pp && a.ability==b.ability && a.nature==b.nature);
    }
    for(unsigned box=0;box<2;box++)for(unsigned i=0;i<s.demo.pc[box].size();i++){
        const auto& a=s.demo.pc[box][i];const auto& b=original.pc[box][i];
        assert(a.species==b.species && a.iv==b.iv && a.ev==b.ev &&
               a.moves==b.moves && a.pp==b.pp);
    }
    for(unsigned i=0;i<s.demo.bag.size();i++)
        assert(s.demo.bag[i].quantity==original.bag[i].quantity);
}
int main(){
    // Real 3DS installed CIA faulted with a write near its main stack.
    // The previous State stored the >32KiB snapshot directly in a local.
    static_assert(sizeof(Demo)>32768,"Fixture must remain large enough to exercise storage guard");
    static_assert(sizeof(State)<256,"UI model must not embed the large fixture");
    State s;
    State sameFixture;
    assert(&s.demo==&sameFixture.demo); // process-lifetime mock, not a stack copy
    assert(s.demo.synthetic);
    const Demo original=s.demo;
    assert(s.demo.synthetic && count(s)==6);
    home(s,0);
    assert(s.screen==Screen::Party && count(s)==3);
    pressKey(s,Key::Up);assert(s.selected==0);
    pressKey(s,Key::Down);pressKey(s,Key::A);
    assert(s.screen==Screen::Pokemon && s.mon==1 && !s.fromPC);
    pressKey(s,Key::A);assert(s.screen==Screen::Edit);
    // IV: default unchanged values cannot proceed.
    pressKey(s,Key::A);assert(s.screen==Screen::Proposal);
    pressKey(s,Key::A);assert(s.screen==Screen::Notice);
    assert(std::strstr(s.notice,"INVALID")!=nullptr);
    pressKey(s,Key::B);assert(s.screen==Screen::Proposal);
    pressKey(s,Key::Up);assert(s.proposal==31); // IV was 31, clamp
    pressKey(s,Key::Down);assert(s.proposal==30);
    pressKey(s,Key::Right);assert(s.proposal==31);
    pressKey(s,Key::Down);
    acknowledgeBlocked(s,Screen::Edit);
    // EV: full budget, candidate must respect total 510.
    pressKey(s,Key::R);assert(s.group==Group::EV);
    pressKey(s,Key::Down);pressKey(s,Key::A);
    assert(s.screen==Screen::Proposal);
    for(unsigned i=0;i<100;i++)pressKey(s,Key::Right);
    assert(s.proposal<=maximum(s));
    assert(s.proposal==252);
    pressKey(s,Key::Down);
    acknowledgeBlocked(s,Screen::Edit);
    // Move ID changes have no ROM-specific allowlist: blocked.
    pressKey(s,Key::R);assert(s.group==Group::Moves);
    pressKey(s,Key::A);pressKey(s,Key::Up);pressKey(s,Key::A);
    assert(s.screen==Screen::Notice);
    assert(std::strstr(s.notice,"UNVERIFIED")!=nullptr);
    pressKey(s,Key::B);pressKey(s,Key::B);
    assert(s.screen==Screen::Edit);
    // PP can be previewed safely but never written; includes cancel.
    pressKey(s,Key::R);assert(s.group==Group::PP);
    pressKey(s,Key::A);assert(s.screen==Screen::Proposal);
    pressKey(s,Key::Up);pressKey(s,Key::A);
    assert(s.screen==Screen::ConfirmOne);
    pressKey(s,Key::B);assert(s.screen==Screen::Proposal);
    pressKey(s,Key::A);
    acknowledgeBlocked(s,Screen::Edit);
    // Ability is a 0/1 slot only and nature is read-only.
    pressKey(s,Key::R);assert(s.group==Group::Ability);
    pressKey(s,Key::A);assert(s.screen==Screen::Notice);
    pressKey(s,Key::B);
    pressKey(s,Key::R);assert(s.group==Group::Nature);
    pressKey(s,Key::A);assert(s.screen==Screen::Notice);
    pressKey(s,Key::B);
    pressKey(s,Key::R);assert(s.group==Group::IV);
    pressKey(s,Key::L);assert(s.group==Group::Nature);
    pressKey(s,Key::B);pressKey(s,Key::B);pressKey(s,Key::B);
    assert(s.screen==Screen::Home);
    home(s,1);
    assert(s.screen==Screen::PC && s.box==0 && count(s)==3);
    pressKey(s,Key::L);assert(s.box==1 && s.selected==0);
    pressKey(s,Key::R);assert(s.box==0 && s.selected==0);
    pressKey(s,Key::R);pressKey(s,Key::Down);pressKey(s,Key::A);
    assert(s.screen==Screen::Pokemon && s.fromPC && s.mon==1);
    pressKey(s,Key::A);assert(s.screen==Screen::Notice);
    assert(std::strstr(s.notice,"UNSUPPORTED")!=nullptr);
    pressKey(s,Key::B);assert(s.screen==Screen::Pokemon);
    pressKey(s,Key::B);assert(s.screen==Screen::PC && s.selected==1);
    pressKey(s,Key::B);assert(s.screen==Screen::Home);
    home(s,2);
    assert(s.screen==Screen::Bag);
    for(unsigned i=0;i<20;i++)pressKey(s,Key::Down);
    assert(s.selected==4);
    pressKey(s,Key::A);assert(s.screen==Screen::Proposal);
    for(unsigned i=0;i<500;i++)pressKey(s,Key::Left);
    assert(s.proposal==1); // Existing stacks may not become zero
    pressKey(s,Key::B);assert(s.screen==Screen::Bag);
    pressKey(s,Key::A);pressKey(s,Key::Up);
    acknowledgeBlocked(s,Screen::Bag);
    pressKey(s,Key::B);assert(s.screen==Screen::Home);
    home(s,3);
    pressKey(s,Key::A);assert(s.screen==Screen::Proposal);
    for(unsigned i=0;i<100001;i++)pressKey(s,Key::Right);
    assert(s.proposal==999999);
    pressKey(s,Key::X);assert(s.proposal==original.money);
    pressKey(s,Key::Up);acknowledgeBlocked(s,Screen::Money);
    pressKey(s,Key::B);assert(s.screen==Screen::Home);
    home(s,4);
    assert(s.screen==Screen::Backups);
    pressKey(s,Key::A);assert(s.screen==Screen::ConfirmOne);
    pressKey(s,Key::B);assert(s.screen==Screen::Backups);
    pressKey(s,Key::A);pressKey(s,Key::A);pressKey(s,Key::A);
    assert(s.screen==Screen::Notice);
    pressKey(s,Key::B);
    for(unsigned i=1;i<3;i++){
        pressKey(s,Key::Down);pressKey(s,Key::A);
        assert(s.screen==Screen::Notice);
        pressKey(s,Key::B);
    }
    pressKey(s,Key::B);assert(s.screen==Screen::Home);
    home(s,5);assert(s.screen==Screen::Recovery);
    for(unsigned i=0;i<5;i++){
        assert(s.selected==i);
        pressKey(s,Key::A);assert(s.screen==Screen::Notice);
        assert(std::strlen(s.notice)>25);
        pressKey(s,Key::B);assert(s.screen==Screen::Recovery);
        pressKey(s,Key::Down);
    }
    pressKey(s,Key::B);assert(s.screen==Screen::Home);
    unchanged(s,original);
    // Invalid provider status: no fake values or valid selection.
    Demo unavailable{};
    State fail(unavailable);
    home(fail,0);assert(count(fail)==0);
    pressKey(fail,Key::A);assert(fail.screen==Screen::Notice);
    pressKey(fail,Key::B);pressKey(fail,Key::B);
    home(fail,1);assert(count(fail)==0);
    pressKey(fail,Key::R);assert(fail.box==0);
    pressKey(fail,Key::B);
    home(fail,3);pressKey(fail,Key::A);
    assert(fail.screen==Screen::Notice); // no real-data mutations
    pressKey(s,Key::Start);assert(s.exitRequested);
    const Screen prev=s.screen;pressKey(s,Key::A);assert(s.screen==prev);
    std::cout<<"PASS Phase19 physical-button state matrix, adapter gating, no save writes\n";
}
