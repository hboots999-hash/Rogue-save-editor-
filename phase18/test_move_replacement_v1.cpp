// Synthetic-only move replacement regression. Zero private ROM/save data.
// Build with -DROGUE18_SYNTHETIC_MOVE_TEST; production is fail-closed.
#ifndef ROGUE18_SYNTHETIC_MOVE_TEST
#error Synthetic move fixture must never be compiled into a release
#endif
#include "move_replacement_v1.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main phase18_original_fixture_unused
#include "test_offline_engine.cpp"
#undef main
#pragma GCC diagnostic pop
#undef CHECK
#include <iostream>
#include <limits>
#define CHECK(x) do{if(!(x)){std::cerr<<"MOVE-V1 FAIL "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
using namespace rogue18;
using namespace rogue18::move_v1;
static const ResearchMove fakeMoves[]={
    {501,5,"Synthetic Ember"}, {502,20,"Synthetic Wave"},
    {503,40,"Synthetic Beam"},{504,64,"Synthetic Spark"}
};
static Bytes fullParty(unsigned permutation){
    Bytes data=fixture();
    raw_v1::Info info{};
    if(!validate(data.data(),data.size(),info))return {};
    const size_t base=info.banks[info.active].sector[1];
    const size_t first=base+layout::PartyStart;
    auto p=plaintext(data.data(),first);
    const uint16_t ids[4]={33,45,55,65};
    for(unsigned slot=0;slot<4;slot++){
        raw_v1::w16(p.data(),12+2*slot,ids[slot]);
        p[20+slot]=uint8_t(10+5*slot);
    }
    p[8]=0xff; // every occupied slot has 3 PP Ups in the fixture
    for(unsigned index=0;index<6;index++){
        const size_t at=first+index*layout::PartyStride;
        if(index)std::memcpy(data.data()+at,data.data()+first,layout::PartyStride);
        raw_v1::w32(data.data(),at,permutation+24u*index);
        encode(data.data(),at,p);
    }
    raw_v1::w32(data.data(),base+layout::PartyCount,6);
    raw_v1::w16(data.data(),base+0xff6,raw_v1::checksum(data.data()+base));
    return data;
}
int main(){
    CHECK(!capability().mayPreviewReplacement &&
          !capability().mayExportReplacement &&
          !capability().mayWriteInstalledGame);
    CHECK(!ExactRomMoveTableApproved);
    CHECK(ppUps(0xff,0)==3 && ppUps(0xff,1)==3 &&
          ppUps(0xff,2)==3 && ppUps(0xff,3)==3);
    CHECK(clearedSlotBonuses(0xff,0)==0xfc);
    CHECK(clearedSlotBonuses(0xff,1)==0xf3);
    CHECK(clearedSlotBonuses(0xff,2)==0xcf);
    CHECK(clearedSlotBonuses(0xff,3)==0x3f);
    CHECK(maxPPForResearch(5,0xff,0)==8);
    CHECK(maxPPForResearch(20,0xff,1)==32);
    CHECK(maxPPForResearch(40,0xff,2)==64);
    CHECK(maxPPForResearch(64,0xff,3)==102);
    ResearchMove resultList[12]{};
    CHECK(searchResearch(fakeMoves,4,"synthetic",0,resultList,12)==4);
    CHECK(searchResearch(fakeMoves,4,"BEAM",0,resultList,12)==1);
    CHECK(resultList[0].id==503);
    CHECK(searchResearch(fakeMoves,4,"syN",1,resultList,2)==2);
    CHECK(resultList[0].id==502 && resultList[1].id==503);
    CHECK(searchResearch(fakeMoves,4,"real-ROM-verified",0,resultList,12)==0);

    unsigned passed=0;
    for(unsigned permutation=0;permutation<24;permutation++){
        const auto original=fullParty(permutation);
        CHECK(original.size()==raw_v1::Size);
        const auto view=v2::inspect(original.data(),original.size());
        CHECK(view.data.valid && view.data.party.size()==6);
        for(unsigned party=0;party<6;party++)
        for(unsigned slot=0;slot<4;slot++){
            move_v1::Request edit{};
            CHECK(begin(view,original.data(),original.size(),party,slot,edit));
            CHECK(edit.expectedPPBonuses==0xff);
            CHECK(edit.expectedMoveId==uint16_t((slot==0?33:slot==1?45:slot==2?55:65)));
            CHECK(edit.expectedPid%24==permutation);
            CHECK(!preview(original.data(),original.size(),edit).ok);
            // Research API is never an approval to save or restore.
            CHECK(!mayWriteInstalledGame(edit));
            edit.replacementMoveId=uint16_t(501+slot);
            const auto candidate=previewResearch(original.data(),original.size(),edit,
                                                  fakeMoves,4);
            CHECK(candidate.ok && candidate.candidate.size()==original.size());
            CHECK(candidate.changedOffsets.size()>=1 &&
                  candidate.changedOffsets.size()<=8);
            CHECK(verifyResearch(original.data(),original.size(),
                     candidate.candidate.data(),candidate.candidate.size(),edit,
                     fakeMoves,4));
            raw_v1::Info check{};
            Mon before{},after{};
            CHECK(validate(candidate.candidate.data(),candidate.candidate.size(),check));
            CHECK(partyAt(original.data(),original.size(),
                  ([&](){raw_v1::Info i{};validate(original.data(),original.size(),i);return i;})(),
                  party,before));
            CHECK(partyAt(candidate.candidate.data(),candidate.candidate.size(),
                          check,party,after));
            CHECK(after.moves[slot]==uint16_t(501+slot));
            CHECK(after.pp[slot]==fakeMoves[slot].basePP);
            for(unsigned other=0;other<4;other++)if(other!=slot){
                CHECK(after.moves[other]==before.moves[other]);
                CHECK(after.pp[other]==before.pp[other]);
            }
            CHECK(after.ivs==before.ivs && after.evs==before.evs &&
                  after.species==before.species && after.pid==before.pid &&
                  after.ot==before.ot && after.heldItem==before.heldItem &&
                  after.abilitySlot==before.abilitySlot &&
                  after.effectiveNature==before.effectiveNature);
            const size_t at=check.banks[check.active].sector[1]+
                            layout::PartyStart+party*layout::PartyStride;
            const auto plain=plaintext(candidate.candidate.data(),at);
            CHECK(ppUps(plain[8],slot)==0);
            for(unsigned other=0;other<4;other++)
                if(other!=slot)CHECK(ppUps(plain[8],other)==3);
            CHECK(std::memcmp(original.data(),candidate.candidate.data(),
                              14u*raw_v1::Sector)==0);
            CHECK(std::memcmp(original.data()+28u*raw_v1::Sector,
                              candidate.candidate.data()+28u*raw_v1::Sector,
                              raw_v1::Size-28u*raw_v1::Sector)==0);
            auto bad=candidate.candidate;bad.back()^=0x40;
            CHECK(!verifyResearch(original.data(),original.size(),
                                  bad.data(),bad.size(),edit,fakeMoves,4));
            CHECK(original==fullParty(permutation)); // immutable input
            ++passed;
        }
    }
    const auto source=fullParty(0);
    const auto view=v2::inspect(source.data(),source.size());
    move_v1::Request q{};
    CHECK(begin(view,source.data(),source.size(),0,0,q));
    q.replacementMoveId=9999;
    CHECK(!previewResearch(source.data(),source.size(),q,fakeMoves,4).ok);
    q.replacementMoveId=q.expectedMoveId;
    CHECK(!previewResearch(source.data(),source.size(),q,fakeMoves,4).ok);
    q.replacementMoveId=501;
    q.expectedOriginalSha256[0]^=1;
    CHECK(!previewResearch(source.data(),source.size(),q,fakeMoves,4).ok);
    CHECK(begin(view,source.data(),source.size(),0,0,q));
    q.replacementMoveId=501;q.expectedPPBonuses^=1;
    CHECK(!previewResearch(source.data(),source.size(),q,fakeMoves,4).ok);
    CHECK(begin(view,source.data(),source.size(),0,0,q));
    q.replacementMoveId=501;q.expectedPokemonChecksum^=1;
    CHECK(!previewResearch(source.data(),source.size(),q,fakeMoves,4).ok);
    CHECK(begin(view,source.data(),source.size(),0,0,q));
    q.replacementMoveId=501;q.expectedCounter++;
    CHECK(!previewResearch(source.data(),source.size(),q,fakeMoves,4).ok);
    CHECK(begin(view,source.data(),source.size(),0,0,q));
    q.replacementMoveId=501;q.partySlot=6;
    CHECK(!previewResearch(source.data(),source.size(),q,fakeMoves,4).ok);
    q.partySlot=0;q.moveSlot=4;
    CHECK(!previewResearch(source.data(),source.size(),q,fakeMoves,4).ok);
    CHECK(!begin(view,source.data(),source.size(),6,0,q));
    CHECK(!begin(view,source.data(),source.size(),0,4,q));
    CHECK(!begin(view,source.data(),source.size()-1,0,0,q));
    CHECK(!begin(view,nullptr,source.size(),0,0,q));
    auto stale=source;stale.back()^=1;
    CHECK(!begin(view,stale.data(),stale.size(),0,0,q));
    CHECK(begin(view,source.data(),source.size(),0,0,q));
    q.replacementMoveId=501;
    CHECK(!previewResearch(stale.data(),stale.size(),q,fakeMoves,4).ok);
    // All 28 sector checksums must fail closed with no valid draft.
    for(unsigned physical=0;physical<28;physical++){
        auto broken=source;
        broken[physical*raw_v1::Sector+64]^=1;
        CHECK(!previewResearch(broken.data(),broken.size(),q,fakeMoves,4).ok);
    }
    const ResearchMove invalidBase[]={{501,0,"zero PP"}};
    CHECK(!previewResearch(source.data(),source.size(),q,invalidBase,1).ok);
    const ResearchMove invalidDup[]={{501,10,"A"},{501,20,"B"}};
    CHECK(!previewResearch(source.data(),source.size(),q,invalidDup,2).ok);
    const ResearchMove invalidBig[]={{501,65,"too large"}};
    CHECK(!previewResearch(source.data(),source.size(),q,invalidBig,1).ok);
    CHECK(!previewResearch(source.data(),source.size(),q,nullptr,0).ok);
    std::cout<<"PASS MOVE-V1 research: "<<passed
             <<" synthetic encrypted replacements (24 permutations x 6 party x 4 moves),"
             <<" PP-Up reset, 28 corrupt sectors, stale SHA, exact 8-byte whitelist,"
             <<" immutable source, search; public production preview BLOCKED\n";
}
