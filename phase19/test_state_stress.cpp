// Phase19 deterministic button-state stress: no live assets or filesystem.
#include "ui.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace phase19;
static unsigned seed=0x41d9c18u;
static Key nextKey(){
    seed=seed*1664525u+1013904223u;
    constexpr Key keys[]{Key::Up,Key::Down,Key::Left,Key::Right,
                         Key::A,Key::B,Key::X,Key::R};
    return keys[(seed>>12)&7u];
}
static void invariant(const State& s){
    assert(static_cast<unsigned>(s.screen)<=static_cast<unsigned>(Screen::Notice));
    assert(s.homeRow<7 && s.field<6);
    assert(s.selected<=450);
    if(s.screen==Screen::PC || (s.fromPC && s.screen==Screen::Pokemon)) {
        assert(s.box<14);
    }
    if(s.screen==Screen::Pokemon){
        if(s.fromPC){assert(s.box<s.demo.pcBoxes && s.mon<s.demo.pcCounts[s.box]);}
        else assert(s.mon<s.demo.partyCount);
    }
    if(s.screen==Screen::Edit){
        assert(s.demo.synthetic && s.demoPreviewEnabled && !s.fromPC);
        assert(s.mon<s.demo.partyCount);
    }
    if(s.screen==Screen::Proposal){
        assert(s.demo.synthetic && s.demoPreviewEnabled);
        assert(s.proposal<=maximum(s));
        assert(s.proposal>=minimum(s));
    }
}
int main(){
    static Demo demo{};populateMock(demo);
    static Demo copy{};populateMock(copy);copy.synthetic=false;
    static Demo demoOriginal=demo,copyOriginal=copy;
    State mock(demo),imported(copy);
    assert(!imported.demoPreviewEnabled);
    for(unsigned i=0;i<40000;i++){
        const Key k=nextKey();
        press(mock,k);press(imported,k);
        invariant(mock);invariant(imported);
        assert(imported.screen!=Screen::Proposal && imported.screen!=Screen::Edit);
    }
    assert(std::memcmp(&demo,&demoOriginal,sizeof(Demo))==0);
    assert(std::memcmp(&copy,&copyOriginal,sizeof(Demo))==0);
    // The strongest permitted simulation of an adapter bug: corrupt its
    // metadata to look synthetic without lifting the independent UI gate.
    copy.synthetic=true;
    imported.screen=Screen::Home;imported.homeRow=0;imported.selected=0;
    press(imported,Key::A);press(imported,Key::A);press(imported,Key::A);
    assert(imported.screen==Screen::Notice && !imported.demoPreviewEnabled);
    assert(!canPreview(imported));
    std::cout<<"PASS Phase19 40k deterministic button actions, immutable sources and read-only isolation\n";
}
