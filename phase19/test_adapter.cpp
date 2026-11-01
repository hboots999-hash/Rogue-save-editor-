// Phase19 read-only adapter boundary tests. No game data or filesystem.
#include "ui.hpp"
#include <cassert>
#include <iostream>
using namespace phase19;
struct Fake : ReadOnlyProvider {
    Demo sample=mockSnapshot();
    Availability money=Availability::Ready;
    Availability mons=Availability::Ready;
    Availability items=Availability::Ready;
    Availability boxes=Availability::Unsupported; // Phase18 cannot parse PC
    unsigned rawParty=3,rawBag=5,rawBoxes=0;
    Availability summary(unsigned& amount,unsigned& counter) const override {
        amount=sample.money;counter=11;return money;
    }
    Availability partyCount(unsigned& count) const override {
        count=rawParty;return mons;
    }
    Availability partyAt(unsigned index,Pokemon& m) const override {
        if(index>=sample.partyCount)return Availability::Invalid;
        m=sample.party[index];return Availability::Ready;
    }
    Availability bagCount(unsigned& count) const override {
        count=rawBag;return items;
    }
    Availability itemAt(unsigned index,Item& it) const override {
        if(index>=sample.bagCount)return Availability::Invalid;
        it=sample.bag[index];return Availability::Ready;
    }
    Availability pcBoxCount(unsigned& count) const override {
        count=rawBoxes;return boxes;
    }
    Availability pcBoxSize(unsigned box,unsigned& size) const override {
        if(box>=2)return Availability::Unsupported;
        size=sample.pcCounts[box];return Availability::Ready;
    }
    Availability pcAt(unsigned box,unsigned index,Pokemon& mon) const override {
        if(box>=2||index>=sample.pcCounts[box])return Availability::Invalid;
        mon=sample.pc[box][index];return Availability::Ready;
    }
};
int main(){
    Fake f;
    Demo real=capture(f);
    assert(!real.synthetic);
    assert(real.money==42000 && real.activeCounter==11);
    assert(real.moneyStatus==Availability::Ready);
    assert(real.partyCount==3 && real.bagCount==5);
    assert(real.pcStatus==Availability::Unsupported && real.pcBoxes==0);
    State s(real);
    press(s,Key::A);assert(s.screen==Screen::Party);
    press(s,Key::A);assert(s.screen==Screen::Pokemon);
    press(s,Key::A);assert(s.screen==Screen::Edit);
    press(s,Key::A);assert(s.screen==Screen::Notice);
    assert(s.returnTo==Screen::Edit);
    press(s,Key::B);press(s,Key::B);press(s,Key::B);press(s,Key::B);
    assert(s.screen==Screen::Home);
    s.homeRow=3;press(s,Key::A);press(s,Key::A);
    assert(s.screen==Screen::Notice);
    assert(!canPreview(s));
    // Malformed metadata or corrupted data cannot produce a selectable value.
    f.rawParty=7;
    real=capture(f);
    assert(real.partyStatus==Availability::Unsupported);
    assert(real.partyCount==0);
    f.rawParty=3;
    f.sample.party[1].ev[3]=253;
    real=capture(f);
    assert(real.partyStatus==Availability::Invalid && real.partyCount==0);
    f.sample=mockSnapshot();
    f.sample.party[1].pp[0]=65;
    real=capture(f);
    assert(real.partyStatus==Availability::Invalid && real.partyCount==0);
    f.sample=mockSnapshot();
    f.sample.party[1].ability=55; // arbitrary ability ID not a slot bit
    real=capture(f);
    assert(real.partyStatus==Availability::Invalid && real.partyCount==0);
    f.sample=mockSnapshot();
    f.rawBag=451;
    real=capture(f);
    assert(real.bagStatus==Availability::Unsupported && real.bagCount==0);
    // A full copied-save bag view can enumerate hundreds of existing stacks.
    f.sample=mockSnapshot();
    f.rawBag=245;f.sample.bagCount=245;
    for(unsigned i=5;i<245;i++)f.sample.bag[i]={"Item",1000+i,1};
    real=capture(f);
    assert(real.bagStatus==Availability::Ready && real.bagCount==245);
    State inventory(real);
    inventory.homeRow=2;press(inventory,Key::A);
    assert(count(inventory)==245);
    press(inventory,Key::R);assert(inventory.selected==6);
    press(inventory,Key::L);assert(inventory.selected==0);
    press(inventory,Key::A);
    assert(inventory.screen==Screen::Notice); // actual-data view cannot propose
    f.sample=mockSnapshot();
    f.rawBag=5;f.sample.bag[0].quantity=0;
    real=capture(f);
    assert(real.bagStatus==Availability::Invalid && real.bagCount==0);
    f.sample=mockSnapshot();f.sample.money=1000000;
    real=capture(f);
    assert(real.moneyStatus==Availability::Invalid && real.money==0);
    f.sample=mockSnapshot();f.boxes=Availability::Ready;f.rawBoxes=2;
    real=capture(f);
    assert(real.pcStatus==Availability::Ready && real.pcBoxes==2);
    assert(real.pcCounts[1]==3);
    State pc(real);pc.homeRow=1;press(pc,Key::A);assert(pc.screen==Screen::PC);
    press(pc,Key::R);assert(pc.box==1);
    press(pc,Key::A);assert(pc.screen==Screen::Pokemon);
    press(pc,Key::A);assert(pc.screen==Screen::Notice); // no PC editing
    // Page across a larger validated mock PC box using D-pad down edges.
    f.sample.pcCounts[0]=12;
    for(unsigned i=3;i<12;i++)f.sample.pc[0][i]=f.sample.pc[0][0];
    real=capture(f);
    assert(real.pcStatus==Availability::Ready && real.pcCounts[0]==12);
    State paged(real);
    paged.homeRow=1;press(paged,Key::A);
    for(unsigned i=0;i<40;i++)press(paged,Key::Down);
    assert(paged.selected==11 && count(paged)==12);
    press(paged,Key::A);
    assert(paged.screen==Screen::Pokemon && paged.mon==11);
    press(paged,Key::B);
    assert(paged.selected==11);
    press(paged,Key::R);
    assert(paged.selected==0 && paged.box==1);

    f.sample.pc[1][1].nature=26;
    real=capture(f);
    assert(real.pcStatus==Availability::Invalid && real.pcBoxes==0);
    f.sample=mockSnapshot();f.rawBoxes=3;
    real=capture(f);
    assert(real.pcStatus==Availability::Unsupported && real.pcBoxes==0);
    f.rawBoxes=0;f.boxes=Availability::Unsupported;
    f.rawParty=0;f.rawBag=0;
    real=capture(f);
    assert(real.partyStatus==Availability::Empty && real.bagStatus==Availability::Empty);
    std::cout<<"PASS Phase19 adapter fail-closed read snapshots and real-view edit guard\n";
}
