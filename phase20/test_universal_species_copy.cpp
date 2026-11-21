// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic-only regression: move assignments do NOT enforce species learnsets.
// This is deliberately NOT a gameplay/real-ROM compatibility certification.
#ifndef ROGUE18_TEST_MOVE_V2
#error "Synthetic preview must never be linked into an end-user CIA"
#endif
#include "phase18/move_replacement_v3.hpp"
#define main phase18_fixture_entry_unused
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "test_offline_engine.cpp"
#pragma GCC diagnostic pop
#undef main
#undef CHECK
#include <array>
#include <cstring>
#include <iostream>
#define CHECK(x) do { if(!(x)) { std::cerr << "UNIVERSAL MOVE TEST FAILED line " << __LINE__ << ": " << #x << "\n"; return 1; } } while(0)

int main() {
    using namespace rogue18;
    constexpr std::array<uint16_t,6> species{{1,25,150,676,777,1000}};
    static_assert(!move_v3::MayWriteInstalledGame, "Live GBA VC writes forbidden");
    static_assert(!move_v3::EmulatorGameplayCertified, "No gameplay certification");
    CHECK(move_v3::selectedOrdinary(89)); // Earthquake: vetted ID, regardless of species
    CHECK(!move_v3::selectedOrdinary(58)); // PP table conflict remains blocked
    unsigned cases=0;
    for(unsigned permutation=0; permutation<24; ++permutation) {
        auto original=fixture(); // generated, non-playable; never a real save
        raw_v1::Info info{};
        CHECK(validate(original.data(),original.size(),info));
        const size_t sector=info.banks[info.active].sector[1];
        const size_t first=sector+layout::PartyStart;
        const auto reference=plaintext(original.data(),first);
        for(unsigned p=0;p<species.size();++p) {
            const size_t at=first+p*layout::PartyStride;
            if(p>=2)
                std::memcpy(original.data()+at,original.data()+first,
                            layout::PartyStride);
            auto plain=reference;
            const uint32_t growth=raw_v1::r32(plain.data(),0);
            raw_v1::w32(plain.data(),0,(growth&~uint32_t(0x7ff))|species[p]);
            for(unsigned slot=0;slot<4;slot++) {
                raw_v1::w16(plain.data(),12+slot*2,uint16_t(20+slot*20));
                plain[20+slot]=uint8_t(15+slot);
            }
            // Every party entry exercises the same one of 24 encryptions.
            // Species and PID differ, allowing a strict species-independence test.
            raw_v1::w32(original.data(),at,permutation+24*p);
            encode(original.data(),at,plain);
        }
        raw_v1::w32(original.data(),sector+layout::PartyCount,
                    static_cast<uint32_t>(species.size()));
        raw_v1::w16(original.data(),sector+0xff6,
                    raw_v1::checksum(original.data()+sector));
        const auto snapshot=v2::inspect(original.data(),original.size());
        CHECK(snapshot.data.valid && snapshot.data.party.size()==species.size());
        const auto sourceSha=sourceDigest(original.data(),original.size());
        for(unsigned p=0;p<species.size();++p)
        for(unsigned slot=0;slot<4;slot++) {
            move_v3::Selection chosen{};
            CHECK(move_v3::begin(snapshot,original.data(),original.size(),
                                 p,slot,chosen));
            const uint16_t wanted=move_v3::FirstCiaIds[cases%move_v3::FirstCiaIds.size()];
            chosen.original.replacementId=wanted;
            const auto result=move_v3::previewSynthetic(
                original.data(),original.size(),chosen,10);
            CHECK(result.ok && result.candidate.size()==original.size());
            CHECK(result.changedOffsets.size()>0 &&
                  result.changedOffsets.size()<=8);
            raw_v1::Info before{},after{};
            Mon oldMon{},newMon{};
            CHECK(validate(original.data(),original.size(),before));
            CHECK(validate(result.candidate.data(),result.candidate.size(),after));
            CHECK(partyAt(original.data(),original.size(),before,p,oldMon));
            CHECK(partyAt(result.candidate.data(),result.candidate.size(),after,p,newMon));
            CHECK(oldMon.species==species[p] && newMon.species==species[p]);
            CHECK(newMon.moves[slot]==wanted && newMon.pp[slot]==10);
            CHECK(newMon.pid==oldMon.pid && newMon.ot==oldMon.ot);
            CHECK(newMon.ivs==oldMon.ivs && newMon.evs==oldMon.evs);
            CHECK(newMon.abilitySlot==oldMon.abilitySlot);
            CHECK(newMon.effectiveNature==oldMon.effectiveNature);
            for(unsigned other=0;other<4;other++)
                if(other!=slot)
                    CHECK(newMon.moves[other]==oldMon.moves[other] &&
                          newMon.pp[other]==oldMon.pp[other]);
            // The older bank and import are never changed by a preview.
            CHECK(std::memcmp(original.data(),result.candidate.data(),
                              14*raw_v1::Sector)==0);
            CHECK(sourceDigest(original.data(),original.size())==sourceSha);
            ++cases;
        }
    }
    CHECK(cases==576);
    std::cout<<"PASS: 576 synthetic replacements covering all 43 allowed move IDs, six species,"
             <<" all 24 block permutations, four occupied slots;"
             <<" no species learnset gate; no live writes or gameplay claims\n";
    return 0;
}
