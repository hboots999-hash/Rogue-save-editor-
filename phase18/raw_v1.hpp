// Phase18 private, versioned snapshot of the read-only Phase10 raw inspector and
// SD-only unique-copy helpers. Scoped to rogue18::raw_v1 to avoid colliding
// with Phase17's global ::h168 declarations in unified Phase20 compilation.
// Source provenance: phase10/preview/offline.hpp, GPL-3.0-or-later.
// Keep this isolated until an independently reviewed shared API is merged.
// No Phase13/14 restore-preflight adapter is included.
// GPL-3.0-or-later. Never opens installed-title archives, AGBSAVE or extdata.
#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <new>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __3DS__
#include <3ds.h>
#endif

namespace rogue18 {
namespace raw_v1 {
constexpr size_t Size=131072;
constexpr size_t Sector=4096;
struct Bank { bool valid=false; unsigned sector[14]{}; uint32_t counter=0,key=0; };
struct Info { bool valid=false; unsigned active=0,partyCount=0,items=0; uint32_t money=0; Bank banks[2]{}; };
struct Result { bool ok=false; std::string message; unsigned changed=0; };

inline uint16_t r16(const uint8_t* p,size_t i) {
    return uint16_t(p[i])|(uint16_t(p[i+1])<<8);
}
inline uint32_t r32(const uint8_t* p,size_t i){
    return uint32_t(r16(p,i))|(uint32_t(r16(p,i+2))<<16);
}
inline void w16(uint8_t* p,size_t i,uint16_t v){
    p[i]=uint8_t(v);p[i+1]=uint8_t(v>>8);
}
inline void w32(uint8_t* p,size_t i,uint32_t v){
    w16(p,i,uint16_t(v));w16(p,i+2,uint16_t(v>>16));
}
inline uint16_t checksum(const uint8_t* sector){
    uint32_t sum=0;
    for(size_t j=0;j<0xff4;j+=4)sum+=r32(sector,j);
    return uint16_t((sum>>16)+sum);
}
inline bool validMon(const uint8_t* source,size_t start){
    if(start+104>Size)return false;
    const uint32_t key=r32(source,start)^r32(source,start+4);
    uint32_t sum=0;
    for(unsigned j=0;j<48;j+=4) {
        const uint32_t word=r32(source,start+32+j)^key;
        sum+=uint16_t(word); sum+=uint16_t(word>>16);
    }
    return uint16_t(sum)==r16(source,start+28);
}
inline bool inspect(const uint8_t* source,size_t n,Info& out){
    out=Info{};
    if(!source||n!=Size)return false;
    for(unsigned b=0;b<2;b++){
        Bank& bank=out.banks[b];
        bool used[14]={};
        bool ok=true;
        uint32_t counter=0;
        for(unsigned p=0;p<14;p++){
            const size_t start=(b*14u+p)*Sector;
            const uint16_t id=r16(source,start+0xff4);
            const uint32_t c=r32(source,start+0xffc);
            if(id>=14 || used[id] || r32(source,start+0xff8)!=0x08012025u ||
               checksum(source+start)!=r16(source,start+0xff6) ||
               (p && c!=counter)){ok=false;break;}
            used[id]=true;
            bank.sector[id]=static_cast<unsigned>(start);
            counter=c;
        }
        if(!ok)continue;
        const unsigned s1=bank.sector[1];
        const unsigned party=r32(source,s1+0x234);
        if(party>6)continue;
        for(unsigned m=0;m<party;m++)
            if(!validMon(source,s1+0x238+104u*m)){ok=false;break;}
        if(!ok)continue;
        bank.counter=counter;
        bank.key=r32(source,bank.sector[0]+0x4c);
        const uint32_t money=r32(source,s1+0x4a8)^bank.key;
        if(money>999999u)continue;
        bank.valid=true;
    }
    // Strict two-complete-bank requirement, not sufficient for ROM authenticity.
    if(!out.banks[0].valid || !out.banks[1].valid ||
       out.banks[0].counter==out.banks[1].counter)return false;
    out.active=out.banks[1].counter>out.banks[0].counter?1u:0u;
    const Bank& bank=out.banks[out.active];
    const unsigned s1=bank.sector[1];
    out.money=r32(source,s1+0x4a8)^bank.key;
    out.partyCount=r32(source,s1+0x234);
    bool gap=false;
    for(unsigned j=0;j<450;j++){
        const size_t off=s1+0x578+4u*j;
        const uint16_t id=r16(source,off);
        if(!id){gap=true;continue;}
        if(gap)return false; // only a contiguous observed bag layout
        const unsigned qty=r16(source,off+2)^uint16_t(bank.key);
        if(qty<1||qty>999)return false;
        out.items++;
    }
    out.valid=true;
    return true;
}
inline unsigned species(const uint8_t* b,const Info& info,unsigned i){
    if(!b||!info.valid||i>=info.partyCount)return 0;
    const unsigned mon=info.banks[info.active].sector[1]+0x238+i*104u;
    const unsigned pid=r32(b,mon), ot=r32(b,mon+4);
    // Gen III canonical 24 permutations, find which 12-byte block is G.
    static const char* order[24]={
       "GAEM","GAME","GEAM","GEMA","GMAE","GMEA",
       "AGEM","AGME","AEGM","AEMG","AMGE","AMEG",
       "EGAM","EGMA","EAGM","EAMG","EMGA","EMAG",
       "MGAE","MGEA","MAGE","MAEG","MEGA","MEAG"
    };
    unsigned block=0;
    while(block<4&&order[pid%24][block]!='G')block++;
    if(block==4)return 0;
    return uint16_t(r32(b,mon+32+block*12)^(pid^ot));
}
inline bool item(const uint8_t* b,const Info& info,unsigned i,unsigned& id,unsigned& qty){
    if(!b||!info.valid||i>=info.items)return false;
    const Bank& bank=info.banks[info.active];
    const size_t off=bank.sector[1]+0x578+i*4u;
    id=r16(b,off);
    qty=r16(b,off+2)^uint16_t(bank.key);
    return id!=0 && qty>0 && qty<=999;
}
} // namespace raw_v1
} // namespace rogue18
