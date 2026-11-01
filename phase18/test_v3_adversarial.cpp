// Independent hostile-input and full-source reversibility checks for v3.
// Non-playable synthetic fixture; no ROM data or installed save access.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main phase18_fixture_v3_test_helper
#include "test_offline_engine.cpp"
#undef main
#pragma GCC diagnostic pop
#undef CHECK
#include "multi_export_v3.hpp"
#include <limits>
#define CHECK(x) do{if(!(x)){std::cerr<<"V3 hostile failure "<<__LINE__<<": "<<#x<<"\n";return 1;}}while(0)
static bool exactOffsets(const Bytes& original,const Result& result){
    if(!result.ok || original.size()!=result.candidate.size())return false;
    std::vector<size_t> changed;
    for(size_t i=0;i<original.size();i++)
        if(original[i]!=result.candidate[i])changed.push_back(i);
    return !changed.empty() && changed==result.changedOffsets;
}
static Bytes sixPartyAndFullBag(unsigned permutation){
    Bytes b=fixture();
    for(unsigned bank=0;bank<2;bank++){
        const unsigned rotation=bank?5u:3u;
        const size_t s1=(bank*14u+(1u+rotation)%14u)*h168::Sector;
        h168::w32(b.data(),s1+layout::PartyCount,6);
        const auto growth=plaintext(b.data(),s1+layout::PartyStart);
        for(unsigned slot=0;slot<6;slot++){
            const size_t pos=s1+layout::PartyStart+slot*104u;
            if(slot)std::memcpy(b.data()+pos,b.data()+s1+layout::PartyStart,104u);
            h168::w32(b.data(),pos,permutation+slot*24u);
            auto p=growth;
            for(unsigned i=0;i<4;i++){
                h168::w16(p.data(),12+i*2,uint16_t(33u+i*12u));
                p[20+i]=uint8_t(10+i*10);
            }
            encode(b.data(),pos,p);
        }
        for(unsigned i=0;i<450;i++){
            const auto off=s1+layout::BagArray+i*4u;
            h168::w16(b.data(),off,uint16_t(100+i));
            h168::w16(b.data(),off+2,uint16_t((i%998u+1u)^
                                       uint16_t(bank?0xf00dbabeu:0xa5a5c33cu)));
        }
        h168::w16(b.data(),s1+0xff6,h168::checksum(b.data()+s1));
    }
    return b;
}
int main(){
    using namespace rogue18;
    using namespace rogue18::v3;
    unsigned tested=0;
    for(unsigned permutation=0;permutation<24;permutation++){
        const Bytes original=sixPartyAndFullBag(permutation);
        auto snapshot=v2::inspect(original.data(),original.size());
        CHECK(snapshot.data.valid);
        Batch batch{};
        CHECK(start(snapshot,batch));
        CHECK(append(snapshot,batch,Kind::Money,0,0,0));
        CHECK(append(snapshot,batch,Kind::ExistingQuantity,449,0,999));
        CHECK(append(snapshot,batch,Kind::ExistingQuantity,225,0,333));
        for(unsigned party=0;party<6;party++){
            CHECK(append(snapshot,batch,Kind::IVs,party,party%6,31));
            CHECK(append(snapshot,batch,Kind::EVs,party,1,0));
            CHECK(append(snapshot,batch,Kind::Move,party,party%4,64));
        }
        CHECK(batch.edits.size()==21);
        CHECK(!append(snapshot,batch,Kind::Money,0,0,999999)); // dup
        CHECK(!append(snapshot,batch,Kind::IVs,0,0,0)); // dup
        CHECK(!append(snapshot,batch,Kind::Nature,0,0,1));
        CHECK(!append(snapshot,batch,Kind::AbilitySlot,0,0,1));
        CHECK(!append(snapshot,batch,Kind::InsertItem,450,0,1));
        CHECK(!append(snapshot,batch,Kind::Move,0,4,1));
        CHECK(!append(snapshot,batch,Kind::IVs,6,0,12));
        CHECK(!append(snapshot,batch,Kind::ExistingQuantity,450,0,12));
        auto combined=preview(original.data(),original.size(),batch);
        CHECK(combined.ok);
        CHECK(verify(original.data(),original.size(),
                     combined.candidate.data(),combined.candidate.size(),batch));
        CHECK(exactOffsets(original,combined));
        CHECK(!mayWriteInstalledGame(batch));
        auto tampered=combined.candidate;
        tampered.back()^=1; // outside authenticated 28 sectors
        CHECK(!verify(original.data(),original.size(),
                      tampered.data(),tampered.size(),batch));
        tampered=original;
        tampered.back()^=1;
        CHECK(!preview(tampered.data(),tampered.size(),batch).ok);
        CHECK(!preview(original.data(),original.size()-1,batch).ok);
        CHECK(!verify(original.data(),original.size(),combined.candidate.data(),
                      combined.candidate.size()-1,batch));
        Batch forged=batch;
        forged.edits[0].expectedDigest[0]^=1;
        CHECK(!preview(original.data(),original.size(),forged).ok);
        forged=batch;
        forged.edits[3].expectedPokemonChecksum^=1;
        CHECK(!preview(original.data(),original.size(),forged).ok);
        forged=batch;
        forged.edits[1].expectedItemId^=1;
        CHECK(!preview(original.data(),original.size(),forged).ok);
        forged=batch;
        forged.edits[0].kind=Kind::InsertItem;
        CHECK(!preview(original.data(),original.size(),forged).ok);
        forged=batch;
        forged.edits.push_back(forged.edits[0]);
        CHECK(!preview(original.data(),original.size(),forged).ok);
        forged=batch;
        forged.expectedCounter++;
        CHECK(!preview(original.data(),original.size(),forged).ok);
        forged=batch;
        forged.expectedDigest[1]^=1;
        CHECK(!preview(original.data(),original.size(),forged).ok);
        CHECK(v2::inspect(combined.candidate.data(),combined.candidate.size()).data.valid);
        CHECK(original==sixPartyAndFullBag(permutation));
        tested++;
    }
    // Long batch tests both fields of each Pokémon and disparate bag entries;
    // deliberate per-step v2 validation must leave a single final candidate.
    {
        const Bytes original=sixPartyAndFullBag(0);
        const auto snapshot=v2::inspect(original.data(),original.size());
        Batch batch{};CHECK(start(snapshot,batch));
        for(unsigned i=0;i<60;i++){
            const auto old=snapshot.data.items[i].quantity;
            const unsigned desired=old==999?998:999;
            CHECK(append(snapshot,batch,Kind::ExistingQuantity,i,0,desired));
        }
        CHECK(append(snapshot,batch,Kind::Money,0,0,999999));
        CHECK(append(snapshot,batch,Kind::Move,5,3,0));
        const auto combined=preview(original.data(),original.size(),batch);
        CHECK(combined.ok && exactOffsets(original,combined));
        CHECK(verify(original.data(),original.size(),
                     combined.candidate.data(),combined.candidate.size(),batch));
        CHECK(original==sixPartyAndFullBag(0));
    }
    std::cout<<"PASS Phase18 independent v3 hostile-input replay: "<<tested
             <<" permutations, six party slots, 21 edits/batch, forged locks, exact diff, 62-edit batch\n";
}
