// SPDX-License-Identifier: GPL-3.0-or-later
// Phase18 MOVE-V1. A RESEARCH-ONLY encrypted party move replacement engine.
// The production entrypoint remains permanently fail-closed until a future
// reviewed commit pins a matching installed-ROM fingerprint and move database.
// No filesystem, authenticated VC container, CMAC, or live game API.
#pragma once
#include "integration_v2.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <new>

namespace rogue18 { namespace move_v1 {
constexpr uint32_t ContractVersion=1;
constexpr bool ExactRomMoveTableApproved=false; // NOT user-provided Evidence.
enum class State : uint8_t { BlockedUntilExactRomProven };
struct Capability {
    State state=State::BlockedUntilExactRomProven;
    bool mayPreviewReplacement=false;
    bool mayExportReplacement=false;
    bool mayWriteInstalledGame=false;
};
inline Capability capability(){return {};}

struct Request {
    uint32_t version=ContractVersion;
    Digest256 expectedOriginalSha256{};
    uint32_t expectedCounter=0;
    unsigned partySlot=0,moveSlot=0;
    uint32_t expectedPid=0,expectedOt=0;
    uint16_t expectedSpecies=0,expectedPokemonChecksum=0;
    uint16_t expectedMoveId=0,replacementMoveId=0;
    uint8_t expectedPP=0,expectedPPBonuses=0;
};
// Return the four 2-bit PP-Up counters encoded in Growth substruct byte +8.
// This is an EX-v2.1 source observation, NOT exact private ROM authentication.
inline uint8_t ppUps(uint8_t bonuses,unsigned moveSlot){
    return moveSlot<4 ? uint8_t((bonuses>>(2u*moveSlot))&3u):0u;
}
inline uint8_t clearedSlotBonuses(uint8_t bonuses,unsigned moveSlot){
    return moveSlot<4 ?
        uint8_t(bonuses&uint8_t(~(3u<<(2u*moveSlot)))):bonuses;
}
inline uint8_t maxPPForResearch(uint8_t basePP,uint8_t bonuses,
                                unsigned moveSlot){
    return moveSlot<4 ?
        uint8_t(basePP+(unsigned(basePP)*20u*ppUps(bonuses,moveSlot))/100u):0u;
}
inline bool begin(const v2::ReadSnapshot& snapshot,
                  const uint8_t* bytes,size_t n,
                  unsigned partySlot,unsigned moveSlot,Request& req){
    req=Request{};
    if(!bytes || n!=raw_v1::Size ||
       snapshot.version!=v2::ContractVersion || !snapshot.data.valid ||
       snapshot.data.apiVersion!=ModelApiVersion ||
       !hasDigest(snapshot.data.sourceSha256) ||
       sourceDigest(bytes,n)!=snapshot.data.sourceSha256 ||
       partySlot>=snapshot.data.party.size() || moveSlot>=4)
        return false;
    const auto& mon=snapshot.data.party[partySlot].value;
    if(snapshot.data.party[partySlot].slot!=partySlot ||
       mon.egg || !mon.species || !mon.moves[moveSlot])
        return false;
    raw_v1::Info info{};
    Mon parsed{};
    if(!validate(bytes,n,info) ||
       info.banks[info.active].counter!=snapshot.data.activeCounter ||
       !partyAt(bytes,n,info,partySlot,parsed) ||
       parsed.pid!=mon.pid || parsed.ot!=mon.ot ||
       parsed.species!=mon.species || parsed.checksum!=mon.checksum ||
       parsed.moves!=mon.moves || parsed.pp!=mon.pp)
        return false;
    const size_t at=info.banks[info.active].sector[1]+
                    layout::PartyStart+partySlot*layout::PartyStride;
    const auto plain=plaintext(bytes,at);
    req.expectedOriginalSha256=snapshot.data.sourceSha256;
    req.expectedCounter=snapshot.data.activeCounter;
    req.partySlot=partySlot;req.moveSlot=moveSlot;
    req.expectedPid=mon.pid;req.expectedOt=mon.ot;
    req.expectedSpecies=mon.species;
    req.expectedPokemonChecksum=mon.checksum;
    req.expectedMoveId=mon.moves[moveSlot];
    req.expectedPP=mon.pp[moveSlot];
    req.expectedPPBonuses=plain[8];
    req.replacementMoveId=mon.moves[moveSlot]; // no-op until explicit choice
    return true;
}
// Public Phase20 API: cannot accept caller-supplied "verified" IDs as proof.
// Future exact-ROM admission MUST be audited, immutable, and version bumped.
inline Result preview(const uint8_t*,size_t,const Request&){
    return blocked("Move replacement locked: installed Rogue 2.1.2 ROM move table not authenticated");
}
inline bool verify(const uint8_t*,size_t,const uint8_t*,size_t,const Request&){
    return false;
}
inline bool mayWriteInstalledGame(const Request&){return false;}

#if defined(ROGUE18_SYNTHETIC_MOVE_TEST)
// Build/test ONLY: an explicitly untrusted catalog of fabricated move IDs.
// NEVER call this path from Phase20 or use synthetic candidates as game saves.
struct ResearchMove {
    uint16_t id=0;
    uint8_t basePP=0;
    const char* name=nullptr; // borrowed immutable caller memory
};
inline bool validResearchCatalog(const ResearchMove* entries,size_t count){
    if(!entries || !count || count>256)return false;
    for(size_t i=0;i<count;i++){
        if(!entries[i].id || !entries[i].basePP || entries[i].basePP>64 ||
           !entries[i].name || !entries[i].name[0])return false;
        for(size_t j=0;j<i;j++)
            if(entries[j].id==entries[i].id)return false;
    }
    return true;
}
inline Result previewResearchUnchecked(const uint8_t* raw,size_t n,
                       const Request& req,
                       const ResearchMove* moves,size_t count){
    if(!raw || n!=raw_v1::Size ||
       req.version!=ContractVersion || req.partySlot>=6 ||
       req.moveSlot>=4 || !hasDigest(req.expectedOriginalSha256) ||
       sourceDigest(raw,n)!=req.expectedOriginalSha256 ||
       !validResearchCatalog(moves,count))
        return blocked("Synthetic move: malformed, stale or untrusted input");
    uint8_t replacementBasePP=0;
    for(size_t i=0;i<count;i++)
        if(moves[i].id==req.replacementMoveId)
            replacementBasePP=moves[i].basePP;
    if(!replacementBasePP || !req.replacementMoveId ||
       req.replacementMoveId==req.expectedMoveId)
        return blocked("Synthetic move not on explicit test allowlist or no-op");
    raw_v1::Info originalInfo{};
    Mon originalMon{};
    if(!validate(raw,n,originalInfo) ||
       req.expectedCounter!=originalInfo.banks[originalInfo.active].counter ||
       !partyAt(raw,n,originalInfo,req.partySlot,originalMon) ||
       originalMon.egg || originalMon.pid!=req.expectedPid ||
       originalMon.ot!=req.expectedOt ||
       originalMon.species!=req.expectedSpecies ||
       originalMon.checksum!=req.expectedPokemonChecksum ||
       originalMon.moves[req.moveSlot]!=req.expectedMoveId ||
       originalMon.pp[req.moveSlot]!=req.expectedPP)
        return blocked("Synthetic move original Pokémon identity/slot is stale");
    for(unsigned m=0;m<4;m++)
        if(m!=req.moveSlot && originalMon.moves[m]==req.replacementMoveId)
            return blocked("Duplicate replacement move on same Pokémon");
    const auto& bank=originalInfo.banks[originalInfo.active];
    const size_t s1=bank.sector[1];
    const size_t at=s1+layout::PartyStart+
                    req.partySlot*layout::PartyStride;
    auto canonical=plaintext(raw,at);
    if(canonical[8]!=req.expectedPPBonuses)
        return blocked("Original PP-Up bonus byte stale");

    // Conservative policy: replacing an occupied move resets ONLY that
    // slot's PP Ups to zero and refills PP to catalog-certified base PP.
    // EX-v2.1's SetMonMoveSlot sets base PP but does not clear PP Ups;
    // our stricter reset policy is not authorization for the private ROM.
    canonical[8]=clearedSlotBonuses(canonical[8],req.moveSlot);
    raw_v1::w16(canonical.data(),12u+req.moveSlot*2u,req.replacementMoveId);
    canonical[20u+req.moveSlot]=replacementBasePP;
    Bytes candidate(raw,raw+n);
    encode(candidate.data(),at,canonical);
    raw_v1::w16(candidate.data(),s1+0xff6,raw_v1::checksum(candidate.data()+s1));

    raw_v1::Info afterInfo{};
    Mon afterMon{};
    if(!validate(candidate.data(),candidate.size(),afterInfo) ||
       afterInfo.active!=originalInfo.active ||
       afterInfo.banks[0].counter!=originalInfo.banks[0].counter ||
       afterInfo.banks[1].counter!=originalInfo.banks[1].counter ||
       afterInfo.money!=originalInfo.money ||
       afterInfo.items!=originalInfo.items ||
       afterInfo.partyCount!=originalInfo.partyCount ||
       !partyAt(candidate.data(),candidate.size(),afterInfo,req.partySlot,afterMon))
        return blocked("Synthetic move encrypted H168 roundtrip failed");
    if(afterMon.pid!=originalMon.pid || afterMon.ot!=originalMon.ot ||
       afterMon.species!=originalMon.species ||
       afterMon.heldItem!=originalMon.heldItem ||
       afterMon.ivs!=originalMon.ivs || afterMon.evs!=originalMon.evs ||
       afterMon.abilitySlot!=originalMon.abilitySlot ||
       afterMon.natureFromPid!=originalMon.natureFromPid ||
       afterMon.effectiveNature!=originalMon.effectiveNature ||
       afterMon.gigantamaxFactor!=originalMon.gigantamaxFactor ||
       afterMon.egg!=originalMon.egg ||
       afterMon.moves[req.moveSlot]!=req.replacementMoveId ||
       afterMon.pp[req.moveSlot]!=replacementBasePP)
        return blocked("Synthetic move changed forbidden Pokémon field");
    for(unsigned m=0;m<4;m++)if(m!=req.moveSlot &&
         (afterMon.moves[m]!=originalMon.moves[m] ||
          afterMon.pp[m]!=originalMon.pp[m]))
        return blocked("Synthetic move changed another slot");
    const auto afterCanonical=plaintext(candidate.data(),at);
    if(afterCanonical[8]!=clearedSlotBonuses(req.expectedPPBonuses,req.moveSlot))
        return blocked("PP-Up reset readback failed");

    const size_t aBlock=at+32u+storedIndex(req.expectedPid,'A')*12u;
    const size_t gBlock=at+32u+storedIndex(req.expectedPid,'G')*12u;
    const std::array<size_t,8> permitted{{
        gBlock+8u,aBlock+req.moveSlot*2u,
        aBlock+req.moveSlot*2u+1u,aBlock+8u+req.moveSlot,
        at+28u,at+29u,s1+0xff6,s1+0xff7
    }};
    Result result{};
    result.reason="SYNTHETIC TEST ONLY: never export or restore this candidate";
    for(size_t i=0;i<n;i++)if(raw[i]!=candidate[i]){
        if(std::find(permitted.begin(),permitted.end(),i)==permitted.end())
            return blocked("Synthetic move changed byte outside exact whitelist");
        result.changedOffsets.push_back(i);
    }
    if(result.changedOffsets.empty())return blocked("Synthetic move no-op");
    result.ok=true;
    result.candidate=std::move(candidate);
    return result;
}
inline Result previewResearch(const uint8_t* raw,size_t n,
                  const Request& req,const ResearchMove* moves,size_t count){
    try {return previewResearchUnchecked(raw,n,req,moves,count);}
    catch(const std::bad_alloc&){return blocked("Synthetic move OOM");}
    catch(const std::exception&){return blocked("Synthetic move exception");}
}
inline bool verifyResearch(const uint8_t* before,size_t n,
                          const uint8_t* candidate,size_t candidateSize,
                          const Request& req,
                          const ResearchMove* moves,size_t count){
    if(!before || !candidate || n!=raw_v1::Size || candidateSize!=n)
        return false;
    const auto expected=previewResearch(before,n,req,moves,count);
    return expected.ok && expected.candidate.size()==n &&
           std::memcmp(expected.candidate.data(),candidate,n)==0;
}
// UI research only: compact case-insensitive substring and stable page
// starting at offset, no heap allocation and no guessed ROM correspondence.
inline char asciiLower(char c){return c>='A'&&c<='Z'?char(c+('a'-'A')):c;}
inline bool containsAscii(const char* name,const char* query){
    if(!name || !query)return false;
    if(!query[0])return true;
    for(size_t i=0;name[i];i++){
        size_t j=0;
        while(query[j] && name[i+j] &&
              asciiLower(name[i+j])==asciiLower(query[j]))j++;
        if(!query[j])return true;
    }
    return false;
}
inline size_t searchResearch(const ResearchMove* rows,size_t count,
                  const char* needle,size_t offset,
                  ResearchMove* results,size_t capacity){
    if(!validResearchCatalog(rows,count) || !needle ||
       !results || capacity>12)return 0;
    size_t seen=0,found=0;
    for(size_t i=0;i<count;i++)if(containsAscii(rows[i].name,needle)){
        if(seen++<offset)continue;
        if(found==capacity)break;
        results[found++]=rows[i];
    }
    return found;
}
#endif // synthetic-only
}} // namespace rogue18::move_v1
