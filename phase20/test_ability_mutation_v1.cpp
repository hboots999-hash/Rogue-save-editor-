// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic (NON-PLAYABLE) Phase18 ability-slot encryption regression.
// HOST-ONLY: synthetic Evidence.verifiedAbilitySlot is *test-fixture proof*
// and must NEVER be copied into the native application or real save gate.
#include "phase18/offline_engine.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
using namespace rogue18;
#define CHECK(x) do { if(!(x)){std::cerr<<"FAIL ability test line "<<__LINE__<<": "<<#x<<"\n";return 1;} }while(0)
static Bytes fixture(unsigned permutation){
    Bytes b(h168::Size,0);
    for(unsigned bank=0;bank<2;bank++){
        const unsigned rot=bank?5u:3u;
        const uint32_t key=bank?0xf00dbabeu:0xa5a5c33cu;
        const size_t s0=(bank*14+(rot+0)%14)*h168::Sector;
        const size_t s1=(bank*14+(rot+1)%14)*h168::Sector;
        h168::w32(b.data(),s0+0x4c,key);
        h168::w32(b.data(),s1+0x234,6);
        h168::w32(b.data(),s1+0x4a8,(40000u+bank)^key);
        for(unsigned mon=0;mon<6;mon++){
            const size_t at=s1+layout::PartyStart+layout::PartyStride*mon;
            const uint32_t pid=24u*(mon+1u)+permutation;
            h168::w32(b.data(),at,pid);
            h168::w32(b.data(),at+4,0x11111000u+mon);
            b[at+84]=uint8_t(35+mon);
            b[at+100]=0xa7;b[at+103]=0x19;
            std::array<uint8_t,48> p{};
            h168::w32(p.data(),0,uint32_t(25+mon)|(uint32_t(99)<<11));
            h168::w16(p.data(),12,33);
            h168::w16(p.data(),14,45);
            p[20]=15;p[21]=25;
            p[24]=4;p[25]=100;p[27]=100;
            uint32_t ivs=0;
            for(unsigned s=0;s<6;s++)ivs|=uint32_t(10+s)<<(5*s);
            if(mon%2)ivs|=uint32_t(1)<<31; // Gigantamax flag, NOT ability
            h168::w32(p.data(),40,ivs);
            h168::w32(p.data(),44,0x00123456u|
                                     (uint32_t(mon%3)<<29));
            encode(b.data(),at,p);
        }
        const uint16_t itemIds[]={113,1,5};
        const uint16_t itemQty[]={10,2,12};
        for(unsigned j=0;j<3;j++){
            const size_t at=s1+layout::BagArray+4*j;
            h168::w16(b.data(),at,itemIds[j]);
            h168::w16(b.data(),at+2,uint16_t(itemQty[j]^uint16_t(key)));
        }
        for(unsigned p=0;p<14;p++){
            uint8_t* sector=b.data()+(bank*14+p)*h168::Sector;
            const unsigned section=(p+14-rot)%14;
            h168::w16(sector,0xff4,uint16_t(section));
            h168::w16(sector,0xff6,h168::checksum(sector));
            h168::w32(sector,0xff8,0x08012025);
            h168::w32(sector,0xffc,40+bank);
        }
    }
    return b;
}
static Request request(const Bytes& b,unsigned slot){
    h168::Info info{};
    if(!validate(b.data(),b.size(),info))return {};
    Mon original{};
    if(!partyAt(b.data(),b.size(),info,slot,original))return {};
    Request q{};
    q.kind=Kind::AbilitySlot;
    q.slot=slot;
    q.expectedCounter=info.banks[info.active].counter;
    q.expectedDigest=sourceDigest(b.data(),b.size());
    q.expectedPid=original.pid;
    q.expectedOt=original.ot;
    q.expectedSpecies=original.species;
    q.expectedPokemonChecksum=original.checksum;
    q.abilitySlot=uint8_t((original.abilitySlot+1u)%3u);
    return q;
}
int main(){
    unsigned verified=0;
    Evidence syntheticEvidence{}; // intentionally false in production
    for(unsigned permutation=0;permutation<24;permutation++){
        const Bytes original=fixture(permutation);
        h168::Info info{};
        CHECK(validate(original.data(),original.size(),info));
        CHECK(info.active==1 && info.partyCount==6 && info.money==40001);
        for(unsigned mon=0;mon<6;mon++){
            Mon before{};
            CHECK(partyAt(original.data(),original.size(),info,mon,before));
            CHECK(before.abilitySlot==mon%3 && before.gigantamaxFactor==mon%2);
            const Request base=request(original,mon);
            // Production feature flag is absent: EVERYTHING is denied.
            CHECK(!propose(original.data(),original.size(),base).ok);
            for(unsigned next=0;next<3;next++){
                if(next==before.abilitySlot)continue;
                Request q=base;q.abilitySlot=uint8_t(next);
                syntheticEvidence.verifiedAbilitySlot=true;
                const auto outcome=propose(original.data(),original.size(),q,
                                          syntheticEvidence);
                CHECK(outcome.ok && outcome.candidate.size()==original.size());
                CHECK(verifyExact(original.data(),original.size(),
                      outcome.candidate.data(),outcome.candidate.size(),
                      q,syntheticEvidence));
                h168::Info after{};
                CHECK(validate(outcome.candidate.data(),outcome.candidate.size(),after));
                Mon updated{};
                CHECK(partyAt(outcome.candidate.data(),outcome.candidate.size(),after,mon,updated));
                CHECK(updated.abilitySlot==next &&
                      updated.pid==before.pid && updated.ot==before.ot &&
                      updated.species==before.species &&
                      updated.gigantamaxFactor==before.gigantamaxFactor &&
                      updated.moves==before.moves && updated.pp==before.pp &&
                      updated.ivs==before.ivs && updated.evs==before.evs &&
                      updated.effectiveNature==before.effectiveNature &&
                      updated.heldItem==before.heldItem &&
                      updated.egg==before.egg);
                const size_t s1=info.banks[info.active].sector[1];
                const size_t at=s1+layout::PartyStart+layout::PartyStride*mon;
                const size_t field=at+32+storedIndex(before.pid,'M')*12+8;
                const size_t checksum=at+28,footer=s1+0xff6;
                CHECK(!outcome.changedOffsets.empty());
                for(size_t pos=0;pos<original.size();pos++){
                    if(original[pos]==outcome.candidate[pos])continue;
                    CHECK((pos>=field && pos<field+4) ||
                          (pos>=checksum && pos<checksum+2) ||
                          (pos>=footer && pos<footer+2));
                }
                for(unsigned other=0;other<6;other++){
                    if(other==mon)continue;
                    Mon compare{};
                    CHECK(partyAt(outcome.candidate.data(),outcome.candidate.size(),
                                  after,other,compare));
                    Mon baseline{};
                    CHECK(partyAt(original.data(),original.size(),info,other,baseline));
                    CHECK(compare.checksum==baseline.checksum &&
                          compare.abilitySlot==baseline.abilitySlot &&
                          compare.pid==baseline.pid);
                }
                CHECK(std::memcmp(original.data(),outcome.candidate.data(),
                                  14*h168::Sector)==0); // inactive bank
                Bytes tampered=outcome.candidate;
                tampered[at+100]^=0x01; // 104-byte extension must be identical
                CHECK(!verifyExact(original.data(),original.size(),
                      tampered.data(),tampered.size(),q,syntheticEvidence));
                ++verified;
            }
            Request invalid=base;invalid.abilitySlot=3;
            CHECK(!propose(original.data(),original.size(),
                           invalid,syntheticEvidence).ok);
            invalid=base;invalid.expectedCounter++;
            CHECK(!propose(original.data(),original.size(),
                           invalid,syntheticEvidence).ok);
            invalid=base;invalid.expectedDigest[0]^=1u;
            CHECK(!propose(original.data(),original.size(),
                           invalid,syntheticEvidence).ok);
            invalid=base;invalid.expectedSpecies++;
            CHECK(!propose(original.data(),original.size(),
                           invalid,syntheticEvidence).ok);
            invalid=base;invalid.expectedPokemonChecksum++;
            CHECK(!propose(original.data(),original.size(),
                           invalid,syntheticEvidence).ok);
            invalid=base;invalid.abilitySlot=before.abilitySlot;
            CHECK(!propose(original.data(),original.size(),
                           invalid,syntheticEvidence).ok);
        }
    }
    CHECK(verified==24u*6u*2u);
    std::cout<<"PASS "<<verified
             <<" synthetic encrypted ability drafts: 24 orders x 6 party slots"
             <<" x 2 new slots; exact byte footprint; inactive bank; identity;"
             <<" Gmax; corruptions and stale fingerprints blocked\n";
}
