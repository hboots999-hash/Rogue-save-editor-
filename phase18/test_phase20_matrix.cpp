// Synthetic-only Phase20 v2 coverage: 24 encrypted orders x all six party
// positions x all four existing move slots, all occupied bag entries.
// No private user saves, ROM resources, restored game or live writer.
#define main phase18_original_fixture_main_not_called
#include "test_offline_engine.cpp"
#undef main
#include <limits>
using namespace rogue18;
static int failMatrix(unsigned line,const char* expression) {
    std::cerr<<"PHASE20 MATRIX FAIL line "<<line<<": "<<expression<<"\n";
    return 1;
}
#undef CHECK
#define CHECK(x) do{if(!(x))return failMatrix(__LINE__,#x);}while(0)

static Bytes sixPartyVariant(const Bytes& original,unsigned permutation){
    Bytes b=original;
    h168::Info info{};
    if(!validate(b.data(),b.size(),info))return {};
    const size_t s1=info.banks[info.active].sector[1];
    const size_t src=s1+layout::PartyStart;
    const auto rawMon=plaintext(original.data(),src);
    auto packed=rawMon;
    const uint16_t moves[4]={33,45,50,60};
    const uint8_t pps[4]={15,25,5,1};
    for(unsigned i=0;i<4;i++){
        h168::w16(packed.data(),12+2*i,moves[i]);
        packed[20+i]=pps[i];
    }
    // Copy only the 104-byte party record, preserve original 80-byte
    // encryption header extension and all unrelated bytes.
    for(unsigned slot=0;slot<6;slot++){
        const size_t at=src+slot*layout::PartyStride;
        std::memcpy(b.data()+at,original.data()+src,layout::PartyStride);
        // All party slots use the requested one of 24 GenIII permutations,
        // distinct PIDs prevent accidental reference to a single mon.
        const uint32_t pid=permutation+24u*slot;
        h168::w32(b.data(),at,pid);
        encode(b.data(),at,packed);
    }
    h168::w32(b.data(),s1+layout::PartyCount,6);
    h168::w16(b.data(),s1+0xff6,h168::checksum(b.data()+s1));
    return b;
}

int main(){
    const Bytes source=fixture();
    unsigned permutations=0, partyTests=0, moveTests=0, quantityTests=0;
    for(unsigned order=0;order<24;order++){
        const auto b=sixPartyVariant(source,order);
        CHECK(b.size()==raw_v1::Size);
        const auto view=v2::inspect(b.data(),b.size());
        CHECK(view.data.valid && view.data.party.size()==6 &&
              view.data.items.size()==3 && view.data.activeCounter==41);
        CHECK(view.data.sourceSha256==sourceDigest(b.data(),b.size()));
        const auto s1=raw_v1::Info{}; (void)s1;
        for(unsigned party=0;party<6;party++){
            const auto& original=view.data.party[party].value;
            CHECK(original.pid%24==order && original.moves[3]==60);
            // Each of six slots: all IV entries, a bounded EV entry,
            // and all four independent existing PP slots.
            for(unsigned field=0;field<6;field++){
                Request iv{};
                CHECK(v2::begin(view,Kind::IVs,party,iv));
                const auto initial=iv.stats[field];
                iv.stats[field]=initial?0:31;
                auto edited=v2::preview(b.data(),b.size(),iv);
                CHECK(edited.ok && edited.candidate.size()==b.size());
                CHECK(v2::verify(b.data(),b.size(),edited.candidate.data(),
                                 edited.candidate.size(),iv));
                auto backView=v2::inspect(edited.candidate.data(),edited.candidate.size());
                CHECK(backView.data.valid && backView.data.party[party].value.ivs[field]==iv.stats[field]);
                CHECK(backView.data.party[party].value.evs==original.evs);
                CHECK(backView.data.party[party].value.moves==original.moves);
                // Cryptographic lock rejects changed trailing bytes even if
                // both full H168 bank checksums and counters remain valid.
                if(field==0){
                    auto stale=b;stale.back()^=0x40;
                    CHECK(!v2::preview(stale.data(),stale.size(),iv).ok);
                    Request reverse{};
                    CHECK(v2::begin(backView,Kind::IVs,party,reverse));
                    reverse.stats=original.ivs;
                    auto rollback=v2::preview(edited.candidate.data(),
                                              edited.candidate.size(),reverse);
                    CHECK(rollback.ok && rollback.candidate==b);
                }
                Request noChange{};
                CHECK(v2::begin(view,Kind::IVs,party,noChange));
                CHECK(!v2::preview(b.data(),b.size(),noChange).ok);
                ++partyTests;
            }
            for(unsigned field=0;field<6;field++){
                Request ev{};
                CHECK(v2::begin(view,Kind::EVs,party,ev));
                const auto initial=ev.stats[field];
                ev.stats[field]=initial?0:1; // total 508 -> <=509
                const auto edited=v2::preview(b.data(),b.size(),ev);
                CHECK(edited.ok);
                CHECK(v2::verify(b.data(),b.size(),edited.candidate.data(),
                                 edited.candidate.size(),ev));
                const auto backView=v2::inspect(edited.candidate.data(),edited.candidate.size());
                CHECK(backView.data.valid && backView.data.party[party].value.evs[field]==ev.stats[field]);
                CHECK(backView.data.party[party].value.ivs==original.ivs);
                ++partyTests;
            }
            for(unsigned move=0;move<4;move++){
                Request pp{};
                CHECK(v2::beginMove(view,party,move,pp));
                CHECK(pp.expectedMoveId==original.moves[move]);
                pp.pp=0; // all original PP nonzero; lower bound
                auto edited=v2::preview(b.data(),b.size(),pp);
                CHECK(edited.ok);
                CHECK(v2::verify(b.data(),b.size(),edited.candidate.data(),
                                 edited.candidate.size(),pp));
                const auto backView=v2::inspect(edited.candidate.data(),edited.candidate.size());
                CHECK(backView.data.valid &&
                      backView.data.party[party].value.pp[move]==0 &&
                      backView.data.party[party].value.moves==original.moves);
                pp.pp=64; // conservative global upper bound (not move-specific)
                CHECK(v2::preview(b.data(),b.size(),pp).ok);
                pp.pp=65;
                CHECK(!v2::preview(b.data(),b.size(),pp).ok);
                pp.pp=0;pp.moveId=1;
                CHECK(!v2::preview(b.data(),b.size(),pp).ok);
                ++moveTests;
            }
            Request invalid{};
            CHECK(!v2::beginMove(view,party,4,invalid));
        }
        for(unsigned bag=0;bag<view.data.items.size();bag++){
            Request q{};
            CHECK(v2::begin(view,Kind::ExistingQuantity,bag,q));
            const auto old=q.quantity;
            q.quantity=999;
            const auto edited=v2::preview(b.data(),b.size(),q);
            CHECK(edited.ok);
            CHECK(v2::verify(b.data(),b.size(),edited.candidate.data(),
                             edited.candidate.size(),q));
            auto backView=v2::inspect(edited.candidate.data(),edited.candidate.size());
            CHECK(backView.data.valid && backView.data.items[bag].quantity==999);
            q.quantity=1;
            CHECK(v2::preview(b.data(),b.size(),q).ok);
            q.quantity=0;
            CHECK(!v2::preview(b.data(),b.size(),q).ok);
            q.quantity=1000;
            CHECK(!v2::preview(b.data(),b.size(),q).ok);
            q.quantity=old;
            CHECK(!v2::preview(b.data(),b.size(),q).ok);
            ++quantityTests;
        }
        Request money{};
        CHECK(v2::begin(view,Kind::Money,0,money));
        money.money=0;
        auto empty=v2::preview(b.data(),b.size(),money);
        CHECK(empty.ok && v2::verify(b.data(),b.size(),
              empty.candidate.data(),empty.candidate.size(),money));
        money.money=999999;
        CHECK(v2::preview(b.data(),b.size(),money).ok);
        money.money=1000000;
        CHECK(!v2::preview(b.data(),b.size(),money).ok);
        money.money=money.expectedMoney;
        CHECK(!v2::preview(b.data(),b.size(),money).ok);
        ++permutations;
    }
    const auto view=v2::inspect(source.data(),source.size());
    Request missing{};
    CHECK(!v2::begin(view,Kind::IVs,6,missing));
    CHECK(!v2::begin(view,Kind::ExistingQuantity,3,missing));
    CHECK(!v2::beginMove(view,0,std::numeric_limits<unsigned>::max(),missing));
    CHECK(!v2::beginMove(view,std::numeric_limits<unsigned>::max(),0,missing));
    CHECK(!v2::begin(view,Kind::Nature,0,missing));
    CHECK(!v2::begin(view,Kind::AbilitySlot,0,missing));
    CHECK(!v2::begin(view,Kind::InsertItem,3,missing));
    std::cout<<"PASS Phase20 synthetic matrix: "<<permutations<<" permutations, "
             <<partyTests<<" IV/EV fields, "<<moveTests<<" PP slots, "
             <<quantityTests<<" bag entries, money bounds/SHA/rollback\n";
}
