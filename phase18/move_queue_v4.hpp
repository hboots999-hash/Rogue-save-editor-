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
inline bool appendScalar(const v2::ReadSnapshot& snapshot,Queue& queue,
                         Kind kind,unsigned slot,unsigned field,uint32_t desired){
    if(queue.version!=ContractVersion)return false;
    if(kind==Kind::Move)for(const auto& edit:queue.replacements)
        if(edit.source.partySlot==slot && edit.source.moveSlot==field)
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
        if(edit.kind==Kind::Move && edit.slot==party && edit.field==moveSlot)
            return false;
    for(const auto& edit:queue.replacements)
        if(edit.source.partySlot==party && edit.source.moveSlot==moveSlot)
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
inline bool mayWriteInstalledGame(const Queue&){return false;}
}} // namespace rogue18::move_queue_v4
