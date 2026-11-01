// Phase17 recovery evidence must never authorize installed-game writes.
// Uses missing/damaged synthetic records only; no console signer, ROM or save.
#include "recovery_evidence_gate.hpp"
#include <cassert>
#include <iostream>
#include <cstring>
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
    // This checks only structural wrapper selection, not CMAC or restoration.
    // A nonadjacent gap currently reaches Phase14 layout() unchecked.
    phase14::Bytes wrapped(rogue_live::ContainerMin,0);
    for(size_t off : {size_t(0),size_t(rogue_live::SlotSize)}) {
        std::memcpy(wrapped.data()+off,".SAV",4);
        h168::w32(wrapped.data(),off+0x34,off?49u:48u);
        h168::w32(wrapped.data(),off+0x38,
                   static_cast<uint32_t>(rogue_live::ExpectedTitle));
        h168::w32(wrapped.data(),off+0x3c,
                   static_cast<uint32_t>(rogue_live::ExpectedTitle>>32));
        std::memset(wrapped.data()+off+0x40,0x43,16);
        h168::w32(wrapped.data(),off+0x50,rogue_live::HeaderSize);
        h168::w32(wrapped.data(),off+0x54,rogue_live::RawSize);
    }
    phase14::Layout structurallySelected{};
    assert(phase14::layout(wrapped,structurallySelected));
    h168::w32(wrapped.data(),rogue_live::SlotSize+0x34,60u);
    if (phase14::layout(wrapped,structurallySelected))
        std::cout<<"::warning::QA-02 Phase14 structural wrapper selector accepted nonadjacent 48/60 counters; require independent release review\\n";
    std::cout<<"PASS: Phase17 corrupted/unchecked/terminal evidence blocks, writes always prohibited\n";
}
