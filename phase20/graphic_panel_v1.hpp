// SPDX-License-Identifier: GPL-3.0-or-later
// Pixel-drawn Nintendo 3DS bottom dashboard. No textures/ROM/sprites, filesystem,
// save editing, or installed-game access. Existing text upper screen is untouched.
// Touch actions are decoded exclusively by touch_layout_v1.hpp.
#pragma once
#include <3ds.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "ui.hpp"
#include "phase20/touch_layout_v1.hpp"
#include "phase20/sprite_bmp_v1.hpp"
namespace phase20 { namespace gui {
struct Color { uint8_t r,g,b; };
constexpr Color BG{8,18,32}, PANEL{19,39,56}, PANEL_HI{22,62,68},
                GREEN{45,209,148}, GOLD{245,195,97}, WHITE{231,247,242},
                MUTED{139,178,186}, LINE{48,80,89}, DARK{6,29,40},
                RED{247,132,128};
struct Canvas {
    uint8_t* pixels=nullptr;
    unsigned width=0,height=0;
    void pixel(int x,int y,Color c) const {
        if(x<0 || y<0 || x>=static_cast<int>(width) ||
           y>=static_cast<int>(height))return;
        // libctru consoleInit(GFX_BOTTOM, ...) switches the framebuffer to
        // RGB565 and disables double buffering. Physical pixels are 320x240,
        // but gfxGetFramebuffer reports the rotated memory dimensions 240x320.
        // Never write three bytes to the two-byte console framebuffer.
        const size_t at=size_t(x)*height+(height-1u-unsigned(y));
        const uint16_t rgb565=uint16_t(((unsigned(c.r)>>3)<<11)|
                                       ((unsigned(c.g)>>2)<<5)|
                                       (unsigned(c.b)>>3));
        reinterpret_cast<uint16_t*>(pixels)[at]=rgb565;
    }
    void rect(int x,int y,int w,int h,Color c) const {
        if(!pixels || w<=0 || h<=0)return;
        int a=x<0?0:x,b=y<0?0:y;
        int r=x+w>int(width)?int(width):x+w;
        int d=y+h>int(height)?int(height):y+h;
        for(int yy=b;yy<d;yy++)for(int xx=a;xx<r;xx++)pixel(xx,yy,c);
    }
    void frame(int x,int y,int w,int h,Color c) const {
        rect(x,y,w,2,c);rect(x,y+h-2,w,2,c);
        rect(x,y,2,h,c);rect(x+w-2,y,2,h,c);
    }
    void circle(int cx,int cy,int r,Color c) const {
        for(int y=-r;y<=r;y++)for(int x=-r;x<=r;x++)
            if(x*x+y*y<=r*r)pixel(cx+x,cy+y,c);
    }
};
// 5x7 monochrome uppercase pixel font. Text is drawn directly, with no
// external font/sprite dependency and no large ARM11 stack arrays.
inline const uint8_t* glyph(char ch) {
    static constexpr uint8_t uppercase[26][7]={
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,14}, {17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
    };
    static constexpr uint8_t digits[10][7]={
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
    };
    static constexpr uint8_t space[7]{};
    static constexpr uint8_t colon[7]{0,4,4,0,4,4,0};
    static constexpr uint8_t dot[7]{0,0,0,0,0,6,6};
    static constexpr uint8_t dash[7]{0,0,0,31,0,0,0};
    static constexpr uint8_t slash[7]{1,1,2,4,8,16,16};
    static constexpr uint8_t arrow[7]{4,2,31,2,4,0,0};
    static constexpr uint8_t question[7]{14,17,1,2,4,0,4};
    if(ch>='a' && ch<='z')ch=char(ch-('a'-'A'));
    if(ch>='A' && ch<='Z')return uppercase[ch-'A'];
    if(ch>='0' && ch<='9')return digits[ch-'0'];
    switch(ch){
        case ' ':return space;
        case ':':return colon;
        case '.':return dot;
        case '-':return dash;
        case '/':return slash;
        case '>':return arrow;
        default:return question;
    }
}
inline void lettering(const Canvas& c,int x,int y,const char* title,Color color,unsigned scale=1,unsigned maxChars=40) {
    if(!title || !scale || scale>3)return;
    for(unsigned i=0;i<maxChars && title[i];i++) {
        if(x+int(i)*int(6*scale)>=int(c.width))break;
        const uint8_t* bits=glyph(title[i]);
        for(unsigned yy=0;yy<7;yy++)for(unsigned xx=0;xx<5;xx++)
            if(bits[yy]&(1u<<(4u-xx)))
                c.rect(x+int(i)*int(6*scale)+int(xx*scale),
                       y+int(yy*scale),int(scale),int(scale),color);
    }
}
inline void emblem(const Canvas& c,int cx,int cy,unsigned kind,Color tint) {
    c.circle(cx,cy,12,tint);
    c.circle(cx,cy,9,DARK);
    if(kind==0 || kind==1) {
        c.rect(cx-11,cy-1,22,3,tint);
        c.circle(cx,cy,5,WHITE);
        c.circle(cx,cy,3,DARK);
    } else if(kind==2) {
        c.rect(cx-6,cy-3,12,10,tint);
        c.rect(cx-3,cy-7,6,5,tint);
    } else if(kind==3) {
        c.circle(cx,cy,7,GOLD);
        lettering(c,cx-3,cy-4,"1",DARK);
    } else if(kind==4) {
        c.rect(cx-7,cy-5,12,11,tint);
        c.rect(cx-4,cy-8,12,11,WHITE);
    } else if(kind==5){
        c.rect(cx-2,cy-7,4,14,WHITE);
        c.rect(cx-7,cy-2,14,4,WHITE);
    }else{
        c.rect(cx-2,cy-9,4,13,WHITE);
        c.rect(cx-6,cy,12,3,WHITE);
        c.rect(cx-3,cy+3,6,3,WHITE);
    }
}
// Optional locally supplied species portrait. Assets are read ONLY from SDMC;
// no official Pokémon sprites are bundled, and no executable, save or title
// archive is ever read/written for artwork. 8 cache slots ~54 KiB static BSS.
struct CachedSprite {
    unsigned species=0;
    bool attempted=false,valid=false;
    uint8_t bgr[48*48*3]{};
};
inline const CachedSprite* spriteFor(unsigned species){
    if(!species || species>9999u)return nullptr;
    static CachedSprite cache[8]{};
    CachedSprite& item=cache[species%8];
    if(item.attempted && item.species==species)return item.valid?&item:nullptr;
    item.species=species;item.attempted=true;item.valid=false;
    char path[96]{};
    const int n=std::snprintf(path,sizeof(path),
        "sdmc:/3ds/PKSM-Rogue/sprites/%04u.bmp",species);
    if(n<=0 || static_cast<size_t>(n)>=sizeof(path))return nullptr;
    std::FILE* file=std::fopen(path,"rb");
    if(!file)return nullptr;
    uint8_t header[54]{};
    SpriteBmpInfo info{};
    const bool ok=std::fread(header,1,sizeof(header),file)==sizeof(header) &&
        validateSpriteBmp(header,sizeof(header),info) &&
        std::fseek(file,0,SEEK_END)==0;
    const long length=ok?std::ftell(file):-1;
    if(ok && length==long(bmp32(header,2)) &&
       length>=long(info.pixelOffset+sizeof(item.bgr)) &&
       std::fseek(file,long(info.pixelOffset),SEEK_SET)==0 &&
       std::fread(item.bgr,1,sizeof(item.bgr),file)==sizeof(item.bgr))
        item.valid=true;
    std::fclose(file);
    return item.valid?&item:nullptr;
}
inline void drawCreaturePlaceholder(const Canvas& c,int x,int y,int size,
                                   unsigned species){
    // Generic, original pixel avatar rather than an unlicensed species image.
    const Color fur=species%3==0?GREEN:(species%3==1?GOLD:MUTED);
    c.circle(x+size/2,y+size/2,size/2-1,PANEL_HI);
    c.rect(x+size/4,y+size/5,size/7,size/3,fur);
    c.rect(x+size*4/7,y+size/5,size/7,size/3,fur);
    c.circle(x+size/2,y+size*3/5,size/3,fur);
    c.rect(x+size/3,y+size/2,size/12,size/12,DARK);
    c.rect(x+size*3/5,y+size/2,size/12,size/12,DARK);
    c.rect(x+size*2/5,y+size*3/4,size/5,size/18,DARK);
}
inline void drawSpeciesPortrait(const Canvas& c,int x,int y,unsigned species,
                               int size){
    if(size<20 || size>70)return;
    const CachedSprite* data=spriteFor(species);
    if(!data){drawCreaturePlaceholder(c,x,y,size,species);return;}
    c.rect(x,y,size,size,PANEL_HI);
    for(int py=0;py<size;py++)for(int px=0;px<size;px++){
        const int sx=px*48/size, sy=py*48/size;
        const size_t idx=size_t((47-sy)*48+sx)*3u;
        const uint8_t b=data->bgr[idx],g=data->bgr[idx+1],r=data->bgr[idx+2];
        // Pure magenta (#FF00FF) is reserved as optional transparency.
        if(r==255 && g==0 && b==255)continue;
        c.pixel(x+px,y+py,Color{r,g,b});
    }
}
inline void tile(const Canvas& c,int x,int y,int w,int h,const char* label,
                 bool chosen,unsigned iconKind,bool enabled=true) {
    const Color fill=chosen?PANEL_HI:PANEL;
    c.rect(x,y,w,h,chosen?GOLD:LINE);
    c.rect(x+2,y+2,w-4,h-4,fill);
    emblem(c,x+22,y+h/2,iconKind,enabled?GREEN:MUTED);
    lettering(c,x+40,y+h/2-7,label,enabled?WHITE:MUTED,2,13);
}
inline void header(const Canvas& c,const phase19::State& s) {
    c.rect(0,0,320,32,DARK);
    c.rect(0,30,320,2,GREEN);
    lettering(c,10,7,"PKSM ROGUE",WHITE,2);
    c.rect(204,7,106,18,s.demo.synthetic?PANEL:PANEL_HI);
    lettering(c,210,12,s.demo.synthetic?"DEMO MODE":"SD COPY",s.demo.synthetic?GOLD:GREEN,1);
}
inline bool draw(const phase19::State& s) {
    if(s.screen!=phase19::Screen::Home && s.screen!=phase19::Screen::Party &&
       s.screen!=phase19::Screen::Pokemon)return false;
    u16 width=0,height=0;
    uint8_t* buffer=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
    // ctru's reported dimensions describe rotated framebuffer storage:
    // 240 pixel stride, 320 columns. Require known console RGB565 format.
    if(!buffer || width!=240 || height!=320 ||
       gfxGetScreenFormat(GFX_BOTTOM)!=GSP_RGB565_OES)return false;
    const Canvas c{buffer,320,240};
    c.rect(0,0,320,240,BG);
    header(c,s);
    if(s.screen==phase19::Screen::Home) {
        static constexpr const char* labels[6]{
            "PARTY","PC BOXES","BAG","MONEY","BACKUPS","RECOVERY"
        };
        for(unsigned row=0;row<3;row++)for(unsigned col=0;col<2;col++){
            const unsigned id=row*2+col;
            tile(c,col?166:8,42+int(row*54),146,46,labels[id],s.homeRow==id,id);
        }
        c.rect(8,208,304,28,s.homeRow==6?GOLD:LINE);
        c.rect(10,210,300,24,s.homeRow==6?PANEL_HI:PANEL);
        lettering(c,47,215,"IMPORT SD COPY",WHITE,2);
    } else if(s.screen==phase19::Screen::Party) {
        lettering(c,10,36,"CHOOSE A POKEMON",MUTED);
        const unsigned count=s.demo.partyStatus==phase19::Availability::Ready ?
            (s.demo.partyCount<6?s.demo.partyCount:6u):0u;
        for(unsigned i=0;i<count;i++) {
            const int x=(i%2)?166:8,y=48+int(i/2)*54;
            const auto& mon=s.demo.party[i];
            c.rect(x,y,146,46,s.selected==i?GOLD:LINE);
            c.rect(x+2,y+2,142,42,s.selected==i?PANEL_HI:PANEL);
            drawSpeciesPortrait(c,x+4,y+6,mon.species,34);
            lettering(c,x+42,y+8,mon.name,WHITE,1,16);
            char lv[26]{};
            if(mon.level)std::snprintf(lv,sizeof(lv),"LV %u",mon.level);
            else std::snprintf(lv,sizeof(lv),"SPECIES %u",mon.species);
            lettering(c,x+42,y+24,lv,MUTED,1);
        }
        if(count==0)lettering(c,20,108,"NO VALID PARTY DATA",GOLD,2);
        c.rect(8,210,102,26,PANEL);
        c.rect(116,210,196,26,PANEL_HI);
        lettering(c,34,219,"BACK",WHITE);
        lettering(c,131,219,"BROWSE MOVES",GREEN);
    } else {
        if(!phase19::selectedPokemonValid(s)) {
            lettering(c,12,90,"POKEMON UNAVAILABLE",RED,2);
        } else {
            const auto& mon=phase19::pokemon(s);
            drawSpeciesPortrait(c,12,40,mon.species,58);
            lettering(c,83,43,mon.name,WHITE,2,19);
            char info[72]{};
            std::snprintf(info,sizeof(info),"LV %u    SPECIES %u",
                          mon.level,mon.species);
            lettering(c,85,65,info,MUTED,1,36);
            std::snprintf(info,sizeof(info),"ABILITY %u   NATURE %u",
                          mon.ability,mon.nature);
            lettering(c,85,83,info,GREEN,1,36);
            c.rect(8,104,304,49,PANEL);
            static constexpr const char* stat[6]{"HP","ATK","DEF","SPA","SPD","SPE"};
            for(unsigned i=0;i<6;i++){
                const int x=15+int(i%2)*151,y=111+int(i/2)*14;
                std::snprintf(info,sizeof(info),"%s IV%u EV%u",
                              stat[i],mon.iv[i],mon.ev[i]);
                lettering(c,x,y,info,WHITE,1,23);
            }
            lettering(c,12,156,"ACTIVE MOVES",MUTED);
            for(unsigned i=0;i<4;i++){
                const int x=8+int(i%2)*158,y=168+int(i/2)*18;
                c.rect(x,y,150,16,PANEL);
                std::snprintf(info,sizeof(info),"M%u ID%u PP%u",
                              i+1,mon.moves[i],mon.pp[i]);
                lettering(c,x+7,y+4,info,WHITE,1,23);
            }
        }
        c.rect(8,204,94,32,PANEL);
        lettering(c,28,216,"BACK",WHITE,1);
        c.rect(110,204,94,32,s.fromPC?LINE:PANEL_HI);
        lettering(c,132,216,"EDIT",s.fromPC?MUTED:GREEN);
        c.rect(212,204,100,32,s.fromPC?LINE:PANEL_HI);
        lettering(c,234,216,"MOVES",s.fromPC?MUTED:GREEN);
        if(s.fromPC)lettering(c,8,194,"PC EDITS NOT SUPPORTED",GOLD);
    }
    gfxFlushBuffers();
    return true;
}
}} // namespace phase20::gui
