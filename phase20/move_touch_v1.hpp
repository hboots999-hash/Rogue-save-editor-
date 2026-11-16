// SPDX-License-Identifier: GPL-3.0-or-later
// Native touchscreen geometry for the existing Phase19 move browser.
// Decides visual selection only; NEVER constructs or approves a save edit.
#pragma once
#include "move_browser.hpp"
#include "phase20/touch_layout_v1.hpp"
namespace phase20 { namespace gui {
enum class MoveTapAction : uint8_t {
    None, PartyRow, SlotRow, CatalogRow, KeyboardRow,
    Back, Previous, Next, Search, ApplySearch, StartDraft
};
struct MoveTap { MoveTapAction action=MoveTapAction::None; unsigned index=0; };
inline MoveTap moveTouchHit(phase19::MovePage page,unsigned x,unsigned y,
                            unsigned partyCount,unsigned matchCount,
                            unsigned selectedOrdinal,bool fromPartyPicker) {
    if(x>=320 || y>=240)return {};
    using phase19::MovePage;
    if(page==MovePage::Party){
        for(unsigned i=0;i<6 && i<partyCount;i++){
            const unsigned left=(i%2)?166:8,top=48+(i/2)*54;
            if(within(x,y,left,top,146,46))return {MoveTapAction::PartyRow,i};
        }
        if(within(x,y,8,208,100,28))return {MoveTapAction::Back,0};
    }else if(page==MovePage::Slots){
        for(unsigned i=0;i<4;i++)
            if(within(x,y,8,48+i*37,304,33))
                return {MoveTapAction::SlotRow,i};
        if(within(x,y,8,208,100,28))return {MoveTapAction::Back,0};
    }else if(page==MovePage::Catalog){
        const unsigned first=(selectedOrdinal/phase19::BrowserPageRows)*
            phase19::BrowserPageRows;
        for(unsigned i=0;i<phase19::BrowserPageRows;i++){
            const unsigned ordinal=first+i;
            if(ordinal<matchCount &&
               within(x,y,8,40+i*26,304,23))
                return {MoveTapAction::CatalogRow,ordinal};
        }
        if(within(x,y,8,208,68,28))return {MoveTapAction::Back,0};
        if(within(x,y,81,208,52,28))return {MoveTapAction::Previous,0};
        if(within(x,y,139,208,54,28))return {MoveTapAction::Next,0};
        if(within(x,y,199,208,113,28))return {MoveTapAction::Search,0};
    }else if(page==MovePage::Keyboard){
        for(unsigned i=0;i<30;i++){
            const unsigned left=8+(i%6)*50,top=42+(i/6)*31;
            if(within(x,y,left,top,46,27))
                return {MoveTapAction::KeyboardRow,i};
        }
        if(within(x,y,8,208,102,28))return {MoveTapAction::Back,0};
        if(within(x,y,116,208,196,28))return {MoveTapAction::ApplySearch,0};
    }else if(page==MovePage::Review){
        if(within(x,y,8,208,102,28))return {MoveTapAction::Back,0};
        if(within(x,y,116,208,196,28))return {MoveTapAction::StartDraft,0};
    }else if(page==MovePage::Notice || page==MovePage::ConfirmOne ||
             page==MovePage::ConfirmTwo){
        // Confirmation A/X remains EXCLUSIVELY on hardware, with physical
        // release checks. Touch cannot skip or approve either stage.
        if(within(x,y,8,208,102,28))return {MoveTapAction::Back,0};
    }
    (void)fromPartyPicker;
    return {};
}
}} // namespace phase20::gui
