// Phase17 recovery evidence must never authorize installed-game writes.
// Uses missing/damaged synthetic records only; no console signer, ROM or save.
#include "recovery_evidence_gate.hpp"
#include <cassert>
#include <iostream>
struct DenyingSigner {
    bool calculate(const phase14::Digest&,phase14::Mac&) { return false; }
};
int main() {
    static_assert(!phase14::LiveWritesEnabled,"live writes must be off");
    DenyingSigner signer{};
    phase17::RecoveryEvidence e{};
    auto report=phase17::inspectRecoveryEvidence(e,signer);
    assert(report.code==phase17::EvidenceCode::StorageError);
    assert(!report.canRunRamRehearsal() && !report.mayWriteInstalledGame());
    assert(!report.mayOverwriteJournal());

    e.backup.state=phase17::RecordState::Missing;
    e.journal.state=phase17::RecordState::Missing;
    e.live.state=phase17::RecordState::Missing;
    e.restoredReceipt=phase17::RecordState::Missing;
    e.rolledbackReceipt=phase17::RecordState::Missing;
    report=phase17::inspectRecoveryEvidence(e,signer);
    assert(report.code==phase17::EvidenceCode::MissingBackup);
    assert(!report.mayWriteInstalledGame());

    phase14::Bytes backup(128,0), badJournal{0xDE,0xAD}, live=backup;
    e.backup={phase17::RecordState::Readable,&backup};
    e.journal={phase17::RecordState::Readable,&badJournal};
    e.live={phase17::RecordState::Readable,&live};
    report=phase17::inspectRecoveryEvidence(e,signer);
    assert(report.code==phase17::EvidenceCode::InvalidJournal);
    assert(!report.canRunRamRehearsal() && !report.mayWriteInstalledGame());

    e.restoredReceipt=phase17::RecordState::Readable;
    report=phase17::inspectRecoveryEvidence(e,signer);
    assert(report.code==phase17::EvidenceCode::TerminalReceiptPresent);
    assert(!report.mayOverwriteJournal());

    e.restoredReceipt=phase17::RecordState::Missing;
    e.rolledbackReceipt=phase17::RecordState::Unreadable;
    report=phase17::inspectRecoveryEvidence(e,signer);
    assert(report.code==phase17::EvidenceCode::StorageError);
    assert(!report.mayWriteInstalledGame());
    std::cout<<"PASS: Phase17 corrupted/unchecked/terminal evidence blocks, writes always prohibited\n";
}
