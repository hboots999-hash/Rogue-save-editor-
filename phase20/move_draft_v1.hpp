// SPDX-License-Identifier: GPL-3.0-or-later
// Phase20 opt-in EXPERIMENTAL one-move SD-copy exporter.
// This adapter does not open the installed 3DS game, CMAC, or restore archives.
#pragma once
#include "phase20/offline_edit_v1.hpp"
#include "phase20/move_catalog_bridge_v1.hpp"
#include "phase18/move_queue_v4.hpp"
#include "move_browser.hpp"
#include <cstdio>
#include <cstring>
#include <new>
#include <exception>
#include <sys/stat.h>

namespace phase20 {
enum class MoveDraftStage : uint8_t { Idle, ConfirmOne, ConfirmTwo, Result };
struct MoveDraftFlow {
    MoveDraftStage stage=MoveDraftStage::Idle;
    phase19::MoveSelectionIntentV1 intent{};
    rogue18::move_queue_v4::SingleMoveReview review{};
    bool succeeded=false;
    char message[256]{};
};
static_assert(!rogue18::move_v3::MayWriteInstalledGame,
              "Move V3 must never write installed GBA VC saves");
static_assert(!rogue18::move_v2::ApplyApproved,
              "Research catalog is not an installed-game Apply permission");

inline void moveMessage(MoveDraftFlow& f,const char* reason) {
    std::snprintf(f.message,sizeof(f.message),"%s",reason?reason:"Unknown error");
}
inline bool checkedMoveQueue(const phase19::Demo& display,
                             const CopyObservation& observed,
                             const LocalMoveProvider& catalog,
                             const phase19::MoveSelectionIntentV1& intent,
                             const uint8_t* source,size_t bytes,
                             rogue18::move_queue_v4::Queue& queue,
                             rogue18::move_queue_v4::SingleMoveReview& review) {
    if(!source || bytes!=CopiedRawBytes || display.synthetic ||
       !observed.structurallyReadable || observed.canRestore ||
       observed.canModifyInstalledGame || !catalog.loaded ||
       intent.version!=phase19::MoveSelectionIntentVersion ||
       !intent.candidatePPMatchesTables ||
       intent.catalogRomSha256!=rogue18::move_v2::RomSha ||
       !rogue18::move_v3::selectedOrdinary(
           static_cast<uint16_t>(intent.replacementMoveId)) ||
       intent.replacementMoveId>65535u ||
       intent.partySlot>=display.partyCount || intent.moveSlot>=4u ||
       display.sourceSha256!=intent.sourceSha256 ||
       observed.sourceSha256!=intent.sourceSha256 ||
       display.activeCounter!=intent.sourceCounter ||
       observed.activeCounter!=intent.sourceCounter)
        return false;
    const auto snapshot=rogue18::v2::inspect(source,bytes);
    if(!sameVisibleSource(display,observed,snapshot) ||
       intent.partySlot>=snapshot.data.party.size()) return false;
    const auto& displayed=display.party[intent.partySlot];
    if(displayed.egg || displayed.species!=intent.species ||
       displayed.moves[intent.moveSlot]!=intent.originalMoveId ||
       displayed.pp[intent.moveSlot]!=intent.originalPP) return false;
    const rogue18::move_v2::Catalog authenticated{
        catalog.packed.data(),catalog.packed.size(),rogue18::move_v2::RomSha};
    if(!rogue18::move_v2::authenticate(authenticated) ||
       !rogue18::move_queue_v4::begin(snapshot,queue) ||
       !rogue18::move_queue_v4::appendReplacement(snapshot,source,bytes,
           authenticated,queue,intent.partySlot,intent.moveSlot,
           static_cast<uint16_t>(intent.replacementMoveId)) ||
       !rogue18::move_queue_v4::reviewSingle(source,bytes,authenticated,queue,review) ||
       !review.mayExportExperimentalDraft || review.mayExport ||
       review.mayRestoreInstalledGame ||
       review.previousId!=intent.originalMoveId ||
       review.replacementId!=intent.replacementMoveId ||
       review.currentPP!=intent.originalPP ||
       review.sourceSha!=intent.sourceSha256)
        return false;
    const auto candidate=rogue18::move_queue_v4::previewSingleMoveDraft(
        source,bytes,authenticated,queue);
    return candidate.ok && candidate.candidate.size()==bytes;
}
inline bool beginExperimentalMoveDraft(MoveDraftFlow& f,
                                      const phase19::MoveBrowser& browser,
                                      const phase19::Demo& display,
                                      const CopyObservation& observed,
                                      const LocalMoveProvider& catalog) {
    f=MoveDraftFlow{};
    f.stage=MoveDraftStage::Result;
    try {
        if(display.synthetic || !catalog.loaded ||
           !phase19::describeMoveSelection(browser,display,f.intent)) {
            moveMessage(f,"Selection not verified. No move draft created.");
            return false;
        }
        auto source=rogue18::raw_v1::readExact(phase19::OfflineImportPath);
        if(!source) {
            moveMessage(f,"Copied 128KiB save missing or unreadable.");
            return false;
        }
        rogue18::move_queue_v4::Queue queued{};
        if(!checkedMoveQueue(display,observed,catalog,f.intent,
                            source.get(),CopiedRawBytes,queued,f.review)) {
            moveMessage(f,"Not one of 43 reviewed moves, or source/catalog rejected.");
            return false;
        }
        f.stage=MoveDraftStage::ConfirmOne;
        moveMessage(f,"Experimental SD-only move draft. NO game Apply.");
        return true;
    } catch(const std::exception&) {
        moveMessage(f,"Memory/validation exception: no draft approved.");
        return false;
    }
}
inline bool availableToken(char (&token)[24]) {
    // Reserve names via the Phase18 O_EXCL writer, never overwrite anything.
    // A failed write after claiming a name is reported and not retried.
    for(unsigned i=1;i<=999;i++) {
        std::snprintf(token,sizeof(token),"trial_%03u",i);
        char original[160]{},draft[160]{};
        const int a=std::snprintf(original,sizeof(original),
            "%s/move_%s_backup.sav",rogue18::move_export_v3::FixedSdDraftDir,token);
        const int b=std::snprintf(draft,sizeof(draft),
            "%s/move_%s_draft.sav",rogue18::move_export_v3::FixedSdDraftDir,token);
        if(a<0 || b<0 || size_t(a)>=sizeof(original) || size_t(b)>=sizeof(draft))
            return false;
        struct stat st{};
        if(::stat(original,&st)!=0 && ::stat(draft,&st)!=0)
            return true;
    }
    return false;
}
inline bool finishExperimentalMoveDraft(MoveDraftFlow& f,
                                        const phase19::Demo& display,
                                        const CopyObservation& observed,
                                        LocalMoveProvider& catalog) {
    f.stage=MoveDraftStage::Result;
    f.succeeded=false;
    try {
        // Reopen both independently controlled SD inputs at final confirmation.
        // Changing a file after the preview never silently exports stale data.
        if(!catalog.read()) {
            moveMessage(f,"ROM metadata missing/changed; draft refused.");
            return false;
        }
        auto source=rogue18::raw_v1::readExact(phase19::OfflineImportPath);
        if(!source) {
            moveMessage(f,"Copied save missing/changed; draft refused.");
            return false;
        }
        rogue18::move_queue_v4::Queue queued{};
        rogue18::move_queue_v4::SingleMoveReview fresh{};
        if(!checkedMoveQueue(display,observed,catalog,f.intent,
                            source.get(),CopiedRawBytes,queued,fresh) ||
           fresh.sourceSha!=f.review.sourceSha ||
           fresh.partySlot!=f.review.partySlot ||
           fresh.moveSlot!=f.review.moveSlot ||
           fresh.previousId!=f.review.previousId ||
           fresh.replacementId!=f.review.replacementId ||
           fresh.catalogBasePP!=f.review.catalogBasePP) {
            moveMessage(f,"Import, move or catalogue changed; export refused.");
            return false;
        }
        char token[24]{};
        if(!availableToken(token)) {
            moveMessage(f,"No unused draft filename; inspect /drafts/ folder.");
            return false;
        }
        const rogue18::move_v2::Catalog db{
            catalog.packed.data(),catalog.packed.size(),rogue18::move_v2::RomSha};
        rogue18::move_export_v3::Receipt receipt{};
        if(!rogue18::move_queue_v4::exportSingleMoveDraft(
                source.get(),CopiedRawBytes,db,queued,token,receipt) ||
           !receipt.ok) {
            moveMessage(f,"Export failed. Inspect SD /drafts/ (original untouched).");
            return false;
        }
        f.succeeded=true;
        std::snprintf(f.message,sizeof(f.message),
            "Created move_%s_draft.sav (and new backup). %u -> %u. NOT game-tested.",
            token,unsigned(receipt.previousMove),unsigned(receipt.newMove));
        return true;
    } catch(const std::exception&) {
        moveMessage(f,"Export exception. Inspect SD copies; no game write.");
        return false;
    }
}
inline void moveDraftInput(MoveDraftFlow& f,bool confirm,bool cancel,
                           const phase19::Demo& display,
                           const CopyObservation& observed,
                           LocalMoveProvider& catalog) {
    if(f.stage==MoveDraftStage::Idle)return;
    if(cancel) { f=MoveDraftFlow{}; return; }
    if(!confirm)return;
    if(f.stage==MoveDraftStage::ConfirmOne)
        f.stage=MoveDraftStage::ConfirmTwo;
    else if(f.stage==MoveDraftStage::ConfirmTwo)
        (void)finishExperimentalMoveDraft(f,display,observed,catalog);
    else if(f.stage==MoveDraftStage::Result)f=MoveDraftFlow{};
}
} // namespace phase20
