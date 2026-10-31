// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "authenticated_restore.hpp"
namespace phase14 {
inline Bytes encodeJournal(const Plan& p) {
    if(p.before.size()!=p.after.size() || p.before.size()>MaxContainer)return {};
    Bytes b(32,0);std::memcpy(b.data(),"P14REC01",8);
    h168::w32(b.data(),8,uint32_t(p.before.size()));
    h168::w32(b.data(),12,uint32_t(p.op)); h168::w32(b.data(),16,p.item);
    b.insert(b.end(),p.before.begin(),p.before.end());
    b.insert(b.end(),p.after.begin(),p.after.end());
    auto sum=hash(b.data(),b.size());b.insert(b.end(),sum.begin(),sum.end());return b;
}
inline bool decodeJournal(const Bytes& b,Plan& p) {
    p={};if(b.size()<64 || std::memcmp(b.data(),"P14REC01",8))return false;
    size_t n=rogue_live::load32(b.data(),8);
    if(n<rogue_live::ContainerMin || n>MaxContainer || b.size()!=64+2*n)return false;
    for(size_t i=20;i<32;i++)if(b[i])return false;
    const auto d=hash(b.data(),b.size()-32);
    if(!std::equal(d.begin(),d.end(),b.end()-32))return false;
    p.before=Bytes(b.begin()+32,b.begin()+32+n);p.after=Bytes(b.begin()+32+n,b.end()-32);
    if(!layout(p.before,p.slots)){p={};return false;}
    const auto op=rogue_live::load32(b.data(),12);
    if(op!=1 && op!=2){p={};return false;}
    p.op=static_cast<rogue_restore::Operation>(op);p.item=rogue_live::load32(b.data(),16);return true;
}
}
