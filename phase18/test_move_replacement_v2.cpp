// Fully generated NON-PLAYABLE move-v2 safety regression.
// No real ROM bytes, move catalog, installed title or private saves.
#ifndef ROGUE18_TEST_MOVE_V2
#error "The replacement engine core is test-only until release gates pass"
#endif
#include "move_queue_v4.hpp"
#define main phase18_fixture_unused_for_move_v2
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "test_offline_engine.cpp"
#pragma GCC diagnostic pop
#undef main
#undef CHECK
#include <iostream>
#include <fstream>
#define CHECK(x) do{if(!(x)){std::cerr<<"MOVE V2 FAIL "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
using namespace rogue18;
static Bytes partyFixture(unsigned perm){
    Bytes original=fixture();
    raw_v1::Info info{};
    if(!validate(original.data(),original.size(),info))return {};
    const size_t s1=info.banks[info.active].sector[1];
    const size_t first=s1+layout::PartyStart;
    auto decrypted=plaintext(original.data(),first);
    decrypted[8]=0xff; // 3 PP-Ups per slot in synthetic fixture
    for(unsigned j=0;j<4;j++){
        raw_v1::w16(decrypted.data(),12+2*j,uint16_t(33+10*j));
        decrypted[20+j]=uint8_t(12+3*j);
    }
    for(unsigned i=0;i<6;i++){
        const size_t at=first+i*layout::PartyStride;
        if(i)std::memcpy(original.data()+at,
                         original.data()+first,layout::PartyStride);
        raw_v1::w32(original.data(),at,perm+24*i);
        encode(original.data(),at,decrypted);
    }
    raw_v1::w32(original.data(),s1+layout::PartyCount,6);
    raw_v1::w16(original.data(),s1+0xff6,
                raw_v1::checksum(original.data()+s1));
    return original;
}
int main(int argc,char** argv){
    using namespace rogue18::move_v2;
    CHECK(!ApplyApproved);
    CHECK(releaseBlockers.size()==6);
    CHECK(releaseBlockers[0]==ApplyBlocker::PpTableModeUnproven);
    CHECK(RomSha.size()==32 && CatalogSha.size()==32);
    const uint8_t dummy[PackedBytes]{};
    Catalog fake{dummy,sizeof(dummy),RomSha};
    CHECK(!authenticate(fake));
    Move move{};
    CHECK(!lookup(fake,53,move));
    CHECK(!lookup(fake,58,move));
    CHECK(!lookup(fake,165,move));
    CHECK(!lookup(fake,884,move));
    CHECK(!lookup(fake,65535,move));
    CHECK(!lookup(fake,0,move));
    CHECK(!page(fake,0,&move,1));
    CHECK(!page(fake,0,&move,13));
    CHECK(!search(fake,"FIRE",0,&move,1));
    CHECK(!search(fake,"",0,&move,13));
    Move display{};
    display.id=1;
    const uint8_t pound[13]={0xca,0xe3,0xe9,0xe2,0xd8,0xff};
    std::memcpy(display.rawName,pound,13);
    char label[14]{};
    CHECK(displayName(display,label));
    CHECK(std::strcmp(label,"Pound")==0);
    CHECK(containsName(label,"ouN"));
    CHECK(!containsName(label,"Flame"));
    CHECK(!displayName(display,nullptr));
    CHECK(!mayRestoreInstalledGame(move_v2::Request{}));

    unsigned passed=0;
    for(unsigned perm=0;perm<24;perm++){
        const auto original=partyFixture(perm);
        CHECK(original.size()==raw_v1::Size);
        const auto view=v2::inspect(original.data(),original.size());
        CHECK(view.data.valid && view.data.party.size()==6);
        for(unsigned party=0;party<6;party++)
        for(unsigned slot=0;slot<4;slot++){
            move_v2::Request q{};
            CHECK(begin(view,original.data(),original.size(),party,slot,q));
            CHECK(q.version==ContractVersion);
            q.replacementId=uint16_t(501+slot);
            CHECK(!preview(original.data(),original.size(),fake,q).ok);
            CHECK(!verify(original.data(),original.size(),
                          original.data(),original.size(),fake,q));
            const auto proposed=previewSynthetic(original.data(),original.size(),
                              q,q.replacementId,uint8_t(5+slot*5));
            CHECK(proposed.ok && proposed.candidate.size()==original.size());
            CHECK(proposed.changedOffsets.size()>=1 &&
                  proposed.changedOffsets.size()<=8);
            CHECK(verifySynthetic(original.data(),original.size(),
                  proposed.candidate.data(),proposed.candidate.size(),
                  q,q.replacementId,uint8_t(5+slot*5)));
            auto changed=proposed.candidate;
            changed.back()^=0x01;
            CHECK(!verifySynthetic(original.data(),original.size(),
                  changed.data(),changed.size(),q,q.replacementId,
                  uint8_t(5+slot*5)));
            changed=proposed.candidate;
            changed[17]^=1;
            CHECK(!verifySynthetic(original.data(),original.size(),
                  changed.data(),changed.size(),q,q.replacementId,
                  uint8_t(5+slot*5)));
            raw_v1::Info afterInfo{};
            Mon after{},before{};
            raw_v1::Info beforeInfo{};
            CHECK(validate(original.data(),original.size(),beforeInfo));
            CHECK(validate(proposed.candidate.data(),proposed.candidate.size(),afterInfo));
            CHECK(partyAt(original.data(),original.size(),beforeInfo,party,before));
            CHECK(partyAt(proposed.candidate.data(),proposed.candidate.size(),
                          afterInfo,party,after));
            CHECK(after.ivs==before.ivs && after.evs==before.evs);
            CHECK(after.species==before.species && after.pid==before.pid);
            CHECK(after.ot==before.ot && after.heldItem==before.heldItem);
            CHECK(after.effectiveNature==before.effectiveNature);
            CHECK(after.abilitySlot==before.abilitySlot);
            CHECK(after.moves[slot]==q.replacementId);
            CHECK(after.pp[slot]==uint8_t(5+slot*5));
            for(unsigned j=0;j<4;j++)if(j!=slot)
                CHECK(after.moves[j]==before.moves[j] &&
                      after.pp[j]==before.pp[j]);
            CHECK(original==partyFixture(perm));
            ++passed;
        }
    }
    auto original=partyFixture(1);
    const auto view=v2::inspect(original.data(),original.size());
    move_v2::Request request{};
    CHECK(begin(view,original.data(),original.size(),0,2,request));
    const auto good=previewSynthetic(original.data(),original.size(),request,600,15);
    CHECK(good.ok);
    // Optional generated, UNPLAYABLE test fixture for an independent
    // Python Phase9 decoder in CI, never a real save or release draft.
    if(argc>1){
        std::ofstream out(argv[1],std::ios::binary|std::ios::trunc);
        CHECK(out.good());
        out.write(reinterpret_cast<const char*>(good.candidate.data()),
                  static_cast<std::streamsize>(good.candidate.size()));
        out.close();CHECK(out.good());
    }
    CHECK(!previewSynthetic(original.data(),original.size(),request,0,15).ok);
    CHECK(!previewSynthetic(original.data(),original.size(),request,
                            request.source.expectedMoveId,15).ok);
    CHECK(!previewSynthetic(original.data(),original.size(),request,600,0).ok);
    CHECK(!previewSynthetic(original.data(),original.size(),request,600,65).ok);
    CHECK(!previewSynthetic(original.data(),original.size()-1,request,600,15).ok);
    auto stale=original;stale.back()^=0x40;
    CHECK(!previewSynthetic(stale.data(),stale.size(),request,600,15).ok);
    auto forged=request;forged.source.expectedOriginalSha256[0]^=1;
    CHECK(!previewSynthetic(original.data(),original.size(),forged,600,15).ok);
    forged=request;forged.source.expectedPokemonChecksum^=1;
    CHECK(!previewSynthetic(original.data(),original.size(),forged,600,15).ok);
    forged=request;forged.source.expectedPPBonuses^=1;
    CHECK(!previewSynthetic(original.data(),original.size(),forged,600,15).ok);
    forged=request;forged.source.expectedCounter++;
    CHECK(!previewSynthetic(original.data(),original.size(),forged,600,15).ok);
    forged=request;forged.source.partySlot=6;
    CHECK(!previewSynthetic(original.data(),original.size(),forged,600,15).ok);
    forged=request;forged.source.moveSlot=4;
    CHECK(!previewSynthetic(original.data(),original.size(),forged,600,15).ok);
    // Duplicate another occupied move is disallowed.
    CHECK(!previewSynthetic(original.data(),original.size(),
                            request,original.size()>0?43:0,15).ok);
    for(unsigned sec=0;sec<28;sec++){
        auto corrupt=original;
        corrupt[sec*raw_v1::Sector+64]^=1;
        CHECK(!previewSynthetic(corrupt.data(),corrupt.size(),
                                request,600,15).ok);
    }
    // Phase20 PP-only queue path still available and unchanged.
    move_queue_v4::Queue queue{};
    CHECK(move_queue_v4::begin(view,queue));
    CHECK(move_queue_v4::appendScalar(view,queue,Kind::Money,0,0,123456));
    CHECK(queue.replacements.empty());
    auto scalar=move_queue_v4::preview(original.data(),original.size(),queue);
    CHECK(scalar.ok && scalar.candidate.size()==original.size());
    CHECK(v3::verify(original.data(),original.size(),scalar.candidate.data(),
                     scalar.candidate.size(),queue.scalars));
    CHECK(!move_queue_v4::appendReplacement(view,original.data(),original.size(),
                 fake,queue,0,2,600)); // fake ROM catalogue can't authorize
    CHECK(queue.replacements.empty());
    CHECK(!move_queue_v4::mayWriteInstalledGame(queue));
    // If a future coordinator fills a queue without authentication, Apply
    // still fails closed even though the legacy scalar batch was valid.
    queue.replacements.push_back(request);
    CHECK(!move_queue_v4::preview(original.data(),original.size(),queue).ok);
    CHECK(!move_queue_v4::appendScalar(view,queue,Kind::Move,0,2,0));
    CHECK(!move_queue_v4::appendScalar(view,queue,Kind::Move,0,0,1));
    CHECK(!move_queue_v4::appendScalar(view,queue,Kind::IVs,0,0,31));
    CHECK(!move_queue_v4::appendScalar(view,queue,Kind::EVs,0,0,31));
    CHECK(!move_queue_v4::verifyStaging(original.data(),original.size(),
                                        fake,queue));
    CHECK(!move_queue_v4::verifyStaging(original.data(),original.size(),
                                        fake,move_queue_v4::Queue{}));
    CHECK(!move_queue_v4::verifyStaging(nullptr,original.size(),
                                        fake,queue));
    CHECK(!move_queue_v4::verifyStaging(original.data(),original.size()-1,
                                        fake,queue));
    CHECK(!move_queue_v4::appendReplacement(view,original.data(),original.size(),
                 fake,queue,0,0,603)); // any second move of same mon blocked

    std::cout<<"PASS move-v2: "<<passed
             <<" synthetically regenerated offline replacements (24x6x4),"
             <<" exact 8-byte proof, 28 corrupt sectors, stale guards,"
             <<" authenticated-catalog refusal and v4 queued Apply lock\n";
}
