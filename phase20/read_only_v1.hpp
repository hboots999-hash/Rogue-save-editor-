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
    // Independent Phase18 model guards and Phase19 presentation guards.
    // Both must pass BEFORE changing either of the caller-visible outputs.
    rogue18::SaveView model{};
    try { model=rogue18::inspectReadOnly(raw,n,false); }
    catch(const std::bad_alloc&) {
        reason="Insufficient memory for copied-save model.";return false;
    }
    if(!model.valid || model.apiVersion!=rogue18::ModelApiVersion ||
       model.confidence!=rogue18::DataConfidence::StructurallyValidated ||
       !rogue18::hasDigest(model.sourceSha256) ||
       !model.capabilities.copiedSaveOnly || model.capabilities.liveWrite ||
       model.capabilities.restore || model.capabilities.pcEdit ||
       model.party.size()>6 || model.items.size()>450) {
        reason="Phase18 copied-save model rejected the data or capabilities.";
        return false;
    }
    // Deliberately static to prevent a large Phase19 snapshot on ARM11 stack.
    // Non-reentrant (the 3DS app uses a single UI thread); only copied values
    // enter visible, and never on an unsuccessful import.
    static phase19::Demo checked{};
    if(!phase19::inspectCopiedBuffer(raw,n,checked,reason))return false;
    if(checked.synthetic || checked.sourceModelApi!=model.apiVersion ||
       checked.sourceSha256!=model.sourceSha256 ||
       checked.activeCounter!=model.activeCounter ||
       checked.money!=model.money ||
       checked.partyCount!=model.party.size() ||
       checked.bagCount!=model.items.size() ||
       checked.pcStatus!=phase19::Availability::Unsupported ||
       checked.backupStatus!=phase19::Availability::Unverified) {
        reason="Phase18/Phase19 read-only model agreement failure.";
        return false;
    }
    for(size_t i=0;i<model.party.size();++i) {
        const auto& a=model.party[i].value;
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
    for(size_t i=0;i<model.items.size();++i)
        if(checked.bag[i].id!=model.items[i].itemId ||
           checked.bag[i].quantity!=model.items[i].quantity) {
            reason="Phase18/Phase19 bag model agreement failure.";
            return false;
        }
    CopyObservation next{};
    next.modelVersion=model.apiVersion;
    next.rawBytes=static_cast<uint32_t>(n);
    next.activeCounter=model.activeCounter;
    next.sourceSha256=model.sourceSha256;
    next.money=model.money;
    next.party=static_cast<unsigned>(model.party.size());
    next.items=static_cast<unsigned>(model.items.size());
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
