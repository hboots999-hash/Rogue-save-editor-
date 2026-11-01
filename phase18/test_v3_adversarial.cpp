// Independent hostile-input and full-source reversibility checks for v3.
// Non-playable synthetic fixture; no ROM data or installed save access.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main phase18_v2_boundary_fixture_only
#include "test_boundaries_v2.cpp"
#undef main
#pragma GCC diagnostic pop
#undef CHECK
#include "multi_export_v3.hpp"
#include <limits>
#define CHECK(x) do{if(!(x)){std::cerr<<"V3 hostile failure "<<__LINE__<<": "<<#x<<"\n";return 1;}}while(0)
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
