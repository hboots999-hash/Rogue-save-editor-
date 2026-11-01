// Cross-product, NON-PLAYABLE fixture regression for Phase20's five v2 ops.
// No copyrighted ROM, private save, privileged archive or installed writes.
#define main phase18_fixture_main_for_matrix
#include "test_offline_engine.cpp"
#undef main
#undef CHECK
#include "integration_v2.hpp"
#include <array>
#include <climits>
#include <iostream>
#define CHECK(x) do { if(!(x)){std::cerr<<"MATRIX failure line "<<__LINE__<<" "<<#x<<"\n";return 1;} }while(0)
static bool exactOffsets(const Bytes& before,const rogue18::Result& result){
    if(!result.ok || before.size()!=result.candidate.size())return false;
    std::vector<size_t> diff;
    for(size_t i=0;i<before.size();++i)
        if(before[i]!=result.candidate[i])diff.push_back(i);
    return !diff.empty() && diff==result.changedOffsets;
}
static bool fingerprint(const Bytes& original,const Request& q){
    return hasDigest(q.expectedDigest) &&
           q.expectedDigest==sourceDigest(original.data(),original.size());
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
            // Keep fixture's known-good BoxPokemon canonical plaintext,
            // but force every possible encrypted block permutation.
            std::memcpy(b.data()+pos,b.data()+s1+layout::PartyStart,104u);
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
    size_t tested=0;
    for(unsigned permutation=0;permutation<24;permutation++){
        const auto original=sixPartyAndFullBag(permutation);
        const auto snapshot=v2::inspect(original.data(),original.size());
        CHECK(snapshot.data.valid && snapshot.data.party.size()==6);
        CHECK(snapshot.data.items.size()==450);
        for(unsigned slot=0;slot<6;slot++){
            for(Kind kind:{Kind::IVs,Kind::EVs}){
                Request req{};
                CHECK(v2::begin(snapshot,kind,slot,req));
                CHECK(fingerprint(original,req));
                req.stats=(kind==Kind::IVs)?
                    std::array<uint8_t,6>{31,0,31,0,31,0}:
                    std::array<uint8_t,6>{0,0,252,0,0,252};
                const auto out=v2::preview(original.data(),original.size(),req);
                CHECK(out.ok && exactOffsets(original,out));
                CHECK(out.candidate.size()==131072 && original!=out.candidate);
                CHECK(v2::inspect(out.candidate.data(),out.candidate.size()).data.valid);
                CHECK(!v2::mayRestoreInstalledGame(req));
                if(slot==0)CHECK(v2::verify(original.data(),original.size(),
                                out.candidate.data(),out.candidate.size(),req));
                CHECK(v2::preview(out.candidate.data(),out.candidate.size(),req).ok==false);
                req.stats=(kind==Kind::IVs)?
                    std::array<uint8_t,6>{32,0,0,0,0,0}:
                    std::array<uint8_t,6>{252,252,252,0,0,0};
                CHECK(!v2::preview(original.data(),original.size(),req).ok);
                tested++;
            }
        }
        // Four nonempty move slots, each permutation and all six party slots.
        for(unsigned slot=0;slot<6;slot++)for(unsigned ms=0;ms<4;ms++){
            Request req{};
            CHECK(v2::beginMove(snapshot,slot,ms,req));
            const auto beforePP=req.pp;
            req.pp=ms%2u?0u:64u;
            CHECK(req.pp!=beforePP);
            const auto out=v2::preview(original.data(),original.size(),req);
            CHECK(out.ok && exactOffsets(original,out));
            CHECK(v2::verify(original.data(),original.size(),
                             out.candidate.data(),out.candidate.size(),req));
            CHECK(!v2::mayRestoreInstalledGame(req));
            req.moveId=uint16_t(req.moveId+1);
            CHECK(!v2::preview(original.data(),original.size(),req).ok);
            tested++;
        }
        for(unsigned index:{0u,1u,224u,225u,448u,449u}){
            Request req{};
            CHECK(v2::begin(snapshot,Kind::ExistingQuantity,index,req));
            CHECK(fingerprint(original,req));
            req.quantity=(index%2u)?1u:999u;
            if(req.quantity==req.expectedQuantity)req.quantity=500;
            const auto out=v2::preview(original.data(),original.size(),req);
            CHECK(out.ok && exactOffsets(original,out));
            CHECK(out.changedOffsets.size()<=4);
            CHECK(v2::verify(original.data(),original.size(),
                             out.candidate.data(),out.candidate.size(),req));
            req.quantity=0;CHECK(!v2::preview(original.data(),original.size(),req).ok);
            req.quantity=1000;CHECK(!v2::preview(original.data(),original.size(),req).ok);
            tested++;
        }
        Request money{};
        CHECK(v2::begin(snapshot,Kind::Money,0,money));
        for(unsigned target:{0u,999999u}){
            money.money=target;
            const auto out=v2::preview(original.data(),original.size(),money);
            CHECK(out.ok && exactOffsets(original,out));
            CHECK(out.changedOffsets.size()<=6);
            CHECK(v2::verify(original.data(),original.size(),
                             out.candidate.data(),out.candidate.size(),money));
            tested++;
        }
        money.money=money.expectedMoney;
        CHECK(!v2::preview(original.data(),original.size(),money).ok);
        money.money=999999u;
        auto stale=original;stale.back()^=0x20;
        CHECK(!v2::preview(stale.data(),stale.size(),money).ok);
        CHECK(!v2::preview(original.data(),original.size()-1,money).ok);
        CHECK(!v2::beginMove(snapshot,6,0,money));
        CHECK(!v2::beginMove(snapshot,0,4,money));
        CHECK(!v2::begin(snapshot,Kind::ExistingQuantity,450,money));
        CHECK(!v2::begin(snapshot,Kind::IVs,UINT_MAX,money));
    }
    const auto empty=sixPartyAndFullBag(0);
    auto changed=empty;
    for(unsigned bank=0;bank<2;bank++){
        const size_t s1=(bank*14u+(1u+(bank?5u:3u))%14u)*h168::Sector;
        h168::w32(changed.data(),s1+layout::PartyCount,0);
        for(unsigned i=0;i<450;i++)
            h168::w16(changed.data(),s1+layout::BagArray+i*4,0);
        h168::w16(changed.data(),s1+0xff6,h168::checksum(changed.data()+s1));
    }
    const auto noSlots=v2::inspect(changed.data(),changed.size());
    CHECK(noSlots.data.valid && noSlots.data.party.empty() && noSlots.data.items.empty());
    Request guard{};
    CHECK(v2::begin(noSlots,Kind::Money,0,guard));
    CHECK(!v2::begin(noSlots,Kind::ExistingQuantity,0,guard));
    CHECK(!v2::begin(noSlots,Kind::IVs,0,guard));
    CHECK(!v2::beginMove(noSlots,0,0,guard));
    std::cout<<"PASS Phase18 v2 cross-product: "<<tested
             <<" bounded IV/EV/PP/bag/money proposals, 24 permutations,"
             <<" six party slots, four move slots, full 450-entry bag,"
             <<" exact footprint, stale SHA and empty slots\n";
}
