// Version-2 integration QA against a generated, non-playable H168 fixture.
// Every Phase20 entrypoint is copy-only. Restoring to an installed game is
// unconditionally blocked, including an otherwise +1-shaped request.
#include "integration_v2.hpp"
#include "offline_export.hpp"
#include <fstream>
#include <iostream>
#include <limits>
using namespace rogue18;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL v2 "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
int main(int argc,char** argv){
    CHECK(argc==2);
    std::ifstream in(argv[1],std::ios::binary);
    Bytes b(std::istreambuf_iterator<char>(in),{});
    CHECK(b.size()==raw_v1::Size);
    const auto view=v2::inspect(b.data(),b.size());
    CHECK(view.version==2 && view.data.valid);
    CHECK(view.data.party.size()==2 && view.data.items.size()==3);
    for(const Kind op:{Kind::Money,Kind::ExistingQuantity,Kind::IVs,Kind::EVs,
                       Kind::Move,Kind::AbilitySlot,Kind::Nature,Kind::InsertItem}){
        const auto cap=v2::capability(op);
        CHECK(!cap.canRestoreInstalledGame);
        CHECK(v2::evidenceText(op)!=nullptr);
        CHECK(cap.support!=v2::Support::Unsupported);
        if(op==Kind::AbilitySlot || op==Kind::Nature || op==Kind::InsertItem){
            CHECK(!cap.canCreateOfflineDraft);
            Request denied{};
            CHECK(!v2::begin(view,op,0,denied));
        }
    }
    Request money{};
    CHECK(v2::begin(view,Kind::Money,0,money));
    CHECK(hasDigest(money.expectedDigest));
    CHECK(v2::legacyPreflightShape(money)==v2::LegacyPreflightShape::None);
    money.money=money.expectedMoney+1u;
    CHECK(v2::legacyPreflightShape(money)==v2::LegacyPreflightShape::MoneyPlusOne);
    CHECK(!v2::mayRestoreInstalledGame(money));
    auto out=v2::preview(b.data(),b.size(),money);
    CHECK(out.ok && out.candidate.size()==b.size() && out.changedOffsets.size()<=6);
    CHECK(v2::verify(b.data(),b.size(),out.candidate.data(),out.candidate.size(),money));
    CHECK(!v2::verify(b.data(),b.size(),out.candidate.data(),out.candidate.size()-1,money));
    CHECK(!v2::preview(b.data(),b.size()-1,money).ok);
    auto tampered=b;
    tampered.back()^=0x01; // both counter and money unchanged
    CHECK(!v2::preview(tampered.data(),tampered.size(),money).ok);
    tampered=out.candidate;
    tampered.back()^=0x80;
    CHECK(!v2::verify(b.data(),b.size(),tampered.data(),tampered.size(),money));
    Request noDigest=money;noDigest.expectedDigest={};
    CHECK(!v2::preview(b.data(),b.size(),noDigest).ok);
    CHECK(!v2::verify(b.data(),b.size(),out.candidate.data(),out.candidate.size(),noDigest));
    CHECK(!v2::begin(view,Kind::Money,1,noDigest));
    Request qty{};
    CHECK(v2::begin(view,Kind::ExistingQuantity,0,qty));
    qty.quantity=qty.expectedQuantity+1u;
    CHECK(v2::legacyPreflightShape(qty)==v2::LegacyPreflightShape::ExistingQuantityPlusOne);
    CHECK(v2::preview(b.data(),b.size(),qty).ok);
    qty.quantity=1000;CHECK(!v2::preview(b.data(),b.size(),qty).ok);
    qty.quantity=11;qty.slot=UINT32_MAX;
    CHECK(!v2::preview(b.data(),b.size(),qty).ok);

    Request iv{};
    CHECK(v2::begin(view,Kind::IVs,0,iv));
    iv.stats={31,30,29,28,27,26};
    out=v2::preview(b.data(),b.size(),iv);
    CHECK(out.ok);
    CHECK(v2::verify(b.data(),b.size(),out.candidate.data(),out.candidate.size(),iv));
    auto check=inspectReadOnly(out.candidate.data(),out.candidate.size());
    CHECK(check.valid && check.party[0].value.ivs==iv.stats);
    Request rollback{};
    CHECK(v2::begin(v2::inspect(out.candidate.data(),out.candidate.size()),
                    Kind::IVs,0,rollback));
    rollback.stats=view.data.party[0].value.ivs;
    auto reversed=v2::preview(out.candidate.data(),out.candidate.size(),rollback);
    CHECK(reversed.ok && reversed.candidate==b);
    iv.stats[0]=32;CHECK(!v2::preview(b.data(),b.size(),iv).ok);
    CHECK(!v2::begin(view,Kind::IVs,7,iv));

    Request ev{};
    CHECK(v2::begin(view,Kind::EVs,1,ev));
    ev.stats={252,0,0,0,0,252};
    CHECK(v2::preview(b.data(),b.size(),ev).ok);
    ev.stats={252,252,252,0,0,0};
    CHECK(!v2::preview(b.data(),b.size(),ev).ok);
    CHECK(!v2::begin(view,Kind::Move,9,ev));

    Request move{};
    CHECK(v2::beginMove(view,0,1,move));
    CHECK(move.moveSlot==1 && move.expectedMoveId==45);
    move.pp=24;
    CHECK(v2::preview(b.data(),b.size(),move).ok);
    move.moveId=600;
    CHECK(!v2::preview(b.data(),b.size(),move).ok);
    CHECK(!v2::beginMove(view,0,4,move));
    CHECK(!v2::beginMove(view,0,2,move)); // empty slot
    CHECK(!v2::beginMove(view,88,0,move));
    CHECK(!v2::preview(nullptr,raw_v1::Size,money).ok);
    CHECK(!v2::inspect(nullptr,raw_v1::Size).data.valid);
    CHECK(!v2::inspect(b.data(),0).data.valid);

    // An exact active/old bank checksum mismatch must fail closed.
    for(unsigned physical=0;physical<28;physical++){
        auto bad=b;
        bad[physical*raw_v1::Sector+64]^=0x5a;
        CHECK(!v2::inspect(bad.data(),bad.size()).data.valid);
    }
    CHECK(b.size()==raw_v1::Size);
    std::cout<<"PASS Phase18 v2: 8 capabilities, SHA lock, forbidden restore/ROM edits, IV/EV/move/items/money, 28 sectors, exact reversal\n";
}
