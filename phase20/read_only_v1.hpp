// SPDX-License-Identifier: GPL-3.0-or-later
// PKSM Rogue Phase20: separate raw copied-save read-only + recovery STATUS boundary.
// No installed-title access, editor proposals, export, save writes or restoration.
// Phase17 producer remains a separately compiled TU; only its value-only C ABI is seen.
#pragma once
#include "phase20/compat_phase19_v1.hpp"
#include "readonly_status_v1.h"
#include "phase19/read_only_import.hpp"
#include "phase19/ui.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <new>
#include <string>

namespace phase20 {
constexpr uint32_t ReadOnlyBridgeVersion = 1u;
constexpr size_t CopiedRawBytes = rogue18::raw_v1::Size;
constexpr uint32_t AuthenticatedContainerBytes = 263168u;
constexpr uint32_t RehearsalJournalBytes = 526400u;
static_assert(CopiedRawBytes == 131072u, "Raw-save format unexpectedly changed");
static_assert(sizeof(phase19::State) < 256u, "3DS UI state exceeds safe stack budget");

// Raw copied-save identities and Phase17 authenticated CONTAINER identities
// are NEVER treated as the same object or as a conversion to a live save.
struct CopyObservation {
    uint32_t bridgeVersion=ReadOnlyBridgeVersion;
    uint32_t modelVersion=0;
    uint32_t rawBytes=0;
    uint32_t activeCounter=0;
    std::array<uint8_t,32> sourceSha256{};
    unsigned money=0,party=0,items=0;
    bool structurallyReadable=false;
    bool romIdentityVerified=false;
    bool canModifyInstalledGame=false;
    bool canRestore=false;
};
enum class RecoveryLevel : uint8_t {
    InvalidEnvelope, NotChecked, BlockedOrMissing, RamRehearsalClaimOnly
};
struct RecoveryObservation {
    RecoveryLevel level=RecoveryLevel::InvalidEnvelope;
    uint32_t reportCode=P17_RO_NOT_CHECKED;
    bool canModifyInstalledGame=false;
    bool canRestore=false;
};

inline RecoveryObservation inspectRecoveryStatus(const P17ReadOnlyStatusV1* report) {
    RecoveryObservation r{};
    if(!p17_readonly_status_shape_valid_v1(report))return r;
    r.reportCode=report->code;
    if(report->code==P17_RO_NOT_CHECKED)r.level=RecoveryLevel::NotChecked;
    else if(report->code==P17_RO_RAM_REHEARSAL_READABLE) {
        // A structurally valid report is NOT a cryptographic verification.
        // It only describes the Phase17 producer's RAM-only inspection.
        bool nonzero=false;
        for(uint8_t c:report->source_sha256)nonzero|=(c!=0);
        r.level=(report->expected_title && nonzero &&
                 report->container_bytes==AuthenticatedContainerBytes &&
                 report->journal_bytes==RehearsalJournalBytes)
               ?RecoveryLevel::RamRehearsalClaimOnly:RecoveryLevel::InvalidEnvelope;
    }else r.level=RecoveryLevel::BlockedOrMissing;
    return r;
}
inline bool inspectCopy(const uint8_t* raw,size_t n,phase19::Demo& visible,
                        CopyObservation& summary,std::string& reason) {
    if(!raw || n!=CopiedRawBytes) {
        reason="Phase20 accepts only exact 131072-byte offline copied saves.";
        return false;
    }
    // Lightweight Phase18 low-level validation only. NEVER pull the
    // experimental editor_model/pc_readonly headers into native ARM11:
    // a prior Phase19 crash investigation exposed a 37-KiB PC stack frame.
    h168::Info info{};
    if(!rogue18::validate(raw,n,info) || !info.valid ||
       info.partyCount>6u || info.items>450u || info.money>999999u) {
        reason="Phase18 read-only decoder rejected sector/party/bag data.";
        return false;
    }
    const auto digest=rogue18::sourceDigest(raw,n);
    if(!rogue18::hasDigest(digest)) {
        reason="Copied-save SHA-256 was invalid.";return false;
    }
    // 3DS event loop is single-threaded: stage large DTO in static storage.
    // Neither displayed snapshot nor summary changes on a failed check.
    static phase19::Demo checked{};
    if(!phase19::inspectCopiedBuffer(raw,n,checked,reason))return false;
    const auto counter=info.banks[info.active].counter;
    if(checked.synthetic ||
       checked.sourceModelApi!=phase19::Phase18ReadModelApiV1 ||
       checked.sourceSha256!=digest ||
       checked.activeCounter!=counter ||
       checked.money!=info.money ||
       checked.partyCount!=info.partyCount ||
       checked.bagCount!=info.items ||
       checked.pcStatus!=phase19::Availability::Unsupported ||
       checked.backupStatus!=phase19::Availability::Unverified) {
        reason="Phase18/Phase19 read-only model agreement failure.";
        return false;
    }
    for(unsigned i=0;i<info.partyCount;++i) {
        rogue18::Mon a{};
        if(!rogue18::partyAt(raw,n,info,i,a)) {
            reason="Read-only party decoding failed independently.";return false;
        }
        const auto& b=checked.party[i];
        if(b.species!=a.species || b.ability!=a.abilitySlot ||
           b.nature!=a.effectiveNature || b.egg!=a.egg) {
            reason="Phase18/Phase19 party model agreement failure.";return false;
        }
        for(unsigned j=0;j<6;j++)
            if(b.iv[j]!=a.ivs[j] || b.ev[j]!=a.evs[j]) {
                reason="Phase18/Phase19 party stat agreement failure.";return false;
            }
        for(unsigned j=0;j<4;j++)
            if(b.moves[j]!=a.moves[j] || b.pp[j]!=a.pp[j]) {
                reason="Phase18/Phase19 move/PP agreement failure.";return false;
            }
    }
    for(unsigned i=0;i<info.items;++i) {
        unsigned id=0,quantity=0;
        if(!h168::item(raw,info,i,id,quantity) ||
           checked.bag[i].id!=id || checked.bag[i].quantity!=quantity) {
            reason="Phase18/Phase19 bag model agreement failure.";return false;
        }
    }
    CopyObservation next{};
    next.modelVersion=phase19::Phase18ReadModelApiV1;
    next.rawBytes=static_cast<uint32_t>(n);
    next.activeCounter=counter;
    next.sourceSha256=digest;
    next.money=info.money;
    next.party=static_cast<unsigned>(info.partyCount);
    next.items=static_cast<unsigned>(info.items);
    next.structurallyReadable=true;
    // Even the successful read-only import grants NO authority to edit.
    visible=checked;
    summary=next;
    reason="Phase20: copied H168 structure readable; ROM identity unverified; NO WRITES.";
    return true;
}

inline bool inspectStream(std::FILE* file,phase19::Demo& visible,
                          CopyObservation& summary,std::string& reason) {
    if(!file) {reason="No copied-save file stream.";return false;}
    if(std::fseek(file,0,SEEK_END)!=0){reason="Cannot inspect copy length.";return false;}
    const long n=std::ftell(file);
    if(n!=static_cast<long>(CopiedRawBytes)) {
        reason=n==static_cast<long>(AuthenticatedContainerBytes)
              ?"Authenticated VC container is NOT a copied raw save."
              :"Wrong copied-save size or failed file length.";
        return false;
    }
    if(std::fseek(file,0,SEEK_SET)!=0){reason="Cannot rewind copied save.";return false;}
    std::unique_ptr<uint8_t[]> data(new(std::nothrow) uint8_t[CopiedRawBytes]);
    if(!data){reason="Not enough 3DS memory for copied-save read.";return false;}
    if(std::fread(data.get(),1,CopiedRawBytes,file)!=CopiedRawBytes ||
       std::ferror(file) || std::fgetc(file)!=EOF || std::ferror(file)) {
        reason="Short/changed/failed SD copy read.";return false;
    }
    return inspectCopy(data.get(),CopiedRawBytes,visible,summary,reason);
}
inline bool inspectFixedSDCopy(phase19::Demo& visible,CopyObservation& summary,
                               std::string& reason) {
    std::unique_ptr<std::FILE,int(*)(std::FILE*)> file(
        std::fopen(phase19::OfflineImportPath,"rb"),&std::fclose);
    if(!file){reason="Offline SD copy absent or unreadable; no game save opened.";return false;}
    return inspectStream(file.get(),visible,summary,reason);
}
} // namespace phase20
