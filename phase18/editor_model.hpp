// Phase 18 -> Phase 19 view-model interface, version 1.
// No UI, IO, live title or restoration code. GPL-3.0-or-later.
#pragma once
#include "pc_readonly.hpp"
#include "source_digest.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rogue18 {
constexpr uint32_t ModelApiVersion=1;
enum class DataConfidence : uint8_t {
    Unsupported=0, StructurallyValidated=1, ExSourceCandidate=2
};
struct ItemView {
    unsigned slot=0;
    uint16_t itemId=0,quantity=0;
};
struct PartyView {
    unsigned slot=0;
    Mon value{};
};
struct EditorCapabilities {
    bool copiedSaveOnly=true;
    bool money=true,existingQuantity=true,ivs=true,evs=true,movePp=true;
    bool arbitraryMoveId=false,abilityId=false,nature=false,insertItem=false;
    bool pcEdit=false,liveWrite=false,restore=false;
};
struct SaveView {
    uint32_t apiVersion=ModelApiVersion;
    bool valid=false;
    DataConfidence confidence=DataConfidence::Unsupported;
    std::string error{};
    uint32_t activeCounter=0,inactiveCounter=0,money=0;
    Digest256 sourceSha256{}; // source snapshot identity, no personal bytes recorded
    unsigned activeBank=0;
    std::vector<PartyView> party{};
    std::vector<ItemView> items{};
    // This is explicitly a candidate shape, never a confirmed 2.1.2 PC.
    PcInspection pc{};
    EditorCapabilities capabilities{};
};
inline SaveView inspectReadOnly(const uint8_t* bytes,size_t size,
                               bool researchPcCandidate=false){
    SaveView v{};
    h168::Info info{};
    if(!validate(bytes,size,info)){
        v.error="Unsupported H168 copy: dual-bank or decrypted data invalid";
        return v;
    }
    v.activeBank=info.active;
    v.activeCounter=info.banks[info.active].counter;
    v.inactiveCounter=info.banks[info.active^1u].counter;
    v.money=info.money;
    for(unsigned i=0;i<info.partyCount;i++){
        Mon mon{};
        if(!partyAt(bytes,size,info,i,mon)){
            v.error="Party record failed read-only integrity check";
            return v;
        }
        v.party.push_back({i,mon});
    }
    for(unsigned i=0;i<info.items;i++){
        unsigned id=0,qty=0;
        if(!h168::item(bytes,info,i,id,qty)){
            v.error="Bag record failed read-only integrity check";
            return v;
        }
        v.items.push_back({i,static_cast<uint16_t>(id),
                            static_cast<uint16_t>(qty)});
    }
    if(researchPcCandidate)v.pc=inspectPcCandidate(bytes,size);
    v.sourceSha256=sourceDigest(bytes,size);
    v.valid=true;
    v.confidence=DataConfidence::StructurallyValidated;
    return v;
}
// Phase19 supplies only desired values after user review. This helper
// constructs optimistic-lock expectations directly from a copied snapshot.
// A request is intentionally a no-op until the UI changes a desired value.
// Evidence must come from ROM-specific verification, not these display values.
inline bool beginRequest(const SaveView& view,Kind kind,unsigned slot,Request& out){
    out=Request{};
    if(!view.valid || view.apiVersion!=ModelApiVersion)return false;
    out.kind=kind;out.slot=slot;
    out.expectedCounter=view.activeCounter;
    out.expectedDigest=view.sourceSha256;
    if(kind==Kind::Money){
        out.expectedMoney=view.money;
        out.money=view.money;
        return slot==0;
    }
    if(kind==Kind::ExistingQuantity){
        if(slot>=view.items.size())return false;
        const auto& item=view.items[slot];
        if(item.slot!=slot)return false;
        out.expectedItemId=item.itemId;
        out.expectedQuantity=item.quantity;
        out.quantity=item.quantity;
        return true;
    }
    if(kind==Kind::InsertItem || kind==Kind::Nature ||
       kind==Kind::AbilitySlot || kind==Kind::IVs ||
       kind==Kind::EVs || kind==Kind::Move){
        if(kind==Kind::InsertItem){
            out.slot=static_cast<unsigned>(view.items.size());
            return slot==out.slot; // authorization still requires external evidence
        }
        if(slot>=view.party.size())return false;
        const auto& mon=view.party[slot].value;
        out.expectedPid=mon.pid;
        out.expectedOt=mon.ot;
        out.expectedSpecies=mon.species;
        out.expectedPokemonChecksum=mon.checksum;
        out.expectedNature=mon.effectiveNature;
        out.abilitySlot=mon.abilitySlot;
        out.nature=mon.effectiveNature;
        if(kind==Kind::IVs)out.stats=mon.ivs;
        if(kind==Kind::EVs)out.stats=mon.evs;
        if(kind==Kind::Move){
            out.moveSlot=0;
            out.expectedMoveId=mon.moves[0];
            out.moveId=mon.moves[0];
            out.expectedPP=mon.pp[0];
            out.pp=mon.pp[0];
        }
        return true;
    }
    return false;
}
} // namespace rogue18
