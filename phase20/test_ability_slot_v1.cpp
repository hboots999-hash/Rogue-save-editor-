// SPDX-License-Identifier: GPL-3.0-or-later
#include "phase20/ability_slot_v1.hpp"
#include <cassert>
#include <iostream>
using namespace phase20;
int main(){
    phase19::Demo snapshot{};
    snapshot.synthetic=true;
    snapshot.partyStatus=phase19::Availability::Ready;
    snapshot.partyCount=2;
    snapshot.party[0].species=25;snapshot.party[0].ability=0;
    snapshot.party[1].species=149;snapshot.party[1].ability=2;
    phase19::State s(snapshot);s.screen=phase19::Screen::Pokemon;s.mon=0;
    AbilityFlow flow{};
    assert(beginAbility(flow,s));
    assert(flow.originalSlot==0 && flow.selectedSlot==0);
    assert(abilitySourceValid(flow,s));
    assert(abilityInput(flow,s,AbilityAction::A) && flow.page==AbilityPage::Select);
    assert(abilityInput(flow,s,AbilityAction::Down));
    assert(flow.selectedSlot==1);
    assert(abilityInput(flow,s,AbilityAction::A) && flow.page==AbilityPage::Review);
    assert(abilityInput(flow,s,AbilityAction::A) && flow.page==AbilityPage::Locked);
    assert(!abilityProductionSaveWriteApproved && !arbitraryAbilityIdStorageVerified);
    assert(abilityInput(flow,s,AbilityAction::B) && flow.page==AbilityPage::Select);
    assert(abilityInput(flow,s,AbilityAction::Up));
    assert(flow.selectedSlot==0);
    assert(abilityInput(flow,s,AbilityAction::B) && flow.page==AbilityPage::Idle);
    // Invalid data cannot reach preview or indirectly switch selected Pokémon.
    s.mon=1;snapshot.party[1].egg=true;
    assert(!beginAbility(flow,s));
    snapshot.party[1].egg=false;
    assert(beginAbility(flow,s));
    assert(flow.originalSlot==2);
    assert(abilityInput(flow,s,AbilityAction::Down) && flow.selectedSlot==0);
    snapshot.sourceSha256[0]++;
    assert(abilityInput(flow,s,AbilityAction::A) && flow.page==AbilityPage::Idle);
    assert(beginAbility(flow,s));
    snapshot.party[1].ability=1;
    assert(abilityInput(flow,s,AbilityAction::Down) && flow.page==AbilityPage::Idle);
    snapshot.party[1].ability=2;
    assert(beginAbility(flow,s));
    s.mon=0;
    assert(abilityInput(flow,s,AbilityAction::B) && flow.page==AbilityPage::Idle);
    s.fromPC=true;assert(!beginAbility(flow,s));
    s.fromPC=false;s.screen=phase19::Screen::Home;assert(!beginAbility(flow,s));
    assert(slotLabel(3)!=nullptr);
    using phase20::gui::AbilityTapKind;
    using phase20::gui::abilityTouchHit;
    for(unsigned i=0;i<3;i++){
        const auto tap=abilityTouchHit(AbilityPage::Select,25,66+i*46);
        assert(tap.kind==AbilityTapKind::Slot && tap.index==i);
    }
    assert(abilityTouchHit(AbilityPage::Select,24,222).kind==AbilityTapKind::Back);
    assert(abilityTouchHit(AbilityPage::Select,120,222).kind==AbilityTapKind::Review);
    assert(abilityTouchHit(AbilityPage::Review,120,222).kind==AbilityTapKind::None);
    assert(abilityTouchHit(AbilityPage::Locked,120,222).kind==AbilityTapKind::None);
    assert(abilityTouchHit(AbilityPage::Review,24,222).kind==AbilityTapKind::Back);
    assert(abilityTouchHit(AbilityPage::Select,320,66).kind==AbilityTapKind::None);
    assert(abilityTouchHit(AbilityPage::Select,20,240).kind==AbilityTapKind::None);
    std::cout<<"PASS ability selection bounded 0..2, stale-source invalidation, egg/PC locked, no writing\n";
}
