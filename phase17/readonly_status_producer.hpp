// SPDX-License-Identifier: GPL-3.0-or-later
// PRIVATE Phase17-only adapter. NEVER include this with Phase18 offline.hpp.
// Consumers receive ONLY readonly_status_v1.h via a compiled C ABI boundary.
#pragma once
#include "recovery_evidence_gate.hpp"
#include "readonly_status_v1.h"
#include <cstring>
#include <limits>

namespace phase17 {
template<class Signer>
bool inspectToStatusV1(const RecoveryEvidence& files,Signer& signer,
                       P17ReadOnlyStatusV1& out) {
    // Publish only a fully validated envelope. On any failure keep an
    // unmistakably blocked status in the caller's buffer; never expose a
    // partial RAM-ready claim if size/ABI validation fails late.
    if(!p17_readonly_status_init_blocked_v1(&out))return false;
    P17ReadOnlyStatusV1 staged=out;
    const auto report=inspectRecoveryEvidence(files,signer);
    // Explicit enum map, not an implicit binary cast.
    switch(report.code) {
        case EvidenceCode::Unchecked: staged.code=P17_RO_NOT_CHECKED;break;
        case EvidenceCode::StorageError:staged.code=P17_RO_STORAGE_ERROR;break;
        case EvidenceCode::MissingBackup:staged.code=P17_RO_BACKUP_MISSING;break;
        case EvidenceCode::MissingJournal:staged.code=P17_RO_JOURNAL_MISSING;break;
        case EvidenceCode::TerminalReceiptPresent:
            staged.code=P17_RO_TERMINAL_RECEIPT_PRESENT;break;
        case EvidenceCode::InvalidJournal:staged.code=P17_RO_JOURNAL_INVALID;break;
        case EvidenceCode::BackupMismatch:staged.code=P17_RO_BACKUP_MISMATCH;break;
        case EvidenceCode::LiveChanged:staged.code=P17_RO_LIVE_CHANGED;break;
        case EvidenceCode::InvalidAuthentication:
            staged.code=P17_RO_AUTHENTICATION_FAILED;break;
        case EvidenceCode::ReadyForRamOnly: {
            if(!files.backup.bytes || report.backupSize>UINT32_MAX ||
               report.journalSize>UINT32_MAX)return false;
            staged.code=P17_RO_RAM_REHEARSAL_READABLE;
            staged.expected_title=rogue_live::ExpectedTitle;
            staged.source_counter=report.sourceCounter;
            staged.container_bytes=uint32_t(report.backupSize);
            staged.journal_bytes=uint32_t(report.journalSize);
            const auto& b=*files.backup.bytes;
            const auto digest=phase14::hash(b.data(),b.size());
            std::memcpy(staged.source_sha256,digest.data(),digest.size());
            break;
        }
    }
    // This status object is never an execute() or recover() capability.
    if(!p17_readonly_status_shape_valid_v1(&staged))return false;
    out=staged;
    return true;
}
} // namespace phase17
