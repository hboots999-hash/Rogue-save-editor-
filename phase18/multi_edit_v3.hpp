// Phase18 v3: one RAM-only draft containing independently guarded scalar edits.
// Consumes ONLY copied H168 raw buffers. No SD/archive/container/restore code.
// v2 single-edit functions and signatures remain unchanged.
#pragma once
#include "integration_v2.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace rogue18 { namespace v3 {
constexpr uint32_t ContractVersion=3;
// A bounded offline batch: 128 scalar changes maximum until real 3DS heap
// and latency measurements justify larger transactions.
constexpr size_t MaxEdits=128;
struct Edit {
    Kind kind=Kind::Money;
    unsigned slot=0,field=0;
    uint32_t expectedValue=0,value=0;
    uint16_t expectedItemId=0,expectedMoveId=0;
    uint32_t expectedPid=0,expectedOt=0;
    uint16_t expectedSpecies=0,expectedPokemonChecksum=0;
    uint32_t expectedCounter=0;
    Digest256 expectedDigest{};
};
struct Batch {
    uint32_t version=ContractVersion;
    Digest256 expectedDigest{};
    uint32_t expectedCounter=0;
    std::vector<Edit> edits{};
};
inline bool allowedKind(Kind k){
    return k==Kind::Money || k==Kind::ExistingQuantity ||
           k==Kind::IVs || k==Kind::EVs || k==Kind::Move;
}
inline bool fieldValid(Kind kind,unsigned field){
    return (kind==Kind::Money || kind==Kind::ExistingQuantity)?field==0:
           (kind==Kind::IVs || kind==Kind::EVs)?field<6:
           kind==Kind::Move?field<4:false;
}
inline bool inRange(Kind kind,uint32_t n){
    return kind==Kind::Money?n<=999999u:
           kind==Kind::ExistingQuantity?n>=1u && n<=999u:
           kind==Kind::IVs?n<=31u:
           kind==Kind::EVs?n<=252u:
           kind==Kind::Move?n<=64u:false;
}
inline bool sameField(const Edit& a,const Edit& b){
    return a.kind==b.kind && a.slot==b.slot && a.field==b.field;
}
inline bool start(const v2::ReadSnapshot& snapshot,Batch& result){
    result=Batch{};
    if(snapshot.version!=v2::ContractVersion || !snapshot.data.valid ||
       !hasDigest(snapshot.data.sourceSha256))return false;
    result.expectedDigest=snapshot.data.sourceSha256;
    result.expectedCounter=snapshot.data.activeCounter;
    return true;
}
inline bool makeEdit(const v2::ReadSnapshot& original,const Batch& batch,
                     Kind kind,unsigned slot,unsigned field,uint32_t value,Edit& out){
    out=Edit{};
    if(batch.version!=ContractVersion || !original.data.valid ||
       original.version!=v2::ContractVersion || !allowedKind(kind) ||
       !fieldValid(kind,field) || !inRange(kind,value) ||
       !hasDigest(batch.expectedDigest) ||
       original.data.sourceSha256!=batch.expectedDigest ||
       original.data.activeCounter!=batch.expectedCounter)return false;
    Request guard{};
    if(kind==Kind::Move){
        if(!v2::beginMove(original,slot,field,guard))return false;
    }else if(!v2::begin(original,kind,slot,guard))return false;
    out.kind=kind;out.slot=slot;out.field=field;out.value=value;
    out.expectedDigest=guard.expectedDigest;
    out.expectedCounter=guard.expectedCounter;
    out.expectedItemId=guard.expectedItemId;
    out.expectedMoveId=guard.expectedMoveId;
    out.expectedPid=guard.expectedPid;out.expectedOt=guard.expectedOt;
    out.expectedSpecies=guard.expectedSpecies;
    out.expectedPokemonChecksum=guard.expectedPokemonChecksum;
    if(kind==Kind::Money)out.expectedValue=guard.expectedMoney;
    else if(kind==Kind::ExistingQuantity)out.expectedValue=guard.expectedQuantity;
    else if(kind==Kind::IVs)out.expectedValue=guard.stats[field];
    else if(kind==Kind::EVs)out.expectedValue=guard.stats[field];
    else out.expectedValue=guard.pp;
    return out.expectedValue!=out.value;
}
inline bool append(const v2::ReadSnapshot& original,Batch& batch,
                   Kind kind,unsigned slot,unsigned field,uint32_t value){
    if(batch.edits.size()>=MaxEdits)return false;
    Edit e{};
    if(!makeEdit(original,batch,kind,slot,field,value,e))return false;
    for(const auto& previous:batch.edits)if(sameField(previous,e))return false;
    batch.edits.push_back(e);
    return true;
}
inline uint32_t currentValue(const v2::ReadSnapshot& view,const Edit& e){
    if(e.kind==Kind::Money)return view.data.money;
    if(e.kind==Kind::ExistingQuantity)return view.data.items[e.slot].quantity;
    const auto& mon=view.data.party[e.slot].value;
    if(e.kind==Kind::IVs)return mon.ivs[e.field];
    if(e.kind==Kind::EVs)return mon.evs[e.field];
    return mon.pp[e.field];
}
inline bool originalGuard(const v2::ReadSnapshot& view,const Batch& batch,const Edit& e){
    if(!allowedKind(e.kind) || !fieldValid(e.kind,e.field) ||
       !inRange(e.kind,e.value) || e.expectedValue==e.value ||
       e.expectedCounter!=batch.expectedCounter ||
       e.expectedDigest!=batch.expectedDigest)return false;
    if(e.kind==Kind::Money)return e.slot==0 && e.expectedValue==view.data.money;
    if(e.kind==Kind::ExistingQuantity){
        return e.slot<view.data.items.size() &&
               view.data.items[e.slot].slot==e.slot &&
               view.data.items[e.slot].itemId==e.expectedItemId &&
               e.expectedItemId!=0 &&
               view.data.items[e.slot].quantity==e.expectedValue;
    }
    if(e.slot>=view.data.party.size() ||
       view.data.party[e.slot].slot!=e.slot)return false;
    const Mon& mon=view.data.party[e.slot].value;
    if(mon.egg || mon.pid!=e.expectedPid || mon.ot!=e.expectedOt ||
       mon.species!=e.expectedSpecies ||
       mon.checksum!=e.expectedPokemonChecksum)return false;
    if(e.kind==Kind::Move &&
       (mon.moves[e.field]==0 || mon.moves[e.field]!=e.expectedMoveId))return false;
    return currentValue(view,e)==e.expectedValue;
}
inline Result preview(const uint8_t* original,size_t n,const Batch& batch){
    if(!original || n!=raw_v1::Size || batch.version!=ContractVersion ||
       !hasDigest(batch.expectedDigest) || batch.edits.empty() ||
       batch.edits.size()>MaxEdits ||
       sourceDigest(original,n)!=batch.expectedDigest)
        return blocked("Invalid, empty, oversized or stale v3 source batch");
    const auto source=v2::inspect(original,n);
    if(!source.data.valid || source.data.activeCounter!=batch.expectedCounter)
        return blocked("Unrecognised or stale original copied H168 save");
    // Read-only preflight: establish identity of every scalar against THE
    // ORIGINAL, and prohibit duplicate ownership of any one field.
    for(size_t i=0;i<batch.edits.size();i++){
        if(!originalGuard(source,batch,batch.edits[i]))
            return blocked("Batch edit identity, original value or bounds invalid");
        for(size_t j=0;j<i;j++)
            if(sameField(batch.edits[j],batch.edits[i]))
                return blocked("Conflicting/duplicate scalar edit");
    }
    // Combined final EV budget; subtract edits' old values before adding
    // desired values. No intermediate mutation is performed at this stage.
    for(unsigned p=0;p<source.data.party.size();p++){
        unsigned sum=0;
        for(const auto v:source.data.party[p].value.evs)sum+=v;
        for(const auto& e:batch.edits)if(e.kind==Kind::EVs && e.slot==p){
            sum-=e.expectedValue;
            sum+=e.value;
        }
        if(sum>510)return blocked("Illegal combined EV total");
    }
    // Reverse-pressure order: EV decreases first, then other edits,
    // then EV increases. Therefore *each intermediate* v2 candidate
    // obeys its 510 total even when transferring EVs between stats.
    std::vector<size_t> order(batch.edits.size());
    for(size_t i=0;i<order.size();i++)order[i]=i;
    const auto priority=[&](size_t i){
        const auto& e=batch.edits[i];
        return e.kind!=Kind::EVs?1:(e.value<e.expectedValue?0:2);
    };
    std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){
        return priority(a)<priority(b);
    });
    Bytes working(original,original+n);
    // Independent round-trip gate on EVERY intermediate copy: never use the
    // original Pokemon checksum after earlier changes to that same mon.
    for(const auto i:order){
        const Edit& e=batch.edits[i];
        const auto current=v2::inspect(working.data(),working.size());
        if(!current.data.valid || current.data.activeCounter!=batch.expectedCounter)
            return blocked("Intermediate snapshot invalid");
        Request req{};
        const bool ready=e.kind==Kind::Move?
            v2::beginMove(current,e.slot,e.field,req):
            v2::begin(current,e.kind,e.slot,req);
        if(!ready || currentValue(current,e)!=e.expectedValue)
            return blocked("Intermediate scalar changed unexpectedly");
        if(e.kind==Kind::Money)req.money=e.value;
        else if(e.kind==Kind::ExistingQuantity)req.quantity=uint16_t(e.value);
        else if(e.kind==Kind::IVs || e.kind==Kind::EVs)req.stats[e.field]=uint8_t(e.value);
        else req.pp=uint8_t(e.value);
        auto step=v2::preview(working.data(),working.size(),req);
        if(!step.ok || step.candidate.size()!=n)
            return blocked("Individually guarded intermediate edit failed");
        working=std::move(step.candidate);
    }
    // Every intermediate v2 preview validates the exact single-edit footprint.\n    // A second independent reconstruction below proves the whole batch.\n    // Independent expected-after image: rebuild from original bytes, with
    // direct encoding of the exact permitted scalar fields (NOT by calling
    // v2 preview). This proves no aggregate mutation escaped the union
    // of allowed byte fields and the two checksums.
    Bytes expected(original,original+n);
    // Every permitted edit is confined to the currently active 4096-byte
    // save section 1. Bound the footprint mask to that section to avoid
    // allocating an unnecessary 128 KiB map on the 3DS.
    std::vector<uint8_t> writable(raw_v1::Sector,0);
    raw_v1::Info info{};
    if(!validate(original,n,info))return blocked("Original H168 structural failure");
    const auto& bank=info.banks[info.active];
    const size_t s1=bank.sector[1];
    const auto mark=[&](size_t at,size_t length){
        if(at<s1 || at-s1>raw_v1::Sector ||
           length>raw_v1::Sector-(at-s1))return false;
        for(size_t i=0;i<length;i++)writable[at-s1+i]=1;
        return true;
    };
    for(const auto& e:batch.edits){
        if(e.kind==Kind::Money){
            const size_t at=s1+layout::Money;
            raw_v1::w32(expected.data(),at,e.value^bank.key);
            if(!mark(at,4))return blocked("Invalid money offset");
        }else if(e.kind==Kind::ExistingQuantity){
            const size_t at=s1+layout::BagArray+e.slot*layout::ItemSlotStride+2;
            raw_v1::w16(expected.data(),at,uint16_t(e.value)^uint16_t(bank.key));
            if(!mark(at,2))return blocked("Invalid item offset");
        }else{
            const size_t at=s1+layout::PartyStart+e.slot*layout::PartyStride;
            auto mon=plaintext(expected.data(),at);
            unsigned block=0;
            if(e.kind==Kind::IVs){
                block=storedIndex(e.expectedPid,'M');
                uint32_t v=raw_v1::r32(mon.data(),40);
                const unsigned shift=e.field*5u;
                v=(v & ~(uint32_t(31)<<shift)) | (e.value<<shift);
                raw_v1::w32(mon.data(),40,v);
                if(!mark(at+32+block*12+4,4))
                    return blocked("Invalid IV offset");
            }else if(e.kind==Kind::EVs){
                block=storedIndex(e.expectedPid,'E');
                mon[24+e.field]=uint8_t(e.value);
                if(!mark(at+32+block*12+e.field,1))
                    return blocked("Invalid EV offset");
            }else{
                block=storedIndex(e.expectedPid,'A');
                mon[20+e.field]=uint8_t(e.value);
                if(!mark(at+32+block*12+8+e.field,1))
                    return blocked("Invalid PP offset");
            }
            encode(expected.data(),at,mon);
            if(!mark(at+28,2))return blocked("Invalid mon checksum");
        }
    }
    raw_v1::w16(expected.data(),s1+0xff6,raw_v1::checksum(expected.data()+s1));
    if(!mark(s1+0xff6,2))return blocked("Invalid sector checksum");
    if(expected!=working)return blocked("Independent aggregate byte proof differs");
    const auto final=v2::inspect(working.data(),working.size());
    if(!final.data.valid || final.data.activeCounter!=batch.expectedCounter ||
       final.data.party.size()!=source.data.party.size() ||
       final.data.items.size()!=source.data.items.size())
        return blocked("Final independent H168 roundtrip failed");
    for(const auto& e:batch.edits)
        if(currentValue(final,e)!=e.value)
            return blocked("Final independent field roundtrip differs");
    Result result{};
    result.reason="V3 RAM-only combined draft; never restore to installed game";
    for(size_t i=0;i<n;i++)if(working[i]!=original[i]){
        if(i<s1 || i-s1>=raw_v1::Sector || !writable[i-s1])
            return blocked("Aggregate footprint escaped allowed fields");
        result.changedOffsets.push_back(i);
    }
    if(result.changedOffsets.empty())return blocked("Aggregate no-op");
    result.ok=true;result.candidate=std::move(working);
    return result;
}
inline bool verify(const uint8_t* original,size_t n,const uint8_t* candidate,
                   size_t candidateSize,const Batch& batch){
    if(!original || !candidate || n!=raw_v1::Size || candidateSize!=n)
        return false;
    const auto expected=preview(original,n,batch);
    return expected.ok && expected.candidate.size()==n &&
           std::memcmp(expected.candidate.data(),candidate,n)==0;
}
inline bool mayWriteInstalledGame(const Batch&){return false;}
}} // namespace rogue18::v3
