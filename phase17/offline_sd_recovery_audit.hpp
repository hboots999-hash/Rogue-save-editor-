// SPDX-License-Identifier: GPL-3.0-or-later
// Pure host/RAM-only review of independently copied Phase20 SD export data.
// NO filesystem, save service, write path, CMAC writer or restore authority.
// This cannot replace Phase18's save-field and byte-footprint validation.
#pragma once
#include "authenticated_restore.hpp"

namespace phase17 {
enum class SdReadState { Unchecked, Missing, Complete, IOError };
struct RawFileObservation {
    SdReadState state=SdReadState::Unchecked;
    const phase14::Bytes* bytes=nullptr; // Test-owned observed full file
};
enum class RawExportReview {
    Unchecked, SDReadFailure, ImportMissing, BackupMissing, DraftMissing,
    PartialOrWrongSize, StaleImport, BackupMismatch, DraftMismatch,
    ReusedOrUnprovenNames, NoChange, OfflineCopyReviewedOnly
};
struct OfflineSdReviewEvidence {
    RawFileObservation import;
    RawFileObservation backup;
    RawFileObservation draft;
    // These flags represent independently verified no-overwrite pathname
    // observations, NOT statements made by the app under test.
    bool distinctNamesProven=false;
    bool exclusiveCreationProven=false;
};
struct OfflineSdReviewReport {
    RawExportReview result=RawExportReview::Unchecked;
    bool rawCopyMatchesExpected=false;
    bool canModifyInstalledGame()const{return false;}
    bool canRestoreAuthenticatedContainer()const{return false;}
    bool authorisesGameplayInjection()const{return false;}
};

// The caller must independently generate expectedDraft via Phase18's
// deterministic verifier and expectedOriginal from a protected pre-test
// snapshot. They are synthetic in the Phase17 test. A positive review says
// ONLY "raw copied SD bytes match this expected test"; neither a proof of
// native SD durability nor a game-save compatibility certificate.
inline OfflineSdReviewReport reviewOfflineSdOnly(
    const phase14::Bytes& expectedOriginal,
    const phase14::Bytes& expectedDraft,
    const OfflineSdReviewEvidence& evidence) {
    OfflineSdReviewReport report{};
    const auto invalid=[](SdReadState s){
        switch(s){
            case SdReadState::Complete:return false;
            case SdReadState::Missing:return false;
            case SdReadState::Unchecked:return true;
            case SdReadState::IOError:return true;
        }
        return true; // Reject malformed enum value.
    };
    if(invalid(evidence.import.state) || invalid(evidence.backup.state) ||
       invalid(evidence.draft.state)) {
        report.result=RawExportReview::SDReadFailure;return report;
    }
    if(evidence.import.state==SdReadState::Missing) {
        report.result=RawExportReview::ImportMissing;return report;
    }
    if(evidence.backup.state==SdReadState::Missing) {
        report.result=RawExportReview::BackupMissing;return report;
    }
    if(evidence.draft.state==SdReadState::Missing) {
        report.result=RawExportReview::DraftMissing;return report;
    }
    if(!evidence.import.bytes || !evidence.backup.bytes ||
       !evidence.draft.bytes ||
       expectedOriginal.size()!=131072u || expectedDraft.size()!=131072u ||
       evidence.import.bytes->size()!=131072u ||
       evidence.backup.bytes->size()!=131072u ||
       evidence.draft.bytes->size()!=131072u) {
        report.result=RawExportReview::PartialOrWrongSize;return report;
    }
    if(*evidence.import.bytes!=expectedOriginal) {
        report.result=RawExportReview::StaleImport;return report;
    }
    if(*evidence.backup.bytes!=expectedOriginal) {
        report.result=RawExportReview::BackupMismatch;return report;
    }
    if(*evidence.draft.bytes!=expectedDraft) {
        report.result=RawExportReview::DraftMismatch;return report;
    }
    if(!evidence.distinctNamesProven || !evidence.exclusiveCreationProven) {
        report.result=RawExportReview::ReusedOrUnprovenNames;return report;
    }
    if(expectedDraft==expectedOriginal) {
        report.result=RawExportReview::NoChange;return report;
    }
    // Publish positive data-matching flag ONLY with the final reviewed
    // status. Partial evidence must not look like a completed SD export.
    report.rawCopyMatchesExpected=true;
    report.result=RawExportReview::OfflineCopyReviewedOnly;
    return report;
}
} // namespace phase17
