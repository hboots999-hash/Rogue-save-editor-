// Phase 18 — pure-memory H168 offline editor. GPL-3.0-or-later.
// No filesystem, SD, PxiFS, game archive, restore or signing operations.
// Layout is observed for the private GBA-P-H168 Emerald Rogue 2.1.2 save,
// but structural validity is NOT proof that a draft is game-playable.
#pragma once
#include "raw_v1.hpp"
#include "bag_rules.hpp"
#include "layout_geometry.hpp"
#include "source_digest.hpp"
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace rogue18 {
namespace h168 = raw_v1; // Internal compatibility alias ONLY; never ::h168
using Bytes=std::vector<uint8_t>;
enum class Kind : uint8_t {
    Money, ExistingQuantity, InsertItem, IVs, EVs, Move, AbilitySlot, Nature
};
struct Evidence {
    // NEVER populate from guesswork or simply because a ROM has an H168 label.
    // No verified H168 move database, pocket map or alternate-ability map is
    // shipped. A calling integration must separately validate those semantics.
    std::vector<uint16_t> verifiedMoveIds{};
    std::vector<uint16_t> verifiedInsertItemIds{};
    bool verifiedBagInsertion=false;
    // Capacity-upgrade byte address is not yet confirmed for private H168.
    // Explicit independent evidence is required; 255 means unverified.
    unsigned verifiedUpgradeCount=255;
    bool verifiedH168BagUpgradeOffset=false;
    std::vector<std::pair<uint16_t,uint8_t>> verifiedItemPockets{};
    bool verifiedAbilitySlot=false;
    bool verifiedNatureModifier=false; // matching ROM's GetNature uses header byte +18
};
struct Mon {
    uint32_t pid=0,ot=0;
    uint16_t species=0,heldItem=0,checksum=0;
    std::array<uint16_t,4> moves{};
    std::array<uint8_t,4> pp{};
    std::array<uint8_t,6> ivs{},evs{};
    uint8_t abilitySlot=0,natureFromPid=0,effectiveNature=0;
    uint8_t gigantamaxFactor=0,rawNatureModifier=0;
    bool egg=false;
};
struct Request {
    Kind kind=Kind::Money;
    uint32_t expectedCounter=0; // active-bank optimistic lock; required
    Digest256 expectedDigest{}; // optional for direct callers; Phase19 always pins SHA-256
    unsigned slot=0;             // party index or bag index
    uint32_t expectedMoney=0,money=0;
    uint16_t expectedItemId=0,expectedQuantity=0,itemId=0,quantity=0;
    uint32_t expectedPid=0,expectedOt=0;
    uint16_t expectedSpecies=0,expectedPokemonChecksum=0;
    std::array<uint8_t,6> stats{};
    uint8_t moveSlot=0,expectedPP=0,pp=0,abilitySlot=0,nature=0;
    uint8_t expectedNature=0;
    uint16_t expectedMoveId=0,moveId=0;
};
struct Result {
    bool ok=false;
    std::string reason;
    Bytes candidate{};
    std::vector<size_t> changedOffsets{};
};
inline bool included(const std::vector<uint16_t>& a,uint16_t v) {
    return std::find(a.begin(),a.end(),v)!=a.end();
}
inline const char* const* orders() {
    static const char* const x[24]={
        "GAEM","GAME","GEAM","GEMA","GMAE","GMEA",
        "AGEM","AGME","AEGM","AEMG","AMGE","AMEG",
        "EGAM","EGMA","EAGM","EAMG","EMGA","EMAG",
        "MGAE","MGEA","MAGE","MAEG","MEGA","MEAG"
    };
    return x;
}
inline unsigned canonicalIndex(char c){
    return c=='G'?0u:c=='A'?1u:c=='E'?2u:3u;
}
inline unsigned storedIndex(uint32_t pid,char c){
    const char* order=orders()[pid%24];
    for(unsigned i=0;i<4;i++)if(order[i]==c)return i;
    return 4;
}
inline uint16_t checksumMon(const std::array<uint8_t,48>& canonical){
    uint32_t sum=0;
    for(unsigned i=0;i<48;i+=2)sum+=h168::r16(canonical.data(),i);
    return uint16_t(sum);
}
inline std::array<uint8_t,48> plaintext(const uint8_t* raw,size_t start){
    std::array<uint8_t,48> canonical{};
    const uint32_t pid=h168::r32(raw,start);
    const uint32_t key=pid^h168::r32(raw,start+4);
    const char* order=orders()[pid%24];
    for(unsigned physical=0;physical<4;physical++){
        const unsigned dest=canonicalIndex(order[physical])*12;
        for(unsigned j=0;j<12;j+=4)
            h168::w32(canonical.data(),dest+j,
                       h168::r32(raw,start+32+physical*12+j)^key);
    }
    return canonical;
}
inline void encode(uint8_t* raw,size_t start,const std::array<uint8_t,48>& canonical){
    const uint32_t pid=h168::r32(raw,start);
    const uint32_t key=pid^h168::r32(raw,start+4);
    const char* order=orders()[pid%24];
    for(unsigned physical=0;physical<4;physical++){
        const unsigned src=canonicalIndex(order[physical])*12;
        for(unsigned j=0;j<12;j+=4)
            h168::w32(raw,start+32+physical*12+j,
                       h168::r32(canonical.data(),src+j)^key);
    }
    h168::w16(raw,start+28,checksumMon(canonical));
}
inline bool decodeMon(const uint8_t* raw,size_t n,size_t start,Mon& result){
    result=Mon{};
    if(!raw || start>n || n-start<104)return false;
    const auto p=plaintext(raw,start);
    if(checksumMon(p)!=h168::r16(raw,start+28))return false;
    result.pid=h168::r32(raw,start);
    result.ot=h168::r32(raw,start+4);
    result.checksum=h168::r16(raw,start+28);
    // EX v2.1 growth 0: species occupies 11 bits, held item two fragments.
    // Do not interpret the entire low u16 as a species when an item is held.
    const uint32_t growth0=h168::r32(p.data(),0);
    result.species=uint16_t(growth0&0x7ffu);
    result.heldItem=uint16_t(((growth0>>11)&0x3ffu)|(((growth0>>26)&1u)<<10));
    if(!result.species)return false;
    for(unsigned i=0;i<4;i++){
        result.moves[i]=h168::r16(p.data(),12+2*i);
        result.pp[i]=p[20+i];
    }
    unsigned evTotal=0;
    for(unsigned i=0;i<6;i++){
        result.evs[i]=p[24+i];
        if(result.evs[i]>252)return false;
        evTotal+=result.evs[i];
    }
    if(evTotal>510)return false;
    const uint32_t packed=h168::r32(p.data(),40);
    for(unsigned i=0;i<6;i++)result.ivs[i]=uint8_t((packed>>(5*i))&31u);
    result.egg=((packed>>30)&1u)!=0;
    result.gigantamaxFactor=uint8_t(packed>>31); // NOT the ability!
    // EX v2.1 Misc+8 bitfield: abilityNum bits 29..30 (2-bit slot).
    result.abilitySlot=uint8_t((h168::r32(p.data(),44)>>29)&3u);
    if(result.abilitySlot>2)return false; // EX defines only 3 ability slots
    result.natureFromPid=uint8_t(result.pid%25);
    result.rawNatureModifier=uint8_t(raw[start+18]>>3);
    result.effectiveNature=uint8_t(result.natureFromPid^result.rawNatureModifier);
    if(result.effectiveNature>=25)return false;
    return true;
}
inline bool validateBankBag(const uint8_t* raw,const h168::Bank& bank){
    const size_t s1=bank.sector[1];
    bool gap=false;
    for(unsigned i=0;i<450;i++){
        const size_t off=s1+layout::BagArray+i*layout::ItemSlotStride;
        const uint16_t id=h168::r16(raw,off);
        if(!id){gap=true;continue;}
        if(gap)return false;
        const uint16_t qty=h168::r16(raw,off+2)^uint16_t(bank.key);
        if(qty<1||qty>999)return false;
    }
    return true;
}
inline bool validate(const uint8_t* raw,size_t n,h168::Info& info){
    if(!h168::inspect(raw,n,info))return false;
    // Save counter is uint32_t and wraps. Phase10's legacy inspector
    // chooses the numerically greatest counter, which is wrong at rollover.
    // Require adjacent counters in modular arithmetic and reselect the
    // true successor even if it is counter zero.
    const uint32_t a=info.banks[0].counter,b=info.banks[1].counter;
    if(static_cast<uint32_t>(a+1u)==b)info.active=1u;
    else if(static_cast<uint32_t>(b+1u)==a)info.active=0u;
    else return false;
    const auto& latest=info.banks[info.active];
    const size_t activeS1=latest.sector[1];
    info.money=h168::r32(raw,activeS1+layout::Money)^latest.key;
    info.partyCount=h168::r32(raw,activeS1+layout::PartyCount);
    info.items=0;
    for(unsigned j=0;j<layout::BagPhysicalSlots;j++){
        if(!h168::r16(raw,activeS1+layout::BagArray+j*layout::ItemSlotStride))break;
        ++info.items;
    }
    for(const auto& bank:info.banks){
        if(!validateBankBag(raw,bank))return false;
        const size_t s1=bank.sector[1];
        const uint32_t count=h168::r32(raw,s1+layout::PartyCount);
        if(count>6 || (h168::r32(raw,s1+layout::Money)^bank.key)>999999u)return false;
        for(unsigned i=0;i<count;i++){
            Mon m{};
            if(!decodeMon(raw,n,s1+layout::PartyStart+layout::PartyStride*i,m))return false;
        }
    }
    return true;
}
inline bool partyAt(const uint8_t* raw,size_t n,const h168::Info& info,
                    unsigned index,Mon& mon){
    if(!info.valid || index>=info.partyCount)return false;
    const size_t s1=info.banks[info.active].sector[1];
    return decodeMon(raw,n,s1+layout::PartyStart+layout::PartyStride*index,mon);
}
inline Result blocked(const char* reason){return {false,reason,{},{}};}
// Creates an edited 128-KiB RAM vector only. Does not export or import files.
// The caller must separately enforce a pinned source fingerprint, fresh SD
// import, verified immutable backup, exclusive draft write and human review.
inline Result propose(const uint8_t* raw,size_t n,const Request& req,
                      const Evidence& evidence=Evidence{}){
    h168::Info before{};
    if(!validate(raw,n,before))return blocked("Unrecognised/incomplete H168 dual-bank layout");
    if(hasDigest(req.expectedDigest) && sourceDigest(raw,n)!=req.expectedDigest)
        return blocked("Source SHA-256 differs from displayed copy");
    const h168::Bank& bank=before.banks[before.active];
    if(req.expectedCounter!=bank.counter)return blocked("Stale active-bank counter");
    // Nature is stored as a 5-bit XOR modifier in the unencrypted BoxPokemon
    // header on EX v2.1. It must not be changed without matching-ROM evidence.
    Bytes draft(raw,raw+n);
    std::vector<size_t> allowed;
    const size_t s1=bank.sector[1];
    const size_t footer=s1+0xff6;
    auto allow=[&](size_t pos,unsigned width){
        for(unsigned j=0;j<width;j++)allowed.push_back(pos+j);
    };
    bool monEdit=false;
    size_t monAt=0;
    Mon oldMon{};
    std::array<uint8_t,48> monData{};
    size_t field=0;
    unsigned width=0;
    if(req.kind==Kind::Money){
        if(before.money!=req.expectedMoney || req.money>999999u ||
           before.money==req.money)return blocked("Money guard, range or no-op");
        field=s1+layout::Money;width=4;
        h168::w32(draft.data(),field,req.money^bank.key);
    }else if(req.kind==Kind::ExistingQuantity){
        unsigned id=0,qty=0;
        if(!h168::item(raw,before,req.slot,id,qty) ||
           id!=req.expectedItemId || qty!=req.expectedQuantity ||
           req.quantity<1 || req.quantity>999 || qty==req.quantity)
            return blocked("Existing item guard, range or no-op");
        field=s1+layout::BagArray+req.slot*layout::ItemSlotStride+2;width=2;
        h168::w16(draft.data(),field,uint16_t(req.quantity^uint16_t(bank.key)));
    }else if(req.kind==Kind::InsertItem){
        if(!evidence.verifiedBagInsertion ||
           !included(evidence.verifiedInsertItemIds,req.itemId))
            return blocked("Unverified item pocket/ID: insertion disabled");
        if(!req.itemId || req.quantity<1 || req.quantity>999 ||
           before.items>=450 || req.slot!=before.items)
            return blocked("Insertion must append once into verified free slot");
        for(unsigned i=0;i<before.items;i++){
            unsigned id=0,qty=0;
            if(!h168::item(raw,before,i,id,qty) || id==req.itemId)
                return blocked("Duplicate/invalid item insertion");
        }
        std::vector<uint16_t> existingIds;
        for(unsigned i=0;i<before.items;i++){
            unsigned id=0,qty=0;
            if(!h168::item(raw,before,i,id,qty))return blocked("Unverified existing item");
            existingIds.push_back(static_cast<uint16_t>(id));
        }
        if(!evidence.verifiedH168BagUpgradeOffset ||
           evidence.verifiedUpgradeCount>BagMaxUpgrades ||
           layout::bagUpgradeCandidate(raw,s1)!=evidence.verifiedUpgradeCount)
            return blocked("Unverified or stale H168 bag upgrade field");
        if(!insertionCompatible(evidence.verifiedItemPockets,existingIds,
                                req.itemId,evidence.verifiedUpgradeCount))
            return blocked("Bag pocket ordering/capacity evidence missing or exceeded");
        field=s1+layout::BagArray+req.slot*layout::ItemSlotStride;width=4;
        h168::w16(draft.data(),field,req.itemId);
        h168::w16(draft.data(),field+2,uint16_t(req.quantity^uint16_t(bank.key)));
    }else{
        if(!partyAt(raw,n,before,req.slot,oldMon) || oldMon.egg ||
           oldMon.pid!=req.expectedPid || oldMon.ot!=req.expectedOt ||
           oldMon.species!=req.expectedSpecies ||
           oldMon.checksum!=req.expectedPokemonChecksum)
            return blocked("Pokémon identity/checksum guard or egg rejected");
        monAt=s1+layout::PartyStart+layout::PartyStride*req.slot;
        monData=plaintext(raw,monAt);
        monEdit=true;
        if(req.kind==Kind::IVs){
            uint32_t pack=h168::r32(monData.data(),40);
            for(unsigned i=0;i<6;i++){
                if(req.stats[i]>31)return blocked("IV out of range");
                pack=(pack&~(uint32_t(31)<<(5*i)))|
                     (uint32_t(req.stats[i])<<(5*i));
            }
            h168::w32(monData.data(),40,pack);
            field=monAt+32+storedIndex(oldMon.pid,'M')*12+4;width=4;
        }else if(req.kind==Kind::EVs){
            unsigned sum=0;
            for(unsigned i=0;i<6;i++){
                if(req.stats[i]>252)return blocked("EV out of range");
                sum+=req.stats[i];
                monData[24+i]=req.stats[i];
            }
            if(sum>510)return blocked("EV total exceeds 510");
            field=monAt+32+storedIndex(oldMon.pid,'E')*12;width=6;
        }else if(req.kind==Kind::Move){
            if(req.moveSlot>=4 ||
               oldMon.moves[req.moveSlot]!=req.expectedMoveId ||
               oldMon.pp[req.moveSlot]!=req.expectedPP ||
               oldMon.moves[req.moveSlot]==0 ||
               req.moveId==0 || req.pp>64)
                return blocked("Move guard, ID, PP or slot invalid");
            if(req.moveId!=req.expectedMoveId &&
               !included(evidence.verifiedMoveIds,req.moveId))
                return blocked("Move ID not independently verified for H168 ROM");
            h168::w16(monData.data(),12+req.moveSlot*2,req.moveId);
            monData[20+req.moveSlot]=req.pp;
            // Move ID and PP are disjoint bytes in the encrypted A block.
            field=monAt+32+storedIndex(oldMon.pid,'A')*12+req.moveSlot*2;
            width=2;
            allow(monAt+32+storedIndex(oldMon.pid,'A')*12+8+req.moveSlot,1);
        }else if(req.kind==Kind::AbilitySlot){
            if(!evidence.verifiedAbilitySlot || req.abilitySlot>2)
                return blocked("Ability ID/mapping unverified or slot out of range");
            // Bits 29..30 in the MISC+8 word, not the gigantamax bit at
            // MISC+4 bit31. This changes only the slot, not ability ID.
            uint32_t abilities=h168::r32(monData.data(),44);
            abilities=(abilities & ~(uint32_t(3)<<29)) |
                      (uint32_t(req.abilitySlot)<<29);
            h168::w32(monData.data(),44,abilities);
            field=monAt+32+storedIndex(oldMon.pid,'M')*12+8;width=4;
        }else if(req.kind==Kind::Nature){
            if(!evidence.verifiedNatureModifier || req.nature>=25 ||
               req.expectedNature!=oldMon.effectiveNature)
                return blocked("Unverified nature encoding, stale expected nature or range");
            draft[monAt+18]=uint8_t((draft[monAt+18]&7u) |
                               ((oldMon.natureFromPid^req.nature)<<3));
            field=monAt+18;width=1;
        }else return blocked("Unsupported operation");
        encode(draft.data(),monAt,monData);
        allow(monAt+28,2); // per-Pokémon decrypted-payload checksum
    }
    allow(field,width);
    allow(footer,2);
    h168::w16(draft.data(),footer,h168::checksum(draft.data()+s1));
    h168::Info after{};
    if(!validate(draft.data(),draft.size(),after) ||
       after.active!=before.active ||
       after.banks[0].counter!=before.banks[0].counter ||
       after.banks[1].counter!=before.banks[1].counter ||
       after.partyCount!=before.partyCount ||
       after.items!=before.items+(req.kind==Kind::InsertItem?1u:0u) ||
       (req.kind!=Kind::Money && after.money!=before.money) ||
       (req.kind==Kind::Money && after.money!=req.money))
        return blocked("Independent draft structural/redecode check failed");
    if(monEdit){
        Mon newMon{};
        if(!partyAt(draft.data(),draft.size(),after,req.slot,newMon) ||
           newMon.pid!=oldMon.pid || newMon.ot!=oldMon.ot ||
           newMon.species!=oldMon.species)
            return blocked("Pokémon identity changed");
        if(req.kind==Kind::IVs && newMon.ivs!=req.stats)
            return blocked("IV readback mismatch");
        if(req.kind==Kind::EVs && newMon.evs!=req.stats)
            return blocked("EV readback mismatch");
        if(req.kind==Kind::Move &&
           (newMon.moves[req.moveSlot]!=req.moveId || newMon.pp[req.moveSlot]!=req.pp))
            return blocked("Move/PP readback mismatch");
        if(req.kind==Kind::AbilitySlot &&
           (newMon.abilitySlot!=req.abilitySlot ||
            newMon.gigantamaxFactor!=oldMon.gigantamaxFactor))
            return blocked("Ability slot/Gigantamax readback mismatch");
        if(req.kind==Kind::Nature &&
           (newMon.effectiveNature!=req.nature || newMon.pid!=oldMon.pid ||
            newMon.abilitySlot!=oldMon.abilitySlot))
            return blocked("Nature readback mismatch");
    }
    if(req.kind==Kind::ExistingQuantity || req.kind==Kind::InsertItem){
        unsigned id=0,qty=0;
        if(!h168::item(draft.data(),after,req.slot,id,qty) ||
           id!=(req.kind==Kind::InsertItem?req.itemId:req.expectedItemId) ||
           qty!=req.quantity)return blocked("Item readback mismatch");
    }
    Result out{};
    out.reason="Offline RAM candidate only; no release/restore approval";
    for(size_t i=0;i<n;i++){
        if(raw[i]==draft[i])continue;
        if(std::find(allowed.begin(),allowed.end(),i)==allowed.end())
            return blocked("Change outside permitted byte footprint");
        out.changedOffsets.push_back(i);
    }
    if(out.changedOffsets.empty())return blocked("No-op edit rejected");
    out.ok=true;
    out.candidate=std::move(draft);
    return out;
}
// Independent caller-facing guard against modified drafts / stale inputs.
// Recreates the exact candidate and checks every byte, including all untouched
// sectors, unused save tail and old bank.
inline bool verifyExact(const uint8_t* original,size_t n,const uint8_t* candidate,
                        size_t candidateLength,const Request& req,
                        const Evidence& evidence=Evidence{}){
    if(!original || !candidate || candidateLength!=n)return false;
    auto expected=propose(original,n,req,evidence);
    return expected.ok &&
           std::memcmp(expected.candidate.data(),candidate,n)==0;
}
} // namespace rogue18
