#include "live_header.hpp"
#include <cassert>
#include <array>
#include <cstring>
#include <iostream>
using namespace rogue_live;
static void w32(unsigned char* p,unsigned k,uint32_t v) {
    p[k]=uint8_t(v);p[k+1]=uint8_t(v>>8);p[k+2]=uint8_t(v>>16);p[k+3]=uint8_t(v>>24);
}
int main(){
    std::array<uint8_t,HeaderSize> a{},b{};
    Slot s{};
    assert(!parseHeader(a.data(),a.size(),0).valid);
    std::memcpy(a.data(),".SAV",4);
    w32(a.data(),0x50,0x200);w32(a.data(),0x54,0x20000);
    w32(a.data(),0x38,0x07d7ba00);w32(a.data(),0x3c,0x00040000);
    w32(a.data(),0x34,10);
    auto top=parseHeader(a.data(),a.size(),0);
    assert(top.valid&&top.title==ExpectedTitle&&top.timesSaved==10);
    auto bottom=top;bottom.offset=SlotSize;bottom.timesSaved=9;
    assert(selectLatest(top,bottom,s)&&s.offset==0);
    bottom.timesSaved=11;
    assert(selectLatest(top,bottom,s)&&s.offset==SlotSize);
    bottom.title=0x0004000000000001ULL;
    assert(!selectLatest(top,bottom,s)); // never use older matching title
    bottom.valid=false;
    assert(selectLatest(top,bottom,s)&&s.offset==0);
    top.title=0;
    assert(!selectLatest(top,bottom,s));
    w32(a.data(),0x54,32768);
    assert(!parseHeader(a.data(),a.size(),0).valid);
    a[0]=0;assert(!parseHeader(a.data(),a.size(),0).valid);
    assert(!parseHeader(nullptr,0,0).valid);
    std::cout<<"PASS Phase 11: Luma GBA VC header IDs, highest slot selection, wrong-title refusal\n";
}
