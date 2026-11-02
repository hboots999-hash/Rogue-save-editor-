// SPDX-License-Identifier: GPL-3.0-or-later
// Versioned exact-ROM move catalog and offline-only replacement PROTOTYPE.
// Source game bytes are NEVER stored here. Production Apply stays disabled
// until binary-mode, PP-Up and gameplay compatibility evidence is reviewed.
#pragma once
#include "move_replacement_v1.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <new>

namespace rogue18 { namespace move_v2 {
constexpr uint32_t ContractVersion=2;
constexpr size_t RecordCount=936;
constexpr size_t NameBytes=13;
constexpr size_t PackedRowBytes=15; // ROM text[13], ppA, ppB
constexpr size_t PackedBytes=RecordCount*PackedRowBytes;
constexpr bool ApplyApproved=false; // hard gate; no caller override
constexpr Digest256 RomSha{{
    0x11,0x1b,0x00,0x08,0xbc,0xec,0x51,0x9c,
    0x59,0xa0,0x2d,0xe5,0x78,0x95,0xa9,0x24,
    0xfe,0xe5,0xd3,0x63,0x3c,0x8b,0x8f,0xbb,
    0x39,0x4a,0x49,0x71,0x53,0x77,0x8c,0xa3
}};
constexpr Digest256 CatalogSha{{
    0x3d,0x2d,0x63,0xe5,0xfd,0x77,0x37,0x1a,
    0x76,0x54,0x0d,0x07,0xa6,0xc5,0xa4,0xd2,
    0x0a,0x9d,0xda,0xe7,0xac,0xea,0x9f,0x03,
    0x03,0x5d,0x86,0x93,0x6e,0xe3,0x4f,0xa4
}};
struct Catalog {
    const uint8_t* packed=nullptr; // caller-owned local ROM-derived metadata
    size_t size=0;
    Digest256 romSha{}; // claimed local dump SHA; not proof of installed game
};
inline bool authenticate(const Catalog& db){
    return db.packed && db.size==PackedBytes &&
           db.romSha==RomSha &&
           sourceDigest(db.packed,db.size)==CatalogSha;
}
inline uint8_t asciiLower(uint8_t x){
    return (x>='A' && x<='Z')?uint8_t(x+32):x;
}
inline bool textKnown(const uint8_t* name){
    if(!name)return false;
    bool any=false,ended=false;
    for(size_t i=0;i<NameBytes;i++){
        if(ended)continue;
        const uint8_t b=name[i];
        if(b==0xff){ended=true;continue;}
        if(b==0 || (b>=0xbb && b<=0xee) ||
           b==0xae || b==0xb4 || b==0xa3){any=true;continue;}
        return false;
    }
    return any && ended;
}
struct Move {
    uint16_t id=0;
    uint8_t basePP=0;
    uint8_t rawName[NameBytes]{};
};
inline bool lookupAuthenticated(const Catalog& db,uint16_t id,Move& out){
    out=Move{};
    // Internal helper: caller has already pinned the entire packed digest.
    if(id==0 || id>848 || id==165)return false;
    const uint8_t* p=db.packed+size_t(id)*PackedRowBytes;
    if(!textKnown(p) || p[13]==0 || p[13]>64 || p[13]!=p[14])
        return false; // conflicting, special, unknown & disabled IDs
    out.id=id;out.basePP=p[13];
    std::memcpy(out.rawName,p,NameBytes);
    return true;
}
inline bool lookup(const Catalog& db,uint16_t id,Move& out){
    out=Move{};
    return authenticate(db) && lookupAuthenticated(db,id,out);
}
// Safe read-only fixed-page UI iteration, no guessable arbitrary IDs or heap.
inline size_t page(const Catalog& db,size_t offset,Move* out,size_t capacity){
    if(!out || capacity>12 || !authenticate(db))return 0;
    size_t seen=0,produced=0;
    for(unsigned id=1;id<=848;id++){
        Move row{};
        // One catalog SHA256 per page, not 848 repeated 14 KiB digests.
        // Caller must hold the authenticated immutable buffer while paging.
        if(!lookupAuthenticated(db,uint16_t(id),row))continue;
        if(seen++<offset)continue;
        if(produced>=capacity)break;
        out[produced++]=row;
    }
    return produced;
}
struct Request {
    uint32_t version=ContractVersion;
    move_v1::Request source{};
    uint16_t replacementId=0;
    Digest256 catalogDigest=CatalogSha;
};
// UI can select read-only move; cannot create a release-approved draft.
inline bool begin(const v2::ReadSnapshot& view,const uint8_t* bytes,size_t n,
                  unsigned partySlot,unsigned moveSlot,Request& out){
    out=Request{};
    return move_v1::begin(view,bytes,n,partySlot,moveSlot,out.source);
}
inline bool choose(const Catalog& db,Request& req,uint16_t id){
    Move row{};
    if(req.version!=ContractVersion || !lookup(db,id,row) ||
       id==req.source.expectedMoveId)return false;
    req.replacementId=id;
    return true;
}
// No user-provided 'verified IDs', PP modes, or booleans can unlock this.
// A future audited version must validate installed-ROM identity independently.
inline Result preview(const uint8_t*,size_t,const Catalog&,const Request&){
    return blocked("Exact ROM catalog found but PP-table mode and replacement behaviour are not certified");
}
inline bool verify(const uint8_t*,size_t,const uint8_t*,size_t,
                   const Catalog&,const Request&){return false;}
inline bool mayRestoreInstalledGame(const Request&){return false;}

namespace detail {
// Offline pure-memory engine. Intentionally not reachable from preview().
// Compiled for ARM11 and exercised with generated synthetic save fixtures;
// a new reviewed production contract is needed before this can be unlocked.
inline Result construct(const uint8_t* raw,size_t n,
                        const move_v1::Request& req,
                        uint16_t replacement,uint8_t basePP){
    if(!raw || n!=raw_v1::Size || !hasDigest(req.expectedOriginalSha256) ||
       sourceDigest(raw,n)!=req.expectedOriginalSha256 ||
       req.version!=move_v1::ContractVersion || req.partySlot>=6 ||
       req.moveSlot>=4 || replacement==0 ||
       replacement==req.expectedMoveId || basePP<1 || basePP>64)
        return blocked("Candidate guards, bounds, identity or source SHA invalid");
    raw_v1::Info before{};
    Mon old{};
    if(!validate(raw,n,before) ||
       before.banks[before.active].counter!=req.expectedCounter ||
       !partyAt(raw,n,before,req.partySlot,old) || old.egg ||
       old.pid!=req.expectedPid || old.ot!=req.expectedOt ||
       old.species!=req.expectedSpecies ||
       old.checksum!=req.expectedPokemonChecksum ||
       old.moves[req.moveSlot]!=req.expectedMoveId ||
       old.pp[req.moveSlot]!=req.expectedPP)
        return blocked("Pokémon checksum, slot or original guard mismatch");
    for(unsigned slot=0;slot<4;slot++)
        if(slot!=req.moveSlot && old.moves[slot]==replacement)
            return blocked("Replacement duplicates another current move");
    const auto& bank=before.banks[before.active];
    const size_t s1=bank.sector[1];
    const size_t at=s1+layout::PartyStart+req.partySlot*layout::PartyStride;
    auto plain=plaintext(raw,at);
    if(plain[8]!=req.expectedPPBonuses)
        return blocked("Original PP-Up byte changed");
    const auto oldPlain=plain;
    plain[8]=move_v1::clearedSlotBonuses(plain[8],req.moveSlot);
    raw_v1::w16(plain.data(),12+2*req.moveSlot,replacement);
    plain[20+req.moveSlot]=basePP;
    Bytes candidate(raw,raw+n);
    encode(candidate.data(),at,plain);
    raw_v1::w16(candidate.data(),s1+0xff6,
                raw_v1::checksum(candidate.data()+s1));
    raw_v1::Info after{};
    Mon revised{};
    if(!validate(candidate.data(),candidate.size(),after) ||
       !partyAt(candidate.data(),candidate.size(),after,req.partySlot,revised) ||
       after.active!=before.active || after.money!=before.money ||
       after.items!=before.items || after.partyCount!=before.partyCount ||
       after.banks[0].counter!=before.banks[0].counter ||
       after.banks[1].counter!=before.banks[1].counter)
        return blocked("Final active and inactive bank validation failed");
    if(revised.pid!=old.pid || revised.ot!=old.ot ||
       revised.species!=old.species ||
       revised.heldItem!=old.heldItem || revised.ivs!=old.ivs ||
       revised.evs!=old.evs || revised.abilitySlot!=old.abilitySlot ||
       revised.effectiveNature!=old.effectiveNature ||
       revised.gigantamaxFactor!=old.gigantamaxFactor ||
       revised.moves[req.moveSlot]!=replacement ||
       revised.pp[req.moveSlot]!=basePP)
        return blocked("Replacement altered unrelated Pokémon fields");
    for(unsigned i=0;i<4;i++)
        if(i!=req.moveSlot &&
           (revised.moves[i]!=old.moves[i] || revised.pp[i]!=old.pp[i]))
            return blocked("Another move slot changed");
    // Validate every plaintext payload byte outside the exact three
    // intended fields before considering the encrypted reconstruction.
    const auto fresh=plaintext(candidate.data(),at);
    if(fresh!=plain)return blocked("Encrypted Pokémon reconstruction mismatch");
    for(unsigned i=0;i<48;i++){
        const bool changedAllowed=i==8 ||
            i==12+2*req.moveSlot || i==13+2*req.moveSlot ||
            i==20+req.moveSlot;
        if(!changedAllowed && fresh[i]!=oldPlain[i])
            return blocked("Unrelated decrypted field drift");
    }
    const size_t aBlock=at+32+storedIndex(old.pid,'A')*12;
    const size_t gBlock=at+32+storedIndex(old.pid,'G')*12;
    const std::array<size_t,8> whitelist{{
        gBlock+8,aBlock+2*req.moveSlot,aBlock+2*req.moveSlot+1,
        aBlock+8+req.moveSlot,at+28,at+29,s1+0xff6,s1+0xff7
    }};
    Result result{};
    result.reason="Exact-ROM core candidate ONLY; release Apply remains disabled";
    for(size_t i=0;i<n;i++)if(raw[i]!=candidate[i]){
        if(std::find(whitelist.begin(),whitelist.end(),i)==whitelist.end())
            return blocked("Byte outside exact 8-byte union was modified");
        result.changedOffsets.push_back(i);
    }
    if(result.changedOffsets.empty())return blocked("Replacement no-op");
    result.ok=true;result.candidate=std::move(candidate);
    return result;
}
} // namespace detail
#if defined(ROGUE18_TEST_MOVE_V2)
// Test-only entrypoints: intentionally cannot authenticate a real catalog.
inline Result previewSynthetic(const uint8_t* before,size_t n,
                               const Request& req,uint16_t replacement,
                               uint8_t fabricatedPP){
    try{return detail::construct(before,n,req.source,replacement,fabricatedPP);}
    catch(const std::exception&){return blocked("Research allocation/exception");}
}
inline bool verifySynthetic(const uint8_t* before,size_t n,
                            const uint8_t* draft,size_t len,
                            const Request& req,uint16_t id,uint8_t pp){
    if(!draft || len!=n)return false;
    const auto expected=previewSynthetic(before,n,req,id,pp);
    return expected.ok && expected.candidate.size()==n &&
           std::memcmp(expected.candidate.data(),draft,n)==0;
}
#endif
}} // namespace rogue18::move_v2
