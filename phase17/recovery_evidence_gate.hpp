// SPDX-License-Identifier: GPL-3.0-or-later
// Phase 17 read-only recovery evidence gate. NO filesystem or save writer.
// This deliberately returns a classification, NOT a writable transaction plan.
#pragma once
#include "journal_codec.hpp"

namespace phase17 {
enum class RecordState { NotChecked, Missing, Readable, Unreadable };

struct ReadOnlyRecord {
    RecordState state=RecordState::NotChecked;
    const phase14::Bytes* bytes=nullptr;
};
struct RecoveryEvidence {
    ReadOnlyRecord backup;
    ReadOnlyRecord journal;
    ReadOnlyRecord live;
    // Check BOTH terminal receipt paths, including IO failures. The Phase 14
    // journal is never silently reused once a terminal marker exists.
    RecordState restoredReceipt=RecordState::NotChecked;
    RecordState rolledbackReceipt=RecordState::NotChecked;
};
enum class EvidenceCode {
    Unchecked, StorageError, MissingBackup, MissingJournal,
    TerminalReceiptPresent, InvalidJournal, BackupMismatch,
    LiveChanged, InvalidAuthentication, ReadyForRamOnly
};
inline const char* evidenceLabel(EvidenceCode code) {
    switch(code) {
        case EvidenceCode::Unchecked: return "EVIDENCE NOT CHECKED";
        case EvidenceCode::StorageError: return "UNREADABLE RECORD: BLOCKED";
        case EvidenceCode::MissingBackup: return "BACKUP MISSING: BLOCKED";
        case EvidenceCode::MissingJournal: return "JOURNAL MISSING: BLOCKED";
        case EvidenceCode::TerminalReceiptPresent: return "TERMINAL RECORD EXISTS";
        case EvidenceCode::InvalidJournal: return "JOURNAL INVALID: BLOCKED";
        case EvidenceCode::BackupMismatch: return "BACKUP DOES NOT MATCH";
        case EvidenceCode::LiveChanged: return "LIVE SAVE HAS CHANGED";
        case EvidenceCode::InvalidAuthentication: return "AUTHENTICATION FAILED";
        case EvidenceCode::ReadyForRamOnly: return "RAM TEST ONLY: READABLE";
    }
    return "UNKNOWN";
}
struct EvidenceReport {
    EvidenceCode code=EvidenceCode::Unchecked;
    size_t backupSize=0;
    size_t journalSize=0;
    uint32_t sourceCounter=0;
    // This gate must never be interpreted as approval of live restoration.
    bool canRunRamRehearsal()const{return code==EvidenceCode::ReadyForRamOnly;}
    bool mayWriteInstalledGame()const{return false;}
    bool mayOverwriteJournal()const{return false;}
};

// View inputs are derived from independent, read-only storage probes. A
// missing receipt is different from a failed/unchecked receipt directory
// lookup. A caller must classify IO errors as Unreadable, not Missing.
// There is NO code path from this result to phase14::execute/recover.
template<class Signer>
EvidenceReport inspectRecoveryEvidence(const RecoveryEvidence& files,Signer& signer) {
    EvidenceReport result{};
    // Never interpret a corrupted/forged state value as an absent receipt.
    // In particular, a receipt read-probe failure cannot be equivalent to
    // RecordState::Missing. This gate does not read or write any file itself.
    const auto badState=[](RecordState state) {
        switch(state) {
            case RecordState::Missing: return false;
            case RecordState::Readable: return false;
            case RecordState::NotChecked: return true;
            case RecordState::Unreadable: return true;
        }
        return true; // Out-of-range enum, even from a corrupt caller.
    };
    if(badState(files.backup.state) || badState(files.journal.state) ||
       badState(files.live.state) || badState(files.restoredReceipt) ||
       badState(files.rolledbackReceipt)) {
        result.code=EvidenceCode::StorageError;return result;
    }
    if(files.restoredReceipt==RecordState::Readable ||
       files.rolledbackReceipt==RecordState::Readable) {
        result.code=EvidenceCode::TerminalReceiptPresent;return result;
    }
    if(files.backup.state==RecordState::Missing) {
        result.code=EvidenceCode::MissingBackup;return result;
    }
    if(files.journal.state==RecordState::Missing) {
        result.code=EvidenceCode::MissingJournal;return result;
    }
    if(files.live.state!=RecordState::Readable || !files.live.bytes ||
       !files.backup.bytes || !files.journal.bytes) {
        result.code=EvidenceCode::StorageError;return result;
    }
    const auto& backup=*files.backup.bytes;
    const auto& journal=*files.journal.bytes;
    const auto& live=*files.live.bytes;
    result.backupSize=backup.size();
    result.journalSize=journal.size();
    // Bound untrusted on-disk records before decoding or allocating the
    // two complete copies carried by the journal. A valid journal has an
    // exact 64-byte envelope around TWO equal-size container images.
    if(backup.size()<rogue_live::ContainerMin ||
       backup.size()>phase14::MaxContainer ||
       journal.size()<64u ||
       journal.size()!=64u+2u*backup.size()) {
        result.code=EvidenceCode::InvalidJournal;return result;
    }
    if(live.size()!=backup.size()) {
        result.code=EvidenceCode::LiveChanged;return result;
    }
    phase14::Plan candidate{};
    if(!phase14::decodeJournal(journal,candidate)) {
        result.code=EvidenceCode::InvalidJournal;return result;
    }
    if(candidate.before!=backup) {
        result.code=EvidenceCode::BackupMismatch;return result;
    }
    // Never accept a stale record, including a change outside the target
    // slot, and never assume an after-image is now the current live save.
    if(live!=backup) {
        result.code=EvidenceCode::LiveChanged;return result;
    }
    phase14::Layout before{},after{};
    if(!phase14::authenticate(live,signer,before) ||
       !phase14::validatePlan(candidate,signer) ||
       !phase14::authenticate(candidate.after,signer,after) ||
       after.active!=before.target ||
       after.counter!=before.counter+1) {
        result.code=EvidenceCode::InvalidAuthentication;return result;
    }
    result.sourceCounter=before.counter;
    result.code=EvidenceCode::ReadyForRamOnly;
    return result;
}
} // namespace phase17
