// Original standalone SHA-256 implementation for Phase18 source snapshot locking.
// GPL-3.0-or-later. Pure memory, no external crypto libraries or filesystem.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace rogue18 {
using Digest256=std::array<uint8_t,32>;
inline bool hasDigest(const Digest256& d){
    for(uint8_t n:d)if(n)return true;
    return false;
}
inline uint32_t rotr32(uint32_t value,unsigned n){
    return (value>>n)|(value<<(32-n));
}
inline Digest256 sourceDigest(const uint8_t* data,size_t len){
    if(!data)return {};
    constexpr uint32_t k[64]={
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
        0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
        0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
        0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
        0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
        0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
        0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
        0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
        0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
    uint32_t h[8]={
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    const auto process=[&](const uint8_t* b){
        uint32_t w[64]{};
        for(unsigned j=0;j<16;j++)w[j]=(uint32_t(b[j*4])<<24)|
           (uint32_t(b[j*4+1])<<16)|(uint32_t(b[j*4+2])<<8)|b[j*4+3];
        for(unsigned j=16;j<64;j++){
            const uint32_t a=w[j-15],c=w[j-2];
            const uint32_t s0=rotr32(a,7)^rotr32(a,18)^(a>>3);
            const uint32_t s1=rotr32(c,17)^rotr32(c,19)^(c>>10);
            w[j]=w[j-16]+s0+w[j-7]+s1;
        }
        uint32_t a=h[0],b0=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],q=h[7];
        for(unsigned j=0;j<64;j++){
            const uint32_t s1=rotr32(e,6)^rotr32(e,11)^rotr32(e,25);
            const uint32_t ch=(e&f)^(~e&g);
            const uint32_t tmp1=q+s1+ch+k[j]+w[j];
            const uint32_t s0=rotr32(a,2)^rotr32(a,13)^rotr32(a,22);
            const uint32_t maj=(a&b0)^(a&c)^(b0&c);
            const uint32_t tmp2=s0+maj;
            q=g;g=f;f=e;e=d+tmp1;d=c;c=b0;b0=a;a=tmp1+tmp2;
        }
        h[0]+=a;h[1]+=b0;h[2]+=c;h[3]+=d;
        h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=q;
    };
    size_t used=0;
    while(len-used>=64){process(data+used);used+=64;}
    uint8_t tail[128]{};
    const size_t rem=len-used;
    for(size_t j=0;j<rem;j++)tail[j]=data[used+j];
    tail[rem]=0x80;
    const uint64_t bits=uint64_t(len)*8u;
    const size_t finalBlock=(rem+1+8<=64)?64:128;
    for(unsigned j=0;j<8;j++)tail[finalBlock-8+j]=uint8_t(bits>>(56-8*j));
    process(tail);
    if(finalBlock==128)process(tail+64);
    Digest256 out{};
    for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)
        out[4*i+j]=uint8_t(h[i]>>(24-8*j));
    return out;
}
} // namespace rogue18
