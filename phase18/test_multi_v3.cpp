// Phase18 v3 all-or-nothing synthetic 128-KiB copied-save transactions.
// Fixture is GENERATED and unplayable. Never uses a private ROM or live save.
#include "multi_export_v3.hpp"
#define main phase18_single_fixture_main_unreferenced
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "test_offline_engine.cpp"
#pragma GCC diagnostic pop
#undef main
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
#undef CHECK
#define CHECK(x) do{if(!(x)){std::cerr<<"V3 FAIL "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)

static Bytes expanded(const Bytes& source,unsigned permutation){
    Bytes b=source;h168::Info info{};
    if(!validate(b.data(),b.size(),info))return {};
    const size_t s1=info.banks[info.active].sector[1];
    const size_t first=s1+layout::PartyStart;
    auto mon=plaintext(source.data(),first);
    for(unsigned m=0;m<4;m++){
        h168::w16(mon.data(),12+2*m,uint16_t(33+15*m));
        mon[20+m]=uint8_t(12+m);
    }
    for(unsigned p=0;p<6;p++){
        const size_t at=first+p*layout::PartyStride;
        if(p)std::memcpy(b.data()+at,source.data()+first,layout::PartyStride);
        h168::w32(b.data(),at,permutation+24u*p);
        encode(b.data(),at,mon);
    }
    h168::w32(b.data(),s1+layout::PartyCount,6);
    h168::w16(b.data(),s1+0xff6,h168::checksum(b.data()+s1));
    return b;
}
static Bytes read(const fs::path& path){
    std::ifstream in(path,std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in),{});
}
static void write(const fs::path& path,const Bytes& b){
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    out.write(reinterpret_cast<const char*>(b.data()),b.size());
    out.close();
}
int main(){
    const Bytes original=fixture();
    unsigned cases=0;
    for(unsigned perm=0;perm<24;perm++){
        const auto b=expanded(original,perm);
        const auto snapshot=v2::inspect(b.data(),b.size());
        CHECK(snapshot.data.valid && snapshot.data.party.size()==6);
        v3::Batch small{};
        CHECK(v3::start(snapshot,small));
        for(unsigned p=0;p<6;p++){
            CHECK(v3::append(snapshot,small,Kind::IVs,p,p%6,31));
            CHECK(v3::append(snapshot,small,Kind::Move,p,p%4,0));
        }
        CHECK(v3::append(snapshot,small,Kind::Money,0,0,999999));
        auto result=v3::preview(b.data(),b.size(),small);
        CHECK(result.ok && v3::verify(b.data(),b.size(),
              result.candidate.data(),result.candidate.size(),small));
        CHECK(std::memcmp(result.candidate.data(),b.data(),14*h168::Sector)==0);
        CHECK(std::memcmp(result.candidate.data()+28*h168::Sector,
              b.data()+28*h168::Sector,raw_v1::Size-28*h168::Sector)==0);
        CHECK(b==expanded(original,perm));
        const auto after=v2::inspect(result.candidate.data(),result.candidate.size());
        CHECK(after.data.valid && after.data.money==999999);
        for(unsigned p=0;p<6;p++){
            CHECK(after.data.party[p].value.ivs[p%6]==31);
            CHECK(after.data.party[p].value.pp[p%4]==0);
            CHECK(after.data.party[p].value.moves==snapshot.data.party[p].value.moves);
            CHECK(after.data.party[p].value.pid==snapshot.data.party[p].value.pid);
        }
        // stale copy retaining valid bank counters / other protected fields
        auto stale=b;stale.back()^=1;
        CHECK(!v3::preview(stale.data(),stale.size(),small).ok);
        stale=result.candidate;stale.back()^=1;
        CHECK(!v3::verify(b.data(),b.size(),stale.data(),stale.size(),small));
        ++cases;
    }
    const auto b=expanded(original,7);
    const auto snapshot=v2::inspect(b.data(),b.size());
    CHECK(snapshot.data.valid);
    v3::Batch all{};
    CHECK(v3::start(snapshot,all));
    // Deliberately append EV increases BEFORE EV decreases. Preview must
    // reorder to keep EVERY intermediate per-Pokemon EV total <=510.
    for(unsigned p=0;p<6;p++){
        for(unsigned stat: {0u,2u,4u,5u})
            CHECK(v3::append(snapshot,all,Kind::EVs,p,stat,1));
        for(unsigned stat: {1u,3u})
            CHECK(v3::append(snapshot,all,Kind::EVs,p,stat,251));
        for(unsigned stat=0;stat<6;stat++)
            CHECK(v3::append(snapshot,all,Kind::IVs,p,stat,31));
        for(unsigned move=0;move<4;move++)
            CHECK(v3::append(snapshot,all,Kind::Move,p,move,0));
    }
    CHECK(v3::append(snapshot,all,Kind::Money,0,0,0));
    for(unsigned item=0;item<3;item++)
        CHECK(v3::append(snapshot,all,Kind::ExistingQuantity,item,0,999));
    CHECK(all.edits.size()==100);
    CHECK(!v3::append(snapshot,all,Kind::Money,0,0,1)); // conflict
    CHECK(!v3::append(snapshot,all,Kind::Move,0,4,0));
    CHECK(!v3::append(snapshot,all,Kind::IVs,6,0,31));
    CHECK(!v3::append(snapshot,all,Kind::InsertItem,0,0,1));
    CHECK(!v3::append(snapshot,all,Kind::AbilitySlot,0,0,1));
    CHECK(!v3::append(snapshot,all,Kind::Nature,0,0,1));
    CHECK(!v3::append(snapshot,all,Kind::Money,0,0,1000000));
    CHECK(!v3::append(snapshot,all,Kind::ExistingQuantity,3,0,50));
    auto combined=v3::preview(b.data(),b.size(),all);
    CHECK(combined.ok && combined.candidate!=b);
    CHECK(v3::verify(b.data(),b.size(),combined.candidate.data(),
                     combined.candidate.size(),all));
    const auto readBack=v2::inspect(combined.candidate.data(),combined.candidate.size());
    CHECK(readBack.data.valid && readBack.data.money==0);
    for(unsigned p=0;p<6;p++){
        unsigned total=0;
        for(unsigned i=0;i<6;i++){
            CHECK(readBack.data.party[p].value.ivs[i]==31);
            total+=readBack.data.party[p].value.evs[i];
        }
        CHECK(total==510);
        for(unsigned i=0;i<4;i++)CHECK(readBack.data.party[p].value.pp[i]==0);
    }
    for(const auto& item:readBack.data.items)CHECK(item.quantity==999);
    CHECK(combined.changedOffsets.size()<=100*4+6*2+2);
    CHECK(std::memcmp(combined.candidate.data(),b.data(),14*h168::Sector)==0);
    CHECK(std::memcmp(combined.candidate.data()+28*h168::Sector,
                      b.data()+28*h168::Sector,raw_v1::Size-28*h168::Sector)==0);
    // Alter the full source identity or any guarded original-field identity.
    auto bad=all;
    bad.expectedDigest[0]^=1;
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.edits[0].expectedDigest[0]^=1;
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.edits[0].expectedPid++;
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.edits.back().expectedItemId++;
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.edits.push_back(bad.edits.front());
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.edits.front().value=bad.edits.front().expectedValue;
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.edits[0].value=252; // EV final over budget
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.version=2;
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.expectedCounter--;
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;bad.edits.clear();
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    bad=all;while(bad.edits.size()<=v3::MaxEdits)bad.edits.push_back(bad.edits.front());
    CHECK(!v3::preview(b.data(),b.size(),bad).ok);
    CHECK(!v3::preview(nullptr,b.size(),all).ok);
    CHECK(!v3::preview(b.data(),b.size()-1,all).ok);
    CHECK(!v3::verify(b.data(),b.size(),combined.candidate.data(),
                      combined.candidate.size()-1,all));
    auto tampered=combined.candidate;
    tampered[raw_v1::Size-1]^=0x80;
    CHECK(!v3::verify(b.data(),b.size(),tampered.data(),tampered.size(),all));
    // Single immutable backup + single separate combined draft. Repeated
    // exports never overwrite the import, original backups or earlier drafts.
    const fs::path root=fs::temp_directory_path()/"pksm_v3_batch_synthetic";
    fs::remove_all(root);fs::create_directories(root/"imports");
    const auto sourcePath=root/"imports/00000001.sav";
    write(sourcePath,b);
    auto first=v3::exportCopiedBatch(b.data(),b.size(),
                sourcePath.c_str(),root.c_str(),all);
    CHECK(first.ok && first.changed==combined.changedOffsets.size());
    CHECK(read(sourcePath)==b);
    CHECK(read(root/"backups"/first.backupName)==b);
    CHECK(read(root/"drafts"/first.draftName)==combined.candidate);
    auto second=v3::exportCopiedBatch(b.data(),b.size(),
                 sourcePath.c_str(),root.c_str(),all);
    CHECK(second.ok && first.backupName!=second.backupName &&
          first.draftName!=second.draftName);
    CHECK(read(root/"backups"/first.backupName)==b);
    CHECK(read(root/"drafts"/first.draftName)==combined.candidate);
    CHECK(read(sourcePath)==b);
    auto different=b;different.back()^=1;write(sourcePath,different);
    CHECK(!v3::exportCopiedBatch(b.data(),b.size(),
           sourcePath.c_str(),root.c_str(),all).ok);
    CHECK(read(sourcePath)==different);
    fs::remove_all(root);
    std::cout<<"PASS v3 24 permutations x 6 party; 100-edit atomic batch, EV transfer,"
             <<" 2 immutable exports, guards/invalid/sha/byte proof\n";
}
