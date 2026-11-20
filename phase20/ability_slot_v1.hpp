// SPDX-License-Identifier: GPL-3.0-or-later
// Phase20 Pokémon ability-slot selector. This is NOT a save writer.
// Existing Phase18 has a separately gated encrypted ability-slot writer;
// no exact-ROM ability ID mapping is available here to authorise it.
#pragma once
#include "ui.hpp"
#include "phase20/touch_layout_v1.hpp"
#include <array>
#include <cstdint>
namespace phase20 {
enum class AbilityPage : uint8_t { Idle, Select, Review, Locked };
enum class AbilityAction : uint8_t { Up, Down, A, B };
struct AbilityFlow {
    AbilityPage page=AbilityPage::Idle;
    unsigned party=0, species=0, originalSlot=0, selectedSlot=0, counter=0;
    std::array<uint8_t,32> digest{};
    bool synthetic=false;
};
inline const char* slotLabel(unsigned slot) {
    switch(slot) {
        case 0:return "ABILITY SLOT 1";
        case 1:return "ABILITY SLOT 2";
        case 2:return "THIRD / HIDDEN SLOT";
        default:return "INVALID SLOT";
    }
}
inline bool beginAbility(AbilityFlow& flow,const phase19::State& s){
    flow=AbilityFlow{};
    if(s.screen!=phase19::Screen::Pokemon || s.fromPC ||
       !phase19::selectedPokemonValid(s) || s.mon>=s.demo.partyCount)
        return false;
    const auto& p=s.demo.party[s.mon];
    if(p.egg || !p.species || p.ability>2 || s.demo.partyStatus!=phase19::Availability::Ready)
        return false;
    flow.page=AbilityPage::Select;
    flow.party=s.mon;flow.species=p.species;
    flow.originalSlot=flow.selectedSlot=p.ability;
    flow.digest=s.demo.sourceSha256;
    flow.counter=s.demo.activeCounter;
    flow.synthetic=s.demo.synthetic;
    return true;
}
inline bool abilitySourceValid(const AbilityFlow& flow,const phase19::State& s){
    return flow.page!=AbilityPage::Idle &&
           s.screen==phase19::Screen::Pokemon && !s.fromPC &&
           phase19::selectedPokemonValid(s) && s.mon==flow.party &&
           s.demo.partyStatus==phase19::Availability::Ready &&
           s.mon<s.demo.partyCount && !s.demo.party[s.mon].egg &&
           s.demo.party[s.mon].species==flow.species &&
           s.demo.party[s.mon].ability==flow.originalSlot &&
           s.demo.activeCounter==flow.counter &&
           s.demo.sourceSha256==flow.digest &&
           s.demo.synthetic==flow.synthetic &&
           flow.originalSlot<=2 && flow.selectedSlot<=2;
}
inline bool abilityInput(AbilityFlow& flow,const phase19::State& s,
                        AbilityAction key){
    if(flow.page==AbilityPage::Idle)return false;
    if(!abilitySourceValid(flow,s)){flow=AbilityFlow{};return true;}
    switch(flow.page){
        case AbilityPage::Select:
            if(key==AbilityAction::B){flow=AbilityFlow{};break;}
            if(key==AbilityAction::Up)flow.selectedSlot=(flow.selectedSlot+2u)%3u;
            if(key==AbilityAction::Down)flow.selectedSlot=(flow.selectedSlot+1u)%3u;
            if(key==AbilityAction::A && flow.selectedSlot!=flow.originalSlot)
                flow.page=AbilityPage::Review;
            break;
        case AbilityPage::Review:
            if(key==AbilityAction::B)flow.page=AbilityPage::Select;
            // Critical: no export or write until exact-ROM mapping and
            // owner-game runtime validation; even synthetic demos fail closed.
            if(key==AbilityAction::A)flow.page=AbilityPage::Locked;
            break;
        case AbilityPage::Locked:
            if(key==AbilityAction::A || key==AbilityAction::B)
                flow.page=AbilityPage::Select;
            break;
        case AbilityPage::Idle:break;
    }
    return true;
}
constexpr bool abilityProductionSaveWriteApproved=false;
constexpr bool arbitraryAbilityIdStorageVerified=false;
static_assert(!abilityProductionSaveWriteApproved && !arbitraryAbilityIdStorageVerified,
              "Real-game ability ID or installed-save edits are not verified");
namespace gui {
enum class AbilityTapKind : uint8_t { None, Slot, Back, Review };
struct AbilityTap { AbilityTapKind kind=AbilityTapKind::None; unsigned index=0; };
inline AbilityTap abilityTouchHit(phase20::AbilityPage page,unsigned x,unsigned y){
    if(x>=320u || y>=240u)return {};
    if(page==phase20::AbilityPage::Select) {
        for(unsigned i=0;i<3;i++)
            if(within(x,y,8,55+i*46u,304,41))
                return {AbilityTapKind::Slot,i};
        if(within(x,y,8,208,94,28))return {AbilityTapKind::Back,0};
        if(within(x,y,110,208,202,28))return {AbilityTapKind::Review,0};
    }else{
        if(within(x,y,8,208,94,28))return {AbilityTapKind::Back,0};
    }
    return {};
}
} // namespace gui
} // namespace phase20
