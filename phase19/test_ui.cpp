// Phase 19 deterministic host navigation and safety contract.
#include "ui.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace phase19;
static void key(State& s, Key k) { press(s,k); }
static void go(State& s, unsigned row) {
    while(s.homeRow<row) key(s,Key::Down);
    key(s,Key::A);
}
static void finishBlocked(State& s, Screen origin) {
    key(s,Key::A);
    assert(s.screen==Screen::ConfirmOne);
    key(s,Key::A);
    assert(s.screen==Screen::ConfirmTwo);
    key(s,Key::A);
    assert(s.screen==Screen::Notice);
    assert(std::strstr(s.notice,"BLOCKED")!=nullptr);
    key(s,Key::B);
    assert(s.screen==origin);
}
int main() {
    State s;
    const Demo immutable=s.demo;
    assert(count(s)==6);
    go(s,0);
    assert(s.screen==Screen::Party && count(s)==3);
    key(s,Key::Down);
    key(s,Key::A);
    assert(s.screen==Screen::Pokemon && s.mon==1);
    key(s,Key::A);
    assert(s.screen==Screen::Edit);
    for(unsigned group=0;group<5;group++) {
        assert(static_cast<unsigned>(s.group)==group);
        key(s,Key::A);
        assert(s.screen==Screen::Proposal);
        for(int i=0;i<900;i++) key(s,Key::Right);
        assert(s.proposal<=maximum(s));
        for(int i=0;i<900;i++) key(s,Key::Left);
        assert(s.proposal>=minimum(s));
        key(s,Key::X);
        assert(s.proposal==original(s));
        key(s,Key::B);
        assert(s.screen==Screen::Edit);
        if(group<4) key(s,Key::R);
    }
    // EV budget must never exceed 510.
    key(s,Key::L); key(s,Key::L); key(s,Key::L);
    assert(s.group==Group::EV);
    key(s,Key::A);
    adjust(s,2000);
    assert(s.proposal<=maximum(s));
    finishBlocked(s,Screen::Edit);
    key(s,Key::B);key(s,Key::B);key(s,Key::B);
    assert(s.screen==Screen::Home);
    go(s,1);
    assert(s.screen==Screen::PC);
    key(s,Key::R); assert(s.box==1 && s.selected==0);
    key(s,Key::Down); key(s,Key::A);
    assert(s.fromPC && s.mon==1 && s.screen==Screen::Pokemon);
    key(s,Key::A);assert(s.screen==Screen::Edit);
    key(s,Key::R);key(s,Key::R);
    assert(s.group==Group::Moves);
    key(s,Key::Down);key(s,Key::A);
    assert(s.screen==Screen::Proposal && s.proposal==pokemon(s).moves[1]);
    key(s,Key::Up);finishBlocked(s,Screen::Edit);
    key(s,Key::B);key(s,Key::B);key(s,Key::B);
    assert(s.screen==Screen::Home);
    go(s,2);
    assert(s.screen==Screen::Bag);
    for(int i=0;i<100;i++) key(s,Key::Down);
    assert(s.selected==4);
    key(s,Key::A);
    assert(s.proposal==12);
    adjust(s,-100000);
    assert(s.proposal==0);
    finishBlocked(s,Screen::Bag);
    key(s,Key::B);
    go(s,3);
    assert(s.screen==Screen::Money);
    key(s,Key::A);adjust(s,1000000);
    assert(s.proposal==999999);
    finishBlocked(s,Screen::Money);
    key(s,Key::B);
    go(s,4);
    assert(s.screen==Screen::Backups);
    key(s,Key::A); assert(s.screen==Screen::ConfirmOne);
    key(s,Key::B); assert(s.screen==Screen::Backups);
    key(s,Key::A); assert(s.screen==Screen::ConfirmOne);
    key(s,Key::A);key(s,Key::A);
    assert(s.screen==Screen::Notice);
    key(s,Key::B); assert(s.screen==Screen::Backups);
    key(s,Key::Down); key(s,Key::A); assert(s.screen==Screen::Notice);
    key(s,Key::A);key(s,Key::Down);key(s,Key::A);
    assert(s.screen==Screen::Notice);
    key(s,Key::B);key(s,Key::B);
    assert(s.screen==Screen::Home);
    go(s,5); assert(s.screen==Screen::Recovery);
    for(unsigned i=0;i<5;i++) {
        assert(s.selected==i);
        key(s,Key::A);
        assert(s.screen==Screen::Notice);
        assert(std::strlen(s.notice)>18);
        key(s,Key::B);
        assert(s.screen==Screen::Recovery);
        if(i<4) key(s,Key::Down);
    }
    key(s,Key::B); assert(s.screen==Screen::Home);
    assert(s.demo.money==immutable.money);
    for(unsigned i=0;i<3;i++) {
        const auto& a=s.demo.party[i]; const auto& b=immutable.party[i];
        assert(a.name==b.name && a.iv==b.iv && a.ev==b.ev && a.moves==b.moves);
        assert(a.ability==b.ability && a.nature==b.nature);
    }
    for(unsigned i=0;i<5;i++) assert(s.demo.bag[i].quantity==immutable.bag[i].quantity);
    for(unsigned box=0;box<2;box++) for(unsigned i=0;i<3;i++) {
        const auto& a=s.demo.pc[box][i]; const auto& b=immutable.pc[box][i];
        assert(a.species==b.species && a.level==b.level &&
               a.iv==b.iv && a.ev==b.ev && a.moves==b.moves &&
               a.ability==b.ability && a.nature==b.nature);
    }

    key(s,Key::Start);
    assert(s.exitRequested);
    const auto scr=s.screen; key(s,Key::A);assert(s.screen==scr);
    std::cout<<"PASS Phase19: party/PC/edit/EV constraints/bag/money/backups/confirm/cancel/read-only\n";
}
