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
    if(!p17_readonly_status_init_blocked_v1(&out))return false;
    const auto report=inspectRecoveryEvidence(files,signer);
    // Explicit enum map, not an implicit binary cast.
    switch(report.code) {
        case EvidenceCode::Unchecked: out.code=P17_RO_NOT_CHECKED;break;
        case EvidenceCode::StorageError:out.code=P17_RO_STORAGE_ERROR;break;
        case EvidenceCode::MissingBackup:out.code=P17_RO_BACKUP_MISSING;break;
        case EvidenceCode::MissingJournal:out.code=P17_RO_JOURNAL_MISSING;break;
        case EvidenceCode::TerminalReceiptPresent:
            out.code=P17_RO_TERMINAL_RECEIPT_PRESENT;break;
        case EvidenceCode::InvalidJournal:out.code=P17_RO_JOURNAL_INVALID;break;
        case EvidenceCode::BackupMismatch:out.code=P17_RO_BACKUP_MISMATCH;break;
        case EvidenceCode::LiveChanged:out.code=P17_RO_LIVE_CHANGED;break;
        case EvidenceCode::InvalidAuthentication:
            out.code=P17_RO_AUTHENTICATION_FAILED;break;
        case EvidenceCode::ReadyForRamOnly: {
            if(!files.backup.bytes || report.backupSize>UINT32_MAX ||
               report.journalSize>UINT32_MAX)return false;
            out.code=P17_RO_RAM_REHEARSAL_READABLE;
            out.expected_title=rogue_live::ExpectedTitle;
            out.source_counter=report.sourceCounter;
            out.container_bytes=uint32_t(report.backupSize);
            out.journal_bytes=uint32_t(report.journalSize);
            const auto& b=*files.backup.bytes;
            const auto digest=phase14::hash(b.data(),b.size());
            std::memcpy(out.source_sha256,digest.data(),digest.size());
            break;
        }
    }
    // This status object is never an execute() or recover() capability.
    return p17_readonly_status_shape_valid_v1(&out)!=0;
}
} // namespace phase17
