// SPDX-License-Identifier: GPL-3.0-or-later
// Pure geometry for the optional Phase20 touchscreen dashboard.
// Maps TOUCH to EXISTING read-only/offline-key workflows, never to writes.
#pragma once
#include <cstdint>
namespace phase20 { namespace gui {
enum class Page : uint8_t { Home, Party, Pokemon, Other };
enum class Action : uint8_t { None, HomeItem, PartyItem, Back, PartyMoves, PokemonEdit, PokemonMoves };
struct Hit { Action action=Action::None; unsigned index=0; };
inline bool within(unsigned x,unsigned y,unsigned left,unsigned top,unsigned width,unsigned height) {
    return x>=left && x<left+width && y>=top && y<top+height;
}
inline Hit hitTest(Page page,unsigned x,unsigned y,unsigned partyCount,bool fromPC) {
    if(x>=320u || y>=240u)return {};
    if(page==Page::Home){
        for(unsigned row=0;row<3;row++)for(unsigned col=0;col<2;col++){
            const unsigned left=col?166u:8u, top=42u+row*54u;
            if(within(x,y,left,top,146u,46u))
                return {Action::HomeItem,row*2u+col};
        }
        if(within(x,y,8,208,304,28))return {Action::HomeItem,6};
    }else if(page==Page::Party){
        for(unsigned row=0;row<3;row++)for(unsigned col=0;col<2;col++){
            const unsigned left=col?166u:8u,top=48u+row*54u,idx=row*2u+col;
            if(idx<partyCount && idx<6u && within(x,y,left,top,146u,46u))
                return {Action::PartyItem,idx};
        }
        if(within(x,y,8,210,102,26))return {Action::Back,0};
        if(within(x,y,116,210,196,26))return {Action::PartyMoves,0};
    }else if(page==Page::Pokemon){
        if(within(x,y,8,204,94,32))return {Action::Back,0};
        if(!fromPC && within(x,y,110,204,94,32))return {Action::PokemonEdit,0};
        if(!fromPC && within(x,y,212,204,100,32))return {Action::PokemonMoves,0};
    }
    return {};
}
}} // namespace phase20::gui
