// Regression of corrupted selections after repeated imports and stale UI flows.
// Deterministic synthetic only; no output paths, titles or filesystem access.
#include "ui.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using namespace phase19;
int main() {
    static Demo d{};populateMock(d);
    State s(d);
    // Imported-copy UI always blocks proposals, even if a stale party slot
    // points beyond the actual data or spoofed data become "synthetic".
    s.screen=Screen::Pokemon;
    s.mon=std::numeric_limits<unsigned>::max();
    assert(!selectedPokemonValid(s));
    assert(pokemon(s).species==0);
    press(s,Key::A);
    assert(s.screen==Screen::Notice);
    press(s,Key::B);assert(s.screen==Screen::Home);
    s.screen=Screen::Edit;s.group=Group::IV;s.mon=200u;s.field=0;
    s.proposalFrom=Screen::Edit;
    assert(!canPreview(s));
    assert(original(s)==0u && maximum(s)==0u);
    assert(!validProposal(s));
    s.mon=0u;
    s.field=99u;
    assert(!canPreview(s) && original(s)==0u && maximum(s)==0u);
    s.field=0;
    s.group=static_cast<Group>(255u);
    assert(editFieldCount(s.group)==0u && !canPreview(s));
    s.group=Group::PP;s.field=4u;
    assert(!canPreview(s) && original(s)==0u);
    // Party source count can become corrupt; rendering count remains clamped.
    d.partyCount=999u;
    s.screen=Screen::Party;assert(count(s)==6u);
    d.partyCount=2u;
    // PC box index and slot count are separately bounded.
    s.screen=Screen::PC;s.box=14u;s.fromPC=true;s.mon=0;
    assert(count(s)==0u && !selectedPokemonValid(s));
    assert(pokemon(s).species==0u);
    d.pcBoxes=100u;s.box=99u;
    assert(count(s)==0u && !selectedPokemonValid(s));
    s.box=0;s.mon=1000u;d.pcCounts[0]=1000u;
    assert(count(s)==30u && !selectedPokemonValid(s));
    d.pcCounts[0]=3u;
    // Bogus bag slot index must not leak through preview old-value function.
    s.screen=Screen::Proposal;s.proposalFrom=Screen::Bag;
    s.selected=std::numeric_limits<unsigned>::max();
    assert(original(s)==0u && !canPreview(s) && !validProposal(s));
    d.bagCount=99999u;
    s.screen=Screen::Bag;
    assert(count(s)==450u);
    d.bagCount=5u;
    // Two independent imported source locks survive unusual screen changes.
    d.synthetic=false;
    switchView(s,false);
    s.screen=Screen::Money;s.proposalFrom=Screen::Money;
    s.proposal=0;
    assert(!canPreview(s) && !validProposal(s));
    d.synthetic=true; // simulated adapter flag bug
    assert(!canPreview(s) && !validProposal(s));
    s.screen=Screen::Pokemon;s.fromPC=false;s.mon=0;
    press(s,Key::A);
    assert(s.screen==Screen::Notice);
    // Repeat source reset stress with varied transient state.
    for(unsigned i=0;i<2000u;i++) {
        d.synthetic=(i%2u)==0u;
        switchView(s,false);
        s.selected=999u;s.mon=999u;s.field=999u;
        s.screen=Screen::Edit;s.proposalFrom=Screen::Edit;
        assert(!canPreview(s) && original(s)==0u && maximum(s)==0u);
        s.screen=Screen::Home;
        switchView(s,d.synthetic);
        assert(s.homeRow==0u && s.selected==0u && s.mon==0u &&
               s.field==0u && s.box==0u && s.proposal==0u);
    }
    std::cout<<"PASS Phase19 stale selections, extreme indices, malformed counts, repeated source resets\n";
}
