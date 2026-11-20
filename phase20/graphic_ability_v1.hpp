// SPDX-License-Identifier: GPL-3.0-or-later
// 3DS bottom-LCD ability slot selector. No ROM ability-ID claims or installed saves.
#pragma once
#include "phase20/graphic_panel_v1.hpp"
#include "phase20/ability_slot_v1.hpp"
namespace phase20 { namespace gui {
inline bool drawAbility(const phase19::State& s,const phase20::AbilityFlow& f){
    u16 width=0,height=0;
    uint8_t* pixels=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
    if(!pixels || width!=240 || height!=320 ||
       gfxGetScreenFormat(GFX_BOTTOM)!=GSP_RGB565_OES)return false;
    const Canvas c{pixels,320,240};
    c.rect(0,0,320,240,BG);
    header(c,s);
    if(f.page==phase20::AbilityPage::Select){
        lettering(c,12,38,"ABILITY SLOTS",WHITE,2);
        for(unsigned i=0;i<3;i++){
            const int y=55+int(i*46);
            c.rect(8,y,304,41,i==f.selectedSlot?GOLD:LINE);
            c.rect(10,y+2,300,37,i==f.selectedSlot?PANEL_HI:PANEL);
            c.circle(30,y+20,13,i==f.selectedSlot?GOLD:GREEN);
            c.circle(30,y+20,9,DARK);
            char index[3]{char('1'+i),'\0'};
            lettering(c,27,y+17,index,WHITE,1);
            lettering(c,54,y+11,phase20::slotLabel(i),WHITE,1,29);
            if(i==f.originalSlot)lettering(c,54,y+25,"CURRENT",GREEN,1);
        }
        lettering(c,11,195,"ACTUAL ABILITY NAME NOT VERIFIED",GOLD,1);
        c.rect(8,208,94,28,PANEL);
        lettering(c,32,218,"BACK",WHITE,1);
        c.rect(110,208,202,28,PANEL_HI);
        lettering(c,162,217,"REVIEW",GREEN,1);
    }else if(f.page==phase20::AbilityPage::Review){
        lettering(c,12,42,"REVIEW SLOT ONLY",WHITE,2);
        char from[24]{},to[24]{};
        std::snprintf(from,sizeof(from),"SLOT %u",f.originalSlot+1);
        std::snprintf(to,sizeof(to),"SLOT %u",f.selectedSlot+1);
        c.rect(8,80,304,48,PANEL);
        lettering(c,26,96,from,GOLD,2);
        lettering(c,157,96,">",WHITE,2);
        lettering(c,190,96,to,GREEN,2);
        lettering(c,12,150,"NO VERIFIED ABILITY MAPPING",GOLD,1);
        lettering(c,12,178,"GAME EDIT DISABLED",RED,2);
        c.rect(8,208,94,28,PANEL);
        lettering(c,32,218,"BACK",WHITE,1);
        lettering(c,125,218,"A ON 3DS: EXPLAIN BLOCK",MUTED,1);
    }else if(f.page==phase20::AbilityPage::Locked){
        lettering(c,10,51,"ABILITY EDIT LOCKED",RED,2);
        lettering(c,12,93,"EXACT ROM MAPPING REQUIRED",GOLD);
        lettering(c,12,119,"NO GAME SAVE MODIFIED",WHITE);
        lettering(c,12,146,"SLOT 1/2/3 IS NOT AN",MUTED);
        lettering(c,12,160,"ARBITRARY ABILITY ID",MUTED);
        c.rect(8,208,94,28,PANEL);
        lettering(c,32,218,"BACK",WHITE);
    }
    gfxFlushBuffers();
    return true;
}
}} // namespace phase20::gui
