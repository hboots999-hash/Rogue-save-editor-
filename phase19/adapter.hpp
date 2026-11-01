// Phase19 read-only view boundary, GPL-3.0-or-later.
// No dependency on Phase18 branch, file I/O, save services or mutation APIs.
#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <type_traits>

namespace phase19 {
enum class Availability : uint8_t { Ready, Empty, Unsupported, Invalid, Unverified };
struct Pokemon {
    char name[20]{};
    unsigned species=0,level=0;
    std::array<unsigned,6> iv{},ev{};
    std::array<unsigned,4> moves{},pp{};
    unsigned ability=0,nature=0;
    bool egg=false;
};
struct Item { char name[20]{}; unsigned id=0,quantity=0; };
struct Demo {
    // Snapshot is immutable inside UI State. This flag cannot authorize writes.
    bool synthetic=false;
    Availability partyStatus=Availability::Unsupported;
    Availability pcStatus=Availability::Unsupported;
    Availability bagStatus=Availability::Unsupported;
    Availability moneyStatus=Availability::Unsupported;
    Availability backupStatus=Availability::Unverified;
    unsigned partyCount=0,pcBoxes=0,bagCount=0,money=0;
    unsigned activeCounter=0; // displayed only, no optimistic lock authorisation
    std::array<unsigned,14> pcCounts{};
    std::array<Pokemon,6> party{};
    std::array<std::array<Pokemon,30>,14> pc{};
    std::array<Item,450> bag{};
};

// Provider interface: Phase18 integrator will implement only after independent
// validation. Status never implies edit rights; no restore/write methods exist.
struct ReadOnlyProvider {
    virtual ~ReadOnlyProvider() = default;
    virtual Availability summary(unsigned& money,unsigned& counter) const=0;
    virtual Availability partyCount(unsigned& count) const=0;
    virtual Availability partyAt(unsigned index,Pokemon& mon) const=0;
    virtual Availability bagCount(unsigned& count) const=0;
    virtual Availability itemAt(unsigned index,Item& item) const=0;
    virtual Availability pcBoxCount(unsigned& count) const=0;
    virtual Availability pcAt(unsigned box,unsigned index,Pokemon& mon) const=0;
    virtual Availability pcBoxSize(unsigned box,unsigned& count) const=0;
};

inline bool validPokemon(const Pokemon& m) {
    if(!m.species || m.level>100 || m.ability>3 || m.nature>24)return false;
    unsigned evsum=0;
    for(unsigned i=0;i<6;i++){
        if(m.iv[i]>31 || m.ev[i]>252)return false;
        evsum+=m.ev[i];
    }
    if(evsum>510)return false;
    for(unsigned i=0;i<4;i++)
        if(m.pp[i]>64)return false;
    return true;
}
// Fail closed. Unsupported or invalid provider data must not leak mock values.
// Hard limits intentionally small until storage layout and paging are proven.
inline void captureInto(const ReadOnlyProvider& p, Demo& out) {
    static_assert(std::is_trivially_copyable<Demo>::value,"Snapshot must remain POD-like");
    std::fill_n(reinterpret_cast<uint8_t*>(&out),sizeof(out),uint8_t(0));
    unsigned n=0, val=0, counter=0;
    out.moneyStatus=p.summary(val,counter);
    if(out.moneyStatus==Availability::Ready && val<=999999) {
        out.money=val;out.activeCounter=counter;
    }else if(out.moneyStatus==Availability::Ready) out.moneyStatus=Availability::Invalid;
    out.partyStatus=p.partyCount(n);
    if(out.partyStatus==Availability::Ready) {
        if(n>out.party.size())out.partyStatus=Availability::Unsupported;
        else {
            out.partyCount=n;
            for(unsigned i=0;i<n;i++){
                if(p.partyAt(i,out.party[i])!=Availability::Ready ||
                   !validPokemon(out.party[i])) {
                    out.partyStatus=Availability::Invalid;out.partyCount=0;break;
                }
                out.party[i].name[19]='\0';
            }
            if(n==0 && out.partyStatus==Availability::Ready)
                out.partyStatus=Availability::Empty;
        }
    }
    n=0;
    out.bagStatus=p.bagCount(n);
    if(out.bagStatus==Availability::Ready) {
        if(n>out.bag.size())out.bagStatus=Availability::Unsupported;
        else {
            out.bagCount=n;
            for(unsigned i=0;i<n;i++){
                if(p.itemAt(i,out.bag[i])!=Availability::Ready ||
                   !out.bag[i].id || out.bag[i].quantity<1 ||
                   out.bag[i].quantity>999) {
                    out.bagStatus=Availability::Invalid;out.bagCount=0;break;
                }
                out.bag[i].name[19]='\0';
            }
            if(n==0 && out.bagStatus==Availability::Ready)
                out.bagStatus=Availability::Empty;
        }
    }
    n=0;
    out.pcStatus=p.pcBoxCount(n);
    if(out.pcStatus==Availability::Ready) {
        if(n>out.pc.size())out.pcStatus=Availability::Unsupported;
        else {
            out.pcBoxes=n;
            for(unsigned b=0;b<n;b++){
                unsigned slots=0;
                if(p.pcBoxSize(b,slots)!=Availability::Ready ||
                   slots>out.pc[b].size()) {
                    out.pcStatus=Availability::Unsupported;break;
                }
                out.pcCounts[b]=slots;
                for(unsigned i=0;i<slots;i++) {
                    if(p.pcAt(b,i,out.pc[b][i])!=Availability::Ready ||
                       !validPokemon(out.pc[b][i])) {
                        out.pcStatus=Availability::Invalid;break;
                    }
                    out.pc[b][i].name[19]='\0';
                }
                if(out.pcStatus!=Availability::Ready)break;
            }
            if(out.pcStatus!=Availability::Ready){
                out.pcBoxes=0;out.pcCounts.fill(0);
            }else if(n==0)out.pcStatus=Availability::Empty;
        }
    }
    // Real-save editing NEVER becomes active through capture().
    out.synthetic=false;
}
inline Demo capture(const ReadOnlyProvider& p) {
    Demo out{};
    captureInto(p,out);
    return out;
}
inline void populateMock(Demo& d) {
    static_assert(std::is_trivially_copyable<Demo>::value,
                  "Snapshot must stay stack-safe to clear in place");
    // Import->demo reset must not retain any private real-copy data in
    // inactive Pokémon/box slots or bag entries.
    std::fill_n(reinterpret_cast<uint8_t*>(&d),sizeof(d),uint8_t(0));
    d.synthetic=true;
    d.partyStatus=d.pcStatus=d.moneyStatus=d.bagStatus=Availability::Ready;
    d.partyCount=3;d.pcBoxes=2;d.pcCounts[0]=3;d.pcCounts[1]=3;d.bagCount=5;d.money=42000;
    d.party[0]={"Volcarona",637,72,{{31,10,30,31,31,31}},{{0,0,0,252,4,252}},
                {{53,483,94,247}},{{15,10,10,15}},1,10,false};
    d.party[1]={"Swampert",260,68,{{31,31,31,4,31,25}},{{252,252,4,0,0,0}},
                {{89,127,182,57}},{{10,15,10,15}},0,3,false};
    d.party[2]={"Gardevoir",282,65,{{27,1,24,31,30,31}},{{0,0,4,252,0,252}},
                {{94,85,247,347}},{{10,15,15,20}},1,15,false};
    d.pc[0][0]={"Gengar",94,60,{{31,5,20,31,15,31}},{{0,0,0,252,4,252}},
                 {{94,247,109,399}},{{10,15,10,20}},0,10,false};
    d.pc[0][1]={"Lucario",448,62,{{31,31,20,15,25,31}},{{0,252,4,0,0,252}},
                 {{370,245,396,197}},{{5,10,10,15}},1,3,false};
    d.pc[0][2]={"Metagross",376,64,{{31,31,31,5,31,10}},{{252,252,4,0,0,0}},
                 {{309,89,428,418}},{{5,10,5,10}},0,2,false};
    d.pc[1][0]={"Garchomp",445,67,{{31,31,31,3,15,31}},{{0,252,4,0,0,252}},
                 {{89,200,242,337}},{{10,15,15,10}},1,3,false};
    d.pc[1][1]={"Milotic",350,55,{{31,2,31,31,31,18}},{{252,0,252,4,0,0}},
                 {{57,58,105,392}},{{15,15,20,10}},0,5,false};
    d.pc[1][2]={"Breloom",286,56,{{31,31,23,8,20,31}},{{4,252,0,0,0,252}},
                 {{147,183,317,264}},{{15,15,10,10}},1,3,false};
    d.bag[0]={"Rare Candy",68,25};
    d.bag[1]={"Poke Ball",4,50};
    d.bag[2]={"Ultra Ball",2,30};
    d.bag[3]={"Potion",13,18};
    d.bag[4]={"Repel",86,12};
}
// Historical value-return helper is retained for host fixtures only.
// The native entry point never calls this large value-returning helper.
inline Demo mockSnapshot() {
    Demo snapshot{};
    populateMock(snapshot);
    return snapshot;
}
// Static in-place population: never create an enormous automatic local Demo
// when initializing the ARM11 application. Returned reference is read-only.
inline const Demo& mockSnapshotRef() {
    static Demo fixture{};
    static const bool loaded=[]() { populateMock(fixture);return true; }();
    (void)loaded;
    return fixture;
}
inline const char* availabilityText(Availability v) {
    switch(v) {
        case Availability::Ready: return "READABLE";
        case Availability::Empty: return "EMPTY";
        case Availability::Unsupported: return "UNSUPPORTED";
        case Availability::Invalid: return "INVALID - DO NOT EDIT";
        default: return "NOT VERIFIED";
    }
}
} // namespace phase19
