// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only native 3DS graphical move browser. The screen is drawn entirely
// from Phase19's existing authenticated/synthetic view model; NOT a writer.
#pragma once
#include "phase20/graphic_panel_v1.hpp"
#include "phase20/move_touch_v1.hpp"
#include "phase20/move_draft_v1.hpp"
#include "phase20/move_filter_v1.hpp"
#include "move_browser.hpp"
#include <algorithm>
#include <cstdio>
namespace phase20 { namespace gui {
inline void pill(const Canvas& c,int x,int y,int w,const char* text,
                 Color foreground=WHITE,Color bg=PANEL) {
    c.rect(x,y,w,26,bg);
    lettering(c,x+8,y+9,text,foreground,1,28);
}
inline void moveLine(const Canvas& c,int y,const char* heading,const char* detail,
                     bool selected,bool candidate){
    c.rect(8,y,304,23,selected?GOLD:LINE);
    c.rect(10,y+2,300,19,selected?PANEL_HI:PANEL);
    lettering(c,16,y+7,heading,candidate?WHITE:MUTED,1,29);
    lettering(c,217,y+7,detail,candidate?GREEN:GOLD,1,16);
}
inline void moveNav(const Canvas& c,const char* primary="BACK",bool left=false,
                    bool right=false,bool search=false) {
    if(search){
        pill(c,8,208,54,"BACK",WHITE,PANEL);
        pill(c,67,208,49,"PREV",WHITE,PANEL);
        pill(c,121,208,49,"NEXT",WHITE,PANEL);
        pill(c,175,208,68,"FILTER",GREEN,PANEL_HI);
        pill(c,248,208,64,"FIND",GREEN,PANEL_HI);
    }else {
        pill(c,8,208,102,primary,WHITE,PANEL);
        if(left || right)
            pill(c,116,208,196,left?"SEARCH":"SD DRAFT",GREEN,PANEL_HI);
    }
}
// Translate only the geometric, read-only tap selection to a normal
// Phase19 MoveKey. Confirmation A/X is deliberately absent from touch maps.
inline bool moveTapKey(const phase19::State& s,phase19::MoveBrowser& v,
                      unsigned x,unsigned y,phase19::MoveKey& key){
    using phase19::MovePage;
    const unsigned count=s.demo.partyStatus==phase19::Availability::Ready?
        std::min(6u,s.demo.partyCount):0u;
    const auto hit=moveTouchHit(v.page,x,y,count,v.matchCount,
                                v.selected,v.fromPartyPicker);
    switch(hit.action){
        case MoveTapAction::PartyRow:
            if(v.page!=MovePage::Party || hit.index>=count)return false;
            v.partySlot=hit.index;key=phase19::MoveKey::A;return true;
        case MoveTapAction::SlotRow:
            if(v.page!=MovePage::Slots || hit.index>=4u)return false;
            v.slot=hit.index;key=phase19::MoveKey::A;return true;
        case MoveTapAction::CatalogRow:
            if(v.page!=MovePage::Catalog || hit.index>=v.matchCount)return false;
            v.selected=hit.index;key=phase19::MoveKey::A;return true;
        case MoveTapAction::KeyboardRow:
            if(v.page!=MovePage::Keyboard || hit.index>=30u)return false;
            v.keyboardIndex=hit.index;key=phase19::MoveKey::A;return true;
        case MoveTapAction::Back:key=phase19::MoveKey::B;return true;
        case MoveTapAction::Previous:key=phase19::MoveKey::Left;return true;
        case MoveTapAction::Filter:key=phase19::MoveKey::X;return true;
        case MoveTapAction::Next:key=phase19::MoveKey::Right;return true;
        case MoveTapAction::Search:
        case MoveTapAction::ApplySearch:key=phase19::MoveKey::Y;return true;
        case MoveTapAction::StartDraft:
            if(v.page!=MovePage::Review ||
               !phase20::isExperimentalDraftCandidate(v,s.demo))return false;
            key=phase19::MoveKey::Y;return true;
        case MoveTapAction::None:return false;
    }
    return false;
}
inline bool drawMoveBrowser(const phase19::State& s,const phase19::MoveBrowser& v,
                            const phase20::MoveListFilter& filter,
                            const phase20::LocalMoveProvider* metadata=nullptr) {
    u16 width=0,height=0;
    uint8_t* buffer=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
    if(!buffer || width!=240 || height!=320 ||
       gfxGetScreenFormat(GFX_BOTTOM)!=GSP_RGB565_OES)return false;
    const Canvas c{buffer,320,240};
    c.rect(0,0,320,240,BG);
    header(c,s);
    using phase19::MovePage;
    using phase19::MoveEvidence;
    if(v.page==MovePage::Party){
        lettering(c,10,36,"CHOOSE POKEMON",MUTED);
        const unsigned count=s.demo.partyStatus==phase19::Availability::Ready?
            std::min(6u,s.demo.partyCount):0u;
        for(unsigned i=0;i<count;i++){
            const auto& m=s.demo.party[i];
            const int x=(i%2)?166:8,y=48+int((i/2)*54);
            c.rect(x,y,146,46,i==v.partySlot?GOLD:LINE);
            c.rect(x+2,y+2,142,42,i==v.partySlot?PANEL_HI:PANEL);
            emblem(c,x+20,y+23,0,m.egg?MUTED:GREEN);
            lettering(c,x+38,y+9,m.name,m.egg?MUTED:WHITE,1,17);
            char info[40]{};
            std::snprintf(info,sizeof(info),m.egg?"EGG - BLOCKED":"SPECIES %u",m.species);
            lettering(c,x+38,y+27,info,m.egg?GOLD:MUTED,1,18);
        }
        if(count==0)lettering(c,22,103,"NO PARTY POKEMON",GOLD,2);
        moveNav(c);
    } else if(!v.activeParty(s.demo)){
        lettering(c,14,72,"PARTY DATA CHANGED",RED,2);
        lettering(c,14,108,"REOPEN PARTY LIST",MUTED);
        moveNav(c);
    } else if(v.page==MovePage::Slots){
        const auto& mon=s.demo.party[v.partySlot];
        lettering(c,10,36,"CHOOSE MOVE SLOT",MUTED);
        for(unsigned i=0;i<4;i++){
            const int y=48+int(i*37);
            c.rect(8,y,304,33,i==v.slot?GOLD:LINE);
            c.rect(10,y+2,300,29,i==v.slot?PANEL_HI:PANEL);
            char info[48]{};
            const auto* old=v.existingMove(s.demo,i);
            if(old)std::snprintf(info,sizeof(info),"%u  %.24s",i+1,old->name);
            else std::snprintf(info,sizeof(info),"SLOT %u   MOVE ID %u",i+1,mon.moves[i]);
            lettering(c,18,y+11,info,mon.moves[i]?WHITE:MUTED,1,35);
            std::snprintf(info,sizeof(info),"PP %u",mon.pp[i]);
            lettering(c,265,y+11,info,mon.moves[i]?GREEN:GOLD,1,9);
        }
        lettering(c,10,193,"SELECT AN OCCUPIED SLOT",MUTED);
        moveNav(c);
    } else if(v.page==MovePage::Catalog) {
        char filterLine[52]{};
        std::snprintf(filterLine,sizeof(filterLine),"MOVES / %s",phase20::filterLabel(filter.mode));
        lettering(c,10,35,filterLine,GREEN,1,49);
        const unsigned first=(v.selected/phase19::BrowserPageRows)*
            phase19::BrowserPageRows;
        for(unsigned row=0;row<phase19::BrowserPageRows;row++){
            const unsigned ordinal=first+row;
            if(ordinal>=v.matchCount)break;
            unsigned index=0;
            if(!v.matchAt(ordinal,index))continue;
            const auto& entry=v.catalog->records[index];
            char name[32]{},right[22]{};
            std::snprintf(name,sizeof(name),"%.28s",entry.name);
            const auto pp=phase20::ppVariants(metadata,v,entry.id);
            if(pp.verified && pp.conflict)
                std::snprintf(right,sizeof(right),"%u PP%u/%u",
                    unsigned(entry.id),unsigned(pp.variantA),unsigned(pp.variantB));
            else if(entry.evidence==MoveEvidence::Candidate)
                std::snprintf(right,sizeof(right),"%u PP%u",
                    unsigned(entry.id),unsigned(entry.basePP));
            else std::snprintf(right,sizeof(right),"%u PP?",unsigned(entry.id));
            moveLine(c,40+int(row*26),name,right,
                     ordinal==v.selected,entry.evidence==MoveEvidence::Candidate);
        }
        if(!v.matchCount)lettering(c,16,88,"NO MOVE MATCHES",GOLD,2);
        char counter[60]{};
        std::snprintf(counter,sizeof(counter),"%u RESULTS   %s",v.matchCount,
                      v.query[0]?v.query:"ALL MOVES");
        lettering(c,12,197,counter,MUTED,1,48);
        moveNav(c,"BACK",false,false,true);
    } else if(v.page==MovePage::Keyboard){
        lettering(c,10,35,"SEARCH MOVE NAME",MUTED);
        for(unsigned i=0;i<30;i++){
            const int x=8+int(i%6)*50,y=42+int(i/6)*31;
            const char k=phase19::MoveBrowser::Keyboard[i];
            c.rect(x,y,46,27,i==v.keyboardIndex?GOLD:LINE);
            c.rect(x+2,y+2,42,23,i==v.keyboardIndex?PANEL_HI:PANEL);
            const char* key=k=='<'?"DEL":k=='!'?"GO":k==' '?"SPC":nullptr;
            char single[2]{k,'\0'};
            lettering(c,x+(key?11:19),y+9,key?key:single,
                      i==v.keyboardIndex?GOLD:WHITE,1,4);
        }
        pill(c,8,208,102,"CANCEL");
        pill(c,116,208,196,"APPLY SEARCH",GREEN,PANEL_HI);
        lettering(c,12,199,v.pending,MUTED,1,18);
    } else if(v.page==MovePage::Review ||
              v.page==MovePage::ConfirmOne || v.page==MovePage::ConfirmTwo){
        const auto& mon=s.demo.party[v.partySlot];
        lettering(c,10,36,v.page==MovePage::Review?"MOVE REPLACEMENT":
                  v.page==MovePage::ConfirmOne?"REVIEW ONLY 1 OF 2":
                  "REVIEW ONLY 2 OF 2",MUTED);
        c.rect(8,54,304,57,PANEL);
        lettering(c,17,60,"EXISTING MOVE",MUTED);
        const auto* original=v.existingMove(s.demo,v.slot);
        char info[64]{};
        std::snprintf(info,sizeof(info),"%s",original?original->name:"UNKNOWN NAME");
        lettering(c,17,81,info,WHITE,2,20);
        std::snprintf(info,sizeof(info),"ID %u / PP %u",
                      mon.moves[v.slot],mon.pp[v.slot]);
        lettering(c,184,64,info,GOLD,1,17);
        const auto oldPP=phase20::ppVariants(metadata,v,mon.moves[v.slot]);
        if(oldPP.verified){
            if(oldPP.conflict)std::snprintf(info,sizeof(info),"ROM PP A/B %u/%u",
                           unsigned(oldPP.variantA),unsigned(oldPP.variantB));
            else std::snprintf(info,sizeof(info),"ROM BASE PP %u",unsigned(oldPP.variantA));
            lettering(c,17,99,info,oldPP.conflict?GOLD:MUTED,1,40);
        }
        c.rect(8,118,304,59,PANEL_HI);
        lettering(c,17,123,"PROPOSED MOVE",GREEN);
        const auto* proposed=v.selectedMove();
        lettering(c,17,144,proposed?proposed->name:"UNAVAILABLE",
                  proposed?WHITE:RED,2,20);
        if(proposed){
            const auto pp=phase20::ppVariants(metadata,v,proposed->id);
            if(pp.verified && pp.conflict)
                std::snprintf(info,sizeof(info),"PP %u/%u",
                    unsigned(pp.variantA),unsigned(pp.variantB));
            else if(proposed->evidence==MoveEvidence::Candidate)
                std::snprintf(info,sizeof(info),"PP %u",unsigned(proposed->basePP));
            else std::snprintf(info,sizeof(info),"PP UNVERIFIED");
            lettering(c,206,130,info,
                pp.conflict?GOLD:GREEN,1,17);
        }
        const bool research=phase20::isExperimentalDraftCandidate(v,s.demo);
        lettering(c,10,184,research?"43-ID SD DRAFT CANDIDATE":
                    "SD DRAFT NOT VERIFIED",research?GREEN:GOLD,1,48);
        if(v.page==MovePage::Review){
            pill(c,8,208,102,"BACK");
            pill(c,116,208,196,research?"CHECK SD DRAFT":"DRAFT LOCKED",
                 research?GREEN:MUTED,research?PANEL_HI:PANEL);
        }else{
            pill(c,8,208,102,"BACK");
            lettering(c,122,217,"USE A ON 3DS",GOLD);
        }
    } else if(v.page==MovePage::Notice){
        lettering(c,10,42,"MOVE CHANGE BLOCKED",RED,2);
        lettering(c,14,88,"NO GAME SAVE WRITTEN",WHITE);
        lettering(c,14,113,"SELECT A VERIFIED MOVE",MUTED);
        pill(c,8,208,102,"BACK");
    }
    gfxFlushBuffers();
    return true;
}
inline bool drawMoveDraft(const phase19::State& s,
                          const phase20::MoveDraftFlow& f){
    u16 width=0,height=0;
    uint8_t* buffer=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
    if(!buffer || width!=240 || height!=320 ||
       gfxGetScreenFormat(GFX_BOTTOM)!=GSP_RGB565_OES)return false;
    const Canvas c{buffer,320,240};
    c.rect(0,0,320,240,BG);
    header(c,s);
    lettering(c,14,41,"OFFLINE MOVE DRAFT",WHITE,2);
    lettering(c,14,73,"EXPERIMENTAL SD COPY ONLY",GOLD);
    lettering(c,14,104,"NOT A GAME SAVE EDIT",RED,2);
    c.rect(8,142,304,61,PANEL);
    lettering(c,17,152,"ORIGINAL SAVE UNCHANGED",GREEN);
    lettering(c,17,172,"NO LIVE GAME RESTORE",MUTED);
    lettering(c,17,215,"A: CONTINUE  B: CANCEL",WHITE);
    gfxFlushBuffers();
    (void)f;
    return true;
}
}} // namespace phase20::gui
