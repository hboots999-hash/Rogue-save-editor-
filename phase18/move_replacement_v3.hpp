// SPDX-License-Identifier: GPL-3.0-or-later
// Phase18 MOVE-V3: exact-ROM authenticated, copy-only EXPERIMENTAL save draft.
// This contract never accesses installed game archives or performs restore.
// Exported drafts MUST be emulator-tested before any use on a physical 3DS.
#pragma once
#include "move_replacement_v2.hpp"
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>

namespace rogue18 { namespace move_v3 {
constexpr uint32_t ContractVersion=3;
constexpr bool MayWriteInstalledGame=false;
constexpr bool EmulatorGameplayCertified=false;
// Chosen from the exact 2.1.2 ROM only: ordinary names/IDs, equal nonzero
// PP in BOTH compiled tables, no special, struggle, or mode-conflicting PP.
// The list is intentionally narrow, NOT a species-level legality promise.
constexpr std::array<uint16_t,43> FirstCiaIds{{
    7,8,9,14,33,53,56,57,73,85,89,94,126,127,157,182,
    183,188,202,229,242,245,247,337,347,349,369,370,
    396,403,405,406,412,417,418,434,442,444,483,521,
    542,583,585
}};
inline bool selectedOrdinary(uint16_t id) {
    return std::find(FirstCiaIds.begin(),FirstCiaIds.end(),id)!=FirstCiaIds.end();
}
struct Selection {
    uint32_t contract=ContractVersion;
    move_v2::Request original{};
};
inline bool begin(const v2::ReadSnapshot& snapshot,
                  const uint8_t* save,size_t n,
                  unsigned partySlot,unsigned moveSlot,Selection& selection){
    selection=Selection{};
    return move_v2::begin(snapshot,save,n,partySlot,moveSlot,selection.original);
}
inline bool select(const move_v2::Catalog& db,Selection& selection,uint16_t id){
    if(selection.contract!=ContractVersion || !selectedOrdinary(id))
        return false;
    return move_v2::choose(db,selection.original,id);
}
// This is the smallest copy-only replacement operation, deliberately
// separate from the Phase18 v3 PP-only scalar edit and Phase20 v4 queue.
// No caller-supplied PP or caller-supplied "approved" bool.
inline Result preview(const uint8_t* raw,size_t n,
                      const move_v2::Catalog& db,
                      const Selection& selection){
    try{
        const auto& req=selection.original;
        move_v2::Move chosen{};
        if(selection.contract!=ContractVersion ||
           req.version!=move_v2::ContractVersion ||
           req.catalogDigest!=move_v2::CatalogSha ||
           !selectedOrdinary(req.replacementId) ||
           !move_v2::lookup(db,req.replacementId,chosen) ||
           chosen.id!=req.replacementId || !raw ||
           n!=raw_v1::Size || !hasDigest(req.source.expectedOriginalSha256) ||
           sourceDigest(raw,n)!=req.source.expectedOriginalSha256 ||
           req.source.version!=move_v1::ContractVersion ||
           req.source.partySlot>=6 || req.source.moveSlot>=4 ||
           chosen.basePP<1 || chosen.basePP>64 ||
           req.replacementId==req.source.expectedMoveId)
            return blocked("Move V3: untrusted ROM, source, slot, or move ID");

        raw_v1::Info before{};Mon mon{};
        if(!validate(raw,n,before) ||
           before.banks[before.active].counter!=req.source.expectedCounter ||
           !partyAt(raw,n,before,req.source.partySlot,mon) || mon.egg ||
           mon.pid!=req.source.expectedPid ||
           mon.ot!=req.source.expectedOt ||
           mon.species!=req.source.expectedSpecies ||
           mon.checksum!=req.source.expectedPokemonChecksum ||
           mon.moves[req.source.moveSlot]!=req.source.expectedMoveId ||
           mon.pp[req.source.moveSlot]!=req.source.expectedPP)
            return blocked("Move V3: stale Pokémon or corrupted save");
        for(unsigned j=0;j<4;j++)
            if(j!=req.source.moveSlot &&
               mon.moves[j]==req.replacementId)
                return blocked("Move V3: duplicate active move");

        const size_t sector=before.banks[before.active].sector[1];
        const size_t at=sector+layout::PartyStart+
                        req.source.partySlot*layout::PartyStride;
        auto plain=plaintext(raw,at);
        if(plain[8]!=req.source.expectedPPBonuses)
            return blocked("Move V3: stale PP-Up guard");
        const auto unmodified=plain;
        // Current experimental rule: forget old slot's PP Ups; give the new
        // move its verified base PP. This is structurally valid, but is NOT
        // yet gameplay-certified against every move-relearner code path.
        plain[8]=move_v1::clearedSlotBonuses(plain[8],req.source.moveSlot);
        raw_v1::w16(plain.data(),12+2*req.source.moveSlot,
                    req.replacementId);
        plain[20+req.source.moveSlot]=chosen.basePP;

        Bytes draft(raw,raw+n);
        encode(draft.data(),at,plain);
        raw_v1::w16(draft.data(),sector+0xff6,
                    raw_v1::checksum(draft.data()+sector));

        // Independent second decode + complete 128 KiB structural reparse.
        raw_v1::Info after{};Mon updated{};
        if(!validate(draft.data(),draft.size(),after) ||
           after.active!=before.active ||
           after.banks[0].counter!=before.banks[0].counter ||
           after.banks[1].counter!=before.banks[1].counter ||
           after.partyCount!=before.partyCount ||
           after.money!=before.money || after.items!=before.items ||
           !partyAt(draft.data(),draft.size(),after,
                    req.source.partySlot,updated) ||
           plaintext(draft.data(),at)!=plain)
            return blocked("Move V3: draft failed independent save reparse");

        // Identity, nature, ability, IVs, EVs, held items, nickname, and
        // unrelated active move slots cannot change.
        if(updated.pid!=mon.pid || updated.ot!=mon.ot ||
           updated.species!=mon.species || updated.heldItem!=mon.heldItem ||
           updated.ivs!=mon.ivs || updated.evs!=mon.evs ||
           updated.abilitySlot!=mon.abilitySlot ||
           updated.effectiveNature!=mon.effectiveNature ||
           updated.gigantamaxFactor!=mon.gigantamaxFactor ||
           updated.moves[req.source.moveSlot]!=req.replacementId ||
           updated.pp[req.source.moveSlot]!=chosen.basePP)
            return blocked("Move V3: unrelated Pokémon identity changed");
        for(unsigned j=0;j<4;j++)
            if(j!=req.source.moveSlot &&
               (updated.moves[j]!=mon.moves[j] ||
                updated.pp[j]!=mon.pp[j]))
                return blocked("Move V3: unrelated move changed");
        for(unsigned i=0;i<48;i++){
            const bool allowed=i==8 || i==12+2*req.source.moveSlot ||
                i==13+2*req.source.moveSlot ||
                i==20+req.source.moveSlot;
            if(!allowed && plain[i]!=unmodified[i])
                return blocked("Move V3: unrelated decrypted byte changed");
        }
        const size_t a=at+32+storedIndex(mon.pid,'A')*12;
        const size_t g=at+32+storedIndex(mon.pid,'G')*12;
        const std::array<size_t,8> whitelist{{
            g+8,a+2*req.source.moveSlot,a+2*req.source.moveSlot+1,
            a+8+req.source.moveSlot,at+28,at+29,
            sector+0xff6,sector+0xff7
        }};
        Result out{};
        for(size_t i=0;i<n;i++){
            if(raw[i]!=draft[i]){
                if(std::find(whitelist.begin(),whitelist.end(),i)==
                   whitelist.end())
                    return blocked("Move V3: byte outside exact footprint");
                out.changedOffsets.push_back(i);
            }
        }
        if(out.changedOffsets.empty() || out.changedOffsets.size()>8)
            return blocked("Move V3: invalid changed-byte count");
        out.candidate=std::move(draft);
        out.ok=true;
        out.reason="Experimental OFFLINE DRAFT only; installed writes disabled; gameplay not certified";
        return out;
    }catch(const std::exception&){
        return blocked("Move V3: exception during offline preview");
    }
}
inline bool verify(const uint8_t* original,size_t n,
                   const uint8_t* candidate,size_t len,
                   const move_v2::Catalog& db,const Selection& selection){
    if(!candidate || len!=raw_v1::Size || n!=len)return false;
    const auto expected=preview(original,n,db,selection);
    return expected.ok && expected.candidate.size()==len &&
           std::memcmp(expected.candidate.data(),candidate,len)==0;
}
inline bool mayWriteInstalledGame(const Selection&){return false;}
}} // namespace rogue18::move_v3
