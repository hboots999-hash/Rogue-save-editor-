// Synthetic-only comprehensive MOVE-V3 test; no real game/save bytes.
#ifndef ROGUE18_TEST_MOVE_V2
#error "Synthetic fixture tests must not ship in the CIA"
#endif
#include "move_replacement_v3.hpp"
#define main offline_fixture_main_unused_v3
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "test_offline_engine.cpp"
#pragma GCC diagnostic pop
#undef main
#undef CHECK
#include <cstdio>
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"MOVE V3 FAILED "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
using namespace rogue18;

static Bytes moveFixture(unsigned perm) {
    auto data=fixture();
    raw_v1::Info info{};
    if(!validate(data.data(),data.size(),info))return {};
    const size_t section=info.banks[info.active].sector[1];
    const size_t first=section+layout::PartyStart;
    auto plain=plaintext(data.data(),first);
    plain[8]=0xff;
    for(unsigned j=0;j<4;j++){
        raw_v1::w16(plain.data(),12+2*j,uint16_t(33+10*j));
        plain[20+j]=uint8_t(12+3*j);
    }
    for(unsigned i=0;i<6;i++){
        const size_t at=first+i*layout::PartyStride;
        if(i)std::memcpy(data.data()+at,data.data()+first,layout::PartyStride);
        raw_v1::w32(data.data(),at,perm+24*i);
        encode(data.data(),at,plain);
    }
    raw_v1::w32(data.data(),section+layout::PartyCount,6);
    raw_v1::w16(data.data(),section+0xff6,
                raw_v1::checksum(data.data()+section));
    return data;
}
int main(int argc,char** argv){
    using namespace rogue18::move_v3;
    static_assert(!MayWriteInstalledGame,"No installed write allowed");
    static_assert(!EmulatorGameplayCertified,"No emulator proof yet");
    CHECK(FirstCiaIds.size()==43);
    for(size_t j=0;j<FirstCiaIds.size();j++)
       for(size_t k=0;k<j;k++)CHECK(FirstCiaIds[j]!=FirstCiaIds[k]);
    CHECK(!selectedOrdinary(58)); // Ice Beam PP conflict
    CHECK(!selectedOrdinary(165)); // Struggle
    CHECK(!selectedOrdinary(172)); // Flame Wheel PP conflict
    CHECK(!selectedOrdinary(355)); // Roost PP conflict
    CHECK(!selectedOrdinary(884)); // special
    CHECK(!selectedOrdinary(0));
    CHECK(selectedOrdinary(53));
    uint8_t fakeBytes[move_v2::PackedBytes]{};
    const move_v2::Catalog forged{fakeBytes,sizeof(fakeBytes),move_v2::RomSha};
    unsigned cases=0;
    for(unsigned perm=0;perm<24;perm++){
        auto original=moveFixture(perm);
        CHECK(original.size()==raw_v1::Size);
        const auto snapshot=v2::inspect(original.data(),original.size());
        CHECK(snapshot.data.valid);
        for(unsigned party=0;party<6;party++)
        for(unsigned slot=0;slot<4;slot++){
            Selection request{};
            CHECK(begin(snapshot,original.data(),original.size(),party,slot,request));
            request.original.replacementId=89;
            CHECK(!preview(original.data(),original.size(),forged,request).ok);
            const auto test=previewSynthetic(original.data(),original.size(),
                                             request,10);
            CHECK(test.ok);
            CHECK(test.candidate.size()==original.size());
            CHECK(test.changedOffsets.size()>0 && test.changedOffsets.size()<=8);
            // Compare exact bytes with the independently implemented v2 engine.
            const auto independent=move_v2::previewSynthetic(original.data(),
                original.size(),request.original,89,10);
            CHECK(independent.ok && independent.candidate==test.candidate);
            CHECK(test.candidate!=original);
            raw_v1::Info before{},after{};
            Mon old{},newMon{};
            CHECK(validate(original.data(),original.size(),before));
            CHECK(validate(test.candidate.data(),test.candidate.size(),after));
            CHECK(partyAt(original.data(),original.size(),before,party,old));
            CHECK(partyAt(test.candidate.data(),test.candidate.size(),after,
                          party,newMon));
            CHECK(newMon.moves[slot]==89 && newMon.pp[slot]==10);
            CHECK(newMon.pid==old.pid && newMon.ot==old.ot);
            CHECK(newMon.ivs==old.ivs && newMon.evs==old.evs);
            CHECK(newMon.effectiveNature==old.effectiveNature);
            CHECK(newMon.abilitySlot==old.abilitySlot);
            CHECK(newMon.species==old.species && newMon.heldItem==old.heldItem);
            for(unsigned j=0;j<4;j++)if(j!=slot)
                CHECK(newMon.moves[j]==old.moves[j] &&
                      newMon.pp[j]==old.pp[j]);
            auto stale=original;stale.back()^=1;
            CHECK(!previewSynthetic(stale.data(),stale.size(),request,10).ok);
            auto corrupted=request;corrupted.original.source.expectedPPBonuses^=1;
            CHECK(!previewSynthetic(original.data(),original.size(),corrupted,10).ok);
            corrupted=request;corrupted.original.source.expectedCounter++;
            CHECK(!previewSynthetic(original.data(),original.size(),corrupted,10).ok);
            corrupted=request;corrupted.original.replacementId=58;
            CHECK(!previewSynthetic(original.data(),original.size(),corrupted,10).ok);
            corrupted=request;corrupted.original.replacementId=0;
            CHECK(!previewSynthetic(original.data(),original.size(),corrupted,10).ok);
            CHECK(original==moveFixture(perm)); // imported original unchanged
            ++cases;
        }
    }
    auto original=moveFixture(0);
    auto snapshot=v2::inspect(original.data(),original.size());
    Selection chosen{};
    CHECK(begin(snapshot,original.data(),original.size(),0,0,chosen));
    chosen.original.replacementId=89;
    const auto draft=previewSynthetic(original.data(),original.size(),chosen,10);
    CHECK(draft.ok);
    if(argc>1) {
        std::FILE* f=std::fopen(argv[1],"wb");
        CHECK(f!=nullptr);
        CHECK(std::fwrite(draft.candidate.data(),1,draft.candidate.size(),f)==
              draft.candidate.size());
        CHECK(std::fclose(f)==0);
    }
    for(unsigned sec=0;sec<28;sec++){
        auto invalid=original;
        invalid[sec*raw_v1::Sector+64]^=1;
        CHECK(!previewSynthetic(invalid.data(),invalid.size(),chosen,10).ok);
    }
    std::cout<<"PASS MOVE-V3: "<<cases
             <<" generated encrypted replacements (24x6x4), v2 exact"
             <<" cross-comparison, SHA guards, forbidden IDs, 28 bad sectors\n";
}
