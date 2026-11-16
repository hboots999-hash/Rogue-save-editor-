// SPDX-License-Identifier: GPL-3.0-or-later
#include "phase20/sprite_bmp_v1.hpp"
#include <array>
#include <cassert>
#include <iostream>
using namespace phase20::gui;
static void wr16(uint8_t* a,unsigned pos,uint16_t v){a[pos]=v&255;a[pos+1]=v>>8;}
static void wr32(uint8_t* a,unsigned pos,uint32_t v){
    wr16(a,pos,uint16_t(v));wr16(a,pos+2,uint16_t(v>>16));
}
int main(){
    std::array<uint8_t,54> h{};
    h[0]='B';h[1]='M';
    wr32(h.data(),2,54+48*48*3);wr32(h.data(),10,54);
    wr32(h.data(),14,40);wr32(h.data(),18,48);wr32(h.data(),22,48);
    wr16(h.data(),26,1);wr16(h.data(),28,24);wr32(h.data(),34,48*48*3);
    SpriteBmpInfo info{};
    assert(validateSpriteBmp(h.data(),h.size(),info));
    assert(info.pixelOffset==54 && info.rowBytes==144);
    for(unsigned p:{18u,22u,26u,28u,30u,34u}){
        auto copy=h;copy[p]=255;
        assert(!validateSpriteBmp(copy.data(),copy.size(),info));
    }
    for(unsigned len=0;len<54;len++){
        assert(!validateSpriteBmp(h.data(),len,info));
    }
    h[0]='X';assert(!validateSpriteBmp(h.data(),h.size(),info));
    std::cout<<"PASS: sprite BMP pinned dimensions, BGR24, bounded offset and malformed input\n";
}
