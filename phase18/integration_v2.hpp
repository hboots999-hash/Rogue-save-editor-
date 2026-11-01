// Stable, versioned read-only/offline preview contract for Phase20.
// No Phase17/14 wrapper/container types. No global h168 namespace.
// All exports remain separate SD COPY drafts; restored/installed writes off.
#pragma once
#include "editor_model.hpp"
#include <array>
#include <cstdint>

namespace rogue18 { namespace v2 {
constexpr uint32_t ContractVersion=2;
enum class Requirement : uint8_t {
    StructurallyValidatedCopy, MatchingRomMoveIdMap,
    MatchingRomAbilityMap, MatchingRomNatureSemantics,
    MatchingRomPocketAndCapacityMap, NotSupported
};
enum class Support : uint8_t {
    OfflinePreview, ResearchOnlyBlocked, Unsupported
};
struct Capability {
    Kind operation;
    Support support;
    Requirement requirement;
    bool canCreateOfflineDraft;
    bool canRestoreInstalledGame;
};
inline Capability capability(Kind kind) {
    switch(kind) {
    case Kind::Money:
    case Kind::ExistingQuantity:
    case Kind::IVs:
    case Kind::EVs:
        return {kind,Support::OfflinePreview,
                Requirement::StructurallyValidatedCopy,true,false};
    case Kind::Move:
        // Existing move PP only. New IDs require a matching-ROM map.
        return {kind,Support::OfflinePreview,
                Requirement::StructurallyValidatedCopy,true,false};
    case Kind::AbilitySlot:
        return {kind,Support::ResearchOnlyBlocked,
                Requirement::MatchingRomAbilityMap,false,false};
    case Kind::Nature:
        return {kind,Support::ResearchOnlyBlocked,
                Requirement::MatchingRomNatureSemantics,false,false};
    case Kind::InsertItem:
        return {kind,Support::ResearchOnlyBlocked,
                Requirement::MatchingRomPocketAndCapacityMap,false,false};
    }
    return {kind,Support::Unsupported,Requirement::NotSupported,false,false};
}
inline bool availableForOfflinePreview(const Request& req) {
    if(!capability(req.kind).canCreateOfflineDraft)return false;
    if(req.kind==Kind::Move && req.moveId!=req.expectedMoveId)return false;
    return true;
}
inline const char* evidenceText(Kind kind){
    switch(kind){
    case Kind::Money:return "131072-byte structurally valid copied H168, SHA256 snapshot and expected money";
    case Kind::ExistingQuantity:return "Copied H168 + SHA256, occupied slot, item ID and existing quantity";
    case Kind::IVs:return "Copied H168 + SHA256, PID/OT/species/checksum; unchanged encrypted header";
    case Kind::EVs:return "Copied H168 + SHA256, PID/OT/species/checksum, six 0..252 EVs total <=510";
    case Kind::Move:return "Copied H168 + SHA256, existing move ID/PP; replacement ID blocked";
    case Kind::AbilitySlot:return "Exact installed-ROM species/ability mapping absent: blocked";
    case Kind::Nature:return "Exact installed-ROM nature/stat semantics absent: blocked";
    case Kind::InsertItem:return "Exact installed-ROM item pocket, upgrade and ID mapping absent: blocked";
    }
    return "Unsupported operation";
}
enum class LegacyPreflightShape : uint8_t {
    None, MoneyPlusOne, ExistingQuantityPlusOne
};
// A shape comparison is INFORMATIONAL, not a Phase14 preflight pass.
// The 263168-byte authenticated wrapper, CMAC, journal and restore
// authorization are NEVER available from this contract.
inline LegacyPreflightShape legacyPreflightShape(const Request& req){
    if(req.kind==Kind::Money &&
       req.expectedMoney<999999u && req.money==req.expectedMoney+1u)
        return LegacyPreflightShape::MoneyPlusOne;
    if(req.kind==Kind::ExistingQuantity &&
       req.expectedItemId!=0 && req.expectedQuantity>0 &&
       req.expectedQuantity<999u &&
       req.quantity==uint32_t(req.expectedQuantity)+1u)
        return LegacyPreflightShape::ExistingQuantityPlusOne;
    return LegacyPreflightShape::None;
}
inline bool mayRestoreInstalledGame(const Request&){return false;}

struct ReadSnapshot {
    uint32_t version=ContractVersion;
    SaveView data{};
};
inline ReadSnapshot inspect(const uint8_t* copied,size_t size){
    ReadSnapshot out{};
    // Deliberately do not request speculative PC inspection for Phase20.
    out.data=inspectReadOnly(copied,size,false);
    return out;
}
inline bool begin(const ReadSnapshot& view,Kind kind,unsigned slot,Request& request){
    request=Request{};
    if(view.version!=ContractVersion || !view.data.valid ||
       !capability(kind).canCreateOfflineDraft)return false;
    if(!beginRequest(view.data,kind,slot,request))return false;
    if(kind==Kind::Move && !beginMoveRequest(view.data,slot,0,request))
        return false;
    return hasDigest(request.expectedDigest);
}
inline bool beginMove(const ReadSnapshot& view,unsigned partySlot,
                      unsigned moveSlot,Request& request){
    request=Request{};
    return view.version==ContractVersion &&
           view.data.valid &&
           beginMoveRequest(view.data,partySlot,moveSlot,request) &&
           hasDigest(request.expectedDigest);
}
// Call only on an explicitly reviewed, user-imported raw H168 copy.
// Never accept arbitrary Evidence. Synthetic research code must call
// rogue18::propose separately, and those drafts are NOT production exports.
inline Result preview(const uint8_t* original,size_t size,const Request& req){
    if(!hasDigest(req.expectedDigest))
        return blocked("Phase20 requires SHA-256 fingerprint of read-only copied save");
    if(!availableForOfflinePreview(req))
        return blocked("Operation requires unverified ROM metadata or is unsupported");
    return propose(original,size,req,Evidence{});
}
inline bool verify(const uint8_t* before,size_t beforeLength,
                   const uint8_t* draft,size_t draftLength,
                   const Request& req){
    if(!hasDigest(req.expectedDigest) || !availableForOfflinePreview(req))
        return false;
    return verifyExact(before,beforeLength,draft,draftLength,req,Evidence{});
}
}} // namespace rogue18::v2
