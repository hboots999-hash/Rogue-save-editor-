/* Phase 13: H168 live restore PRE-FLIGHT, data-only, NO IO or Pxi writes.
 * A valid save alone is never authorization to restore. Small allowlisted edits
 * must be independently provable against the currently loaded raw save.
 * GPL-3.0-or-later.
 */
#pragma once
#include "offline.hpp"
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <string>
namespace rogue_restore {
enum class Operation:uint8_t { MoneyPlusOne=1, ItemQuantityPlusOne=2 };
struct Validation {
    bool ok=false;
    std::string reason;
    unsigned changed=0;
    unsigned oldValue=0,newValue=0;
    unsigned targetItem=0;
    Validation(bool v=false,std::string msg={},unsigned n=0,unsigned old=0,
               unsigned next=0,unsigned id=0)
        : ok(v),reason(std::move(msg)),changed(n),oldValue(old),
          newValue(next),targetItem(id) {}

};
inline Validation validate(const uint8_t* before,const uint8_t* after,
                           Operation operation,unsigned itemIndex=0){
    if(!before||!after)return {false,"Missing raw save"};
    h168::Info a{},b{};
    if(!h168::inspect(before,h168::Size,a)||
       !h168::inspect(after,h168::Size,b))
        return {false,"Invalid source or candidate save"};
    if(a.active!=b.active||a.partyCount!=b.partyCount||
       a.items!=b.items||
       a.banks[0].counter!=b.banks[0].counter||
       a.banks[1].counter!=b.banks[1].counter)
        return {false,"Changed save topology"};
    const auto& bank=a.banks[a.active];
    const auto& newer=b.banks[b.active];
    for(unsigned i=0;i<14;i++){
        if(bank.sector[i]!=newer.sector[i])
            return {false,"Changed section order"};
    }
    size_t pos=0,width=0;
    unsigned oldValue=0,newValue=0,targetItem=0;
    if(operation==Operation::MoneyPlusOne){
        oldValue=a.money;newValue=b.money;
        if(oldValue>=999999u||newValue!=oldValue+1)
            return {false,"Only money +1 currently supported"};
        pos=bank.sector[1]+0x4a8;width=4;
    }else if(operation==Operation::ItemQuantityPlusOne){
        unsigned oldId=0,newId=0,oldQty=0,newQty=0;
        if(!h168::item(before,a,itemIndex,oldId,oldQty)||
           !h168::item(after,b,itemIndex,newId,newQty)||
           oldId!=newId||oldQty>=999||newQty!=oldQty+1)
            return {false,"Only existing item quantity +1 supported"};
        oldValue=oldQty;newValue=newQty;targetItem=oldId;
        pos=bank.sector[1]+0x578+itemIndex*4u+2;width=2;
        if(a.money!=b.money)return {false,"Unexpected money change"};
    }else return {false,"Unknown operation"};
    const auto ck=bank.sector[1]+0xff6;
    unsigned changed=0;
    bool changedField=false;
    for(size_t i=0;i<h168::Size;i++){
        if(before[i]==after[i])continue;
        ++changed;
        if(i>=pos && i<pos+width)changedField=true;
        else if(i!=ck && i!=ck+1)
            return {false,"Modified byte outside field/checksum"};
    }
    if(!changedField || changed>width+2)
        return {false,"Bad write footprint"};
    // h168::inspect checked every segment and Pokémon checksum in both
    // images; the limited footprint also proves party and old bank unchanged.
    return {true,"Allowlisted candidate, NOT permission to restore",
            changed,oldValue,newValue,targetItem};
}
// The next phase may hand a limited candidate to the authenticated writer.
// This function NEVER writes. It makes one 128KiB RAM copy and validates it.
inline std::unique_ptr<uint8_t[]> prepare(const uint8_t* source,
                                          Operation op,unsigned itemIndex,
                                          Validation& proof){
    proof={false,"Unverified"};
    h168::Info info{};
    if(!h168::inspect(source,h168::Size,info)){
        proof.reason="Source not validated";return {};
    }
    const auto& bank=info.banks[info.active];
    const size_t s1=bank.sector[1];
    size_t off=0;
    unsigned old=0;
    uint32_t encoded=0;
    if(op==Operation::MoneyPlusOne){
        if(info.money>=999999){proof.reason="Money limit";return {};}
        off=s1+0x4a8;old=info.money;
        encoded=(old+1)^bank.key;
    }else if(op==Operation::ItemQuantityPlusOne){
        unsigned id=0,qty=0;
        if(!h168::item(source,info,itemIndex,id,qty)||qty>=999){
            proof.reason="Unknown item/quantity limit";return {};
        }
        off=s1+0x578+itemIndex*4u+2;old=qty;
        encoded=uint16_t(qty+1)^uint16_t(bank.key);
    }else{
        proof.reason="Unsupported operation";return {};
    }
    auto candidate=std::unique_ptr<uint8_t[]>(new(std::nothrow)uint8_t[h168::Size]);
    if(!candidate){proof.reason="Memory allocation failed";return {};}
    std::memcpy(candidate.get(),source,h168::Size);
    if(op==Operation::MoneyPlusOne)
        h168::w32(candidate.get(),off,encoded);
    else h168::w16(candidate.get(),off,uint16_t(encoded));
    h168::w16(candidate.get(),s1+0xff6,h168::checksum(candidate.get()+s1));
    proof=validate(source,candidate.get(),op,itemIndex);
    if(!proof.ok)return {};
    if(proof.oldValue!=old){proof={false,"Preflight old value mismatch"};return {};}
    return candidate;
}
} // namespace rogue_restore
