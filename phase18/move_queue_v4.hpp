// Phase20-facing v4 queue adapter. Existing v3 scalar PP-only batch is unchanged.
// Replacement additions are recorded, never executed until a new release gate.
#pragma once
#include "move_replacement_v2.hpp"
#include "multi_edit_v3.hpp"
#include <vector>
namespace rogue18 { namespace move_queue_v4 {
constexpr uint32_t ContractVersion=4;
constexpr size_t MaxReplacements=24;
struct Queue {
    uint32_t version=ContractVersion;
    v3::Batch scalars{};
    std::vector<move_v2::Request> replacements{};
};
inline bool begin(const v2::ReadSnapshot& snapshot,Queue& out){
    out=Queue{};
    return v3::start(snapshot,out.scalars);
}
// A replacement rewrites the mon's 48-byte encrypted payload and checksum.
// Before an authenticated atomic composer exists, no other IV/EV/PP/replacement
// operation may target that Pokémon in the same queue.
inline bool touchesMon(const Kind kind){
    return kind==Kind::Move || kind==Kind::IVs || kind==Kind::EVs;
}
inline bool appendScalar(const v2::ReadSnapshot& snapshot,Queue& queue,
                         Kind kind,unsigned slot,unsigned field,uint32_t desired){
    if(queue.version!=ContractVersion)return false;
    if(touchesMon(kind))for(const auto& edit:queue.replacements)
        if(edit.source.partySlot==slot)
            return false;
    return v3::append(snapshot,queue.scalars,kind,slot,field,desired);
}
inline bool appendReplacement(const v2::ReadSnapshot& snapshot,
                              const uint8_t* original,size_t n,
                              const move_v2::Catalog& db,Queue& queue,
                              unsigned party,unsigned moveSlot,uint16_t newId){
    if(queue.version!=ContractVersion ||
       queue.replacements.size()>=MaxReplacements ||
       !move_v2::authenticate(db) ||
       !snapshot.data.valid ||
       queue.scalars.expectedDigest!=snapshot.data.sourceSha256 ||
       queue.scalars.expectedCounter!=snapshot.data.activeCounter)return false;
    for(const auto& edit:queue.scalars.edits)
        if(touchesMon(edit.kind) && edit.slot==party)
            return false;
    for(const auto& edit:queue.replacements)
        if(edit.source.partySlot==party)
            return false;
    move_v2::Request request{};
    if(!move_v2::begin(snapshot,original,n,party,moveSlot,request) ||
       !move_v2::choose(db,request,newId))return false;
    try{queue.replacements.push_back(request);}
    catch(const std::exception&){return false;}
    return true;
}
inline Result preview(const uint8_t* original,size_t n,
                      const Queue& queue){
    if(queue.version!=ContractVersion)
        return blocked("Wrong queued-edit contract version");
    if(!queue.replacements.empty())
        return blocked("Move replacement queue staged only: exact-ROM Apply gate disabled");
    return v3::preview(original,n,queue.scalars);
}
// Informational only. Revalidates immutable source and requested identity
// without producing a candidate. Failure must disable even the Review action.
inline bool verifyStaging(const uint8_t* original,size_t n,
                          const move_v2::Catalog& catalog,const Queue& queue){
    if(queue.version!=ContractVersion ||
       queue.replacements.size()>MaxReplacements ||
       !original || n!=raw_v1::Size ||
       !move_v2::authenticate(catalog))
        return false;
    const auto view=v2::inspect(original,n);
    if(!view.data.valid ||
       view.data.sourceSha256!=queue.scalars.expectedDigest ||
       view.data.activeCounter!=queue.scalars.expectedCounter ||
       queue.scalars.version!=v3::ContractVersion ||
       queue.scalars.edits.size()>v3::MaxEdits)
        return false;
    for(size_t i=0;i<queue.scalars.edits.size();i++){
        const auto& scalar=queue.scalars.edits[i];
        if(!v3::originalGuard(view,queue.scalars,scalar))
            return false;
        for(size_t j=0;j<i;j++)
            if(v3::sameField(scalar,queue.scalars.edits[j]))
                return false;
    }
    for(size_t i=0;i<queue.replacements.size();i++){
        const auto& wanted=queue.replacements[i];
        if(wanted.version!=move_v2::ContractVersion)
            return false;
        move_v2::Request fresh{};
        if(!move_v2::begin(view,original,n,wanted.source.partySlot,
                           wanted.source.moveSlot,fresh))
            return false;
        const auto& a=fresh.source;
        const auto& b=wanted.source;
        if(a.expectedOriginalSha256!=b.expectedOriginalSha256 ||
           a.expectedCounter!=b.expectedCounter ||
           a.partySlot!=b.partySlot || a.moveSlot!=b.moveSlot ||
           a.expectedPid!=b.expectedPid || a.expectedOt!=b.expectedOt ||
           a.expectedSpecies!=b.expectedSpecies ||
           a.expectedPokemonChecksum!=b.expectedPokemonChecksum ||
           a.expectedMoveId!=b.expectedMoveId ||
           a.expectedPP!=b.expectedPP ||
           a.expectedPPBonuses!=b.expectedPPBonuses ||
           wanted.catalogDigest!=move_v2::CatalogSha)
            return false;
        move_v2::Move candidate{};
        if(!move_v2::lookup(catalog,wanted.replacementId,candidate) ||
           candidate.id==b.expectedMoveId)
            return false;
        for(size_t j=0;j<i;j++)
            if(queue.replacements[j].source.partySlot==b.partySlot)
                return false;
        for(const auto& scalar:queue.scalars.edits)
            if(touchesMon(scalar.kind) && scalar.slot==b.partySlot)
                return false;
    }
    return true;
}
// Read-only Phase20 Review screen for exactly ONE staged replacement.
// This is NOT a teachability/legality decision or draft/export approval.
// No mixed v3 scalar edits are accepted in the initial one-move path.
struct SingleMoveReview {
    uint32_t contract=ContractVersion;
    Digest256 sourceSha{},catalogSha=move_v2::CatalogSha{};
    unsigned partySlot=0,moveSlot=0;
    uint16_t previousId=0,replacementId=0;
    uint8_t currentPP=0,catalogBasePP=0,originalPPUps=0;
    bool mayExport=false,mayRestoreInstalledGame=false;
};
inline bool reviewSingle(const uint8_t* original,size_t n,
                         const move_v2::Catalog& db,
                         const Queue& queue,SingleMoveReview& out){
    out=SingleMoveReview{};
    if(queue.replacements.size()!=1 || !queue.scalars.edits.empty() ||
       !verifyStaging(original,n,db,queue))return false;
    const auto& req=queue.replacements.front();
    const auto& source=req.source;
    move_v2::Move move{};
    if(!move_v2::lookup(db,req.replacementId,move))return false;
    raw_v1::Info info{};Mon originalMon{};
    if(!validate(original,n,info) ||
       !partyAt(original,n,info,source.partySlot,originalMon))
        return false;
    for(unsigned slot=0;slot<4;slot++)
        if(slot!=source.moveSlot &&
           originalMon.moves[slot]==req.replacementId)return false;
    out.sourceSha=source.expectedOriginalSha256;
    out.partySlot=source.partySlot;
    out.moveSlot=source.moveSlot;
    out.previousId=source.expectedMoveId;
    out.replacementId=move.id;
    out.currentPP=source.expectedPP;
    out.catalogBasePP=move.basePP;
    out.originalPPUps=move_v1::ppUps(source.expectedPPBonuses,source.moveSlot);
    return true;
}
inline bool mayWriteInstalledGame(const Queue&){return false;}
}} // namespace rogue18::move_queue_v4
