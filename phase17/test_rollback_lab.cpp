// SPDX-License-Identifier: GPL-3.0-or-later
// Nonplayable H168 synthetic save. Host-only shadow writer/rollback tests.
// NEVER contacts the installed game, SD card or privileged Nintendo services.
#include "rollback_lab.hpp"
#include "recovery_evidence_gate.hpp"
#include "readonly_status_producer.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <algorithm>
using phase14::Bytes;
struct FakeSigner {
    int calls=0,failAt=0;
    bool hasReadHandle=false,unstable=false;
    bool calculate(const phase14::Digest& digest,phase14::Mac& mac) {
        if(!hasReadHandle)return false;
        ++calls;
        if(calls==failAt)return false;
        const auto d=phase14::hash(digest.data(),digest.size());
        std::copy_n(d.begin(),mac.size(),mac.begin());
        if(unstable)mac[0]^=static_cast<uint8_t>(calls);
        return true;
    }
};
static Bytes wrapped(const Bytes& raw,FakeSigner& signer,bool secondActive) {
    Bytes b(rogue_live::ContainerMin,0x5a);
    signer.hasReadHandle=true;
    for(size_t off:{size_t(0),size_t(rogue_live::SlotSize)}) {
        std::fill_n(b.begin()+off,rogue_live::HeaderSize,0);
        std::memcpy(b.data()+off,".SAV",4);
        const bool latest=(off!=0)==secondActive;
        h168::w32(b.data(),off+0x34,latest?49:48);
        h168::w32(b.data(),off+0x38,uint32_t(rogue_live::ExpectedTitle));
        h168::w32(b.data(),off+0x3c,uint32_t(rogue_live::ExpectedTitle>>32));
        std::fill_n(b.begin()+off+0x40,16,0x43);
        h168::w32(b.data(),off+0x50,rogue_live::HeaderSize);
        h168::w32(b.data(),off+0x54,rogue_live::RawSize);
        std::copy(raw.begin(),raw.end(),b.begin()+off+rogue_live::HeaderSize);
        phase14::Mac tag{};
        assert(phase14::stableMac(signer,phase14::slotHash(b,off),tag));
        std::copy(tag.begin(),tag.end(),b.begin()+off+0x10);
    }
    return b;
}
int main(int argc,char**argv) {
    assert(argc==2);
    static_assert(!phase14::LiveWritesEnabled,"Real installed-game writes must stay disabled");
    std::ifstream f(argv[1],std::ios::binary);
    const Bytes raw((std::istreambuf_iterator<char>(f)),{});
    assert(raw.size()==h168::Size);
    h168::Info inspect{};
    assert(h168::inspect(raw.data(),raw.size(),inspect));
    unsigned cases=0;
    for(const bool secondActive:{false,true}) {
        for(const auto operation:{rogue_restore::Operation::MoneyPlusOne,
                                  rogue_restore::Operation::ItemQuantityPlusOne}) {
            FakeSigner signer{};
            const Bytes source=wrapped(raw,signer,secondActive);
            phase14::Layout l{};
            assert(phase14::authenticate(source,signer,l));
            rogue_restore::Validation proof{};
            auto rawCandidate=rogue_restore::prepare(
                source.data()+l.active+rogue_live::HeaderSize,operation,0,proof);
            assert(rawCandidate&&proof.ok);
            const Bytes candidate(rawCandidate.get(),rawCandidate.get()+h168::Size);
            phase14::Plan plan{};
            assert(phase14::prepare(source,candidate,operation,0,signer,plan));
            assert(phase14::validatePlan(plan,signer));
            assert(plan.before==source && plan.after!=source);
            const Bytes record=phase14::encodeJournal(plan);
            assert(record.size()==526400);
            // Read-only multi-file recovery gate: every unknown/missing,
            // stale or corrupt state blocks before any transaction.
            using phase17::RecordState;
            using phase17::EvidenceCode;
            phase17::RecoveryEvidence evidence{};
            evidence.backup={RecordState::Readable,&source};
            evidence.journal={RecordState::Readable,&record};
            evidence.live={RecordState::Readable,&source};
            evidence.restoredReceipt=RecordState::Missing;
            evidence.rolledbackReceipt=RecordState::Missing;
            const auto okEvidence=phase17::inspectRecoveryEvidence(evidence,signer);
            assert(okEvidence.code==EvidenceCode::ReadyForRamOnly);
            assert(okEvidence.canRunRamRehearsal());
            assert(!okEvidence.mayWriteInstalledGame());
            assert(!okEvidence.mayOverwriteJournal());
            assert(okEvidence.backupSize==263168);
            assert(okEvidence.journalSize==526400);
            assert(okEvidence.sourceCounter==49);
            // Phase20 public status is a value-only observation. It never
            // exposes a Plan, write handle, privileged session or permission.
            P17ReadOnlyStatusV1 status{};
            assert(phase17::inspectToStatusV1(evidence,signer,status));
            assert(p17_readonly_status_shape_valid_v1(&status));
            assert(status.abi_version==1 && status.struct_size==80);
            assert(status.code==P17_RO_RAM_REHEARSAL_READABLE);
            assert(status.capability_bits==0);
            assert(status.expected_title==rogue_live::ExpectedTitle);
            assert(status.source_counter==49);
            assert(status.container_bytes==263168 &&
                   status.journal_bytes==526400);
            const auto originalDigest=phase14::hash(source.data(),source.size());
            assert(std::equal(originalDigest.begin(),originalDigest.end(),
                              status.source_sha256));
            status.capability_bits=1;
            assert(!p17_readonly_status_shape_valid_v1(&status));
            status.capability_bits=0;
            status.reserved1[0]=1;
            assert(!p17_readonly_status_shape_valid_v1(&status));
            status.reserved1[0]=0;
            auto invalidEvidence=evidence;
            invalidEvidence.journal.state=RecordState::Missing;
            assert(phase17::inspectToStatusV1(invalidEvidence,signer,status));
            assert(status.code==P17_RO_JOURNAL_MISSING);
            assert(status.container_bytes==0 && status.source_counter==0 &&
                   status.expected_title==0 && status.capability_bits==0);
            assert(std::all_of(std::begin(status.source_sha256),
                               std::end(status.source_sha256),
                               [](uint8_t x){return x==0;}));
            invalidEvidence=evidence;
            invalidEvidence.rolledbackReceipt=RecordState::Unreadable;
            assert(phase17::inspectToStatusV1(invalidEvidence,signer,status));
            assert(status.code==P17_RO_STORAGE_ERROR);
            // Malformed wrapper fields must fail BEFORE a signed plan can
            // be constructed (no guessing or partial-slot fallback).
            phase14::Layout rejectedLayout{};
            for(size_t off:{size_t(0),size_t(rogue_live::SlotSize)}) {
                for(size_t field:{size_t(0),size_t(0x38),size_t(0x50),
                                  size_t(0x54)}) {
                    auto malformed=source;
                    malformed[off+field]^=1;
                    assert(!phase14::layout(malformed,rejectedLayout));
                }
            }
            {
                auto malformed=source;
                h168::w32(malformed.data(),0x34,49);
                h168::w32(malformed.data(),rogue_live::SlotSize+0x34,49);
                assert(!phase14::layout(malformed,rejectedLayout));
                h168::w32(malformed.data(),0x34,48);
                h168::w32(malformed.data(),rogue_live::SlotSize+0x34,60);
                assert(!phase14::layout(malformed,rejectedLayout));
                h168::w32(malformed.data(),0x34,0);
                h168::w32(malformed.data(),rogue_live::SlotSize+0x34,UINT32_MAX);
                assert(!phase14::layout(malformed,rejectedLayout));
                malformed=source;
                malformed.pop_back(); // beyond both slots, but raw identity changed
                assert(!phase14::authenticate(malformed,signer,rejectedLayout));
            }
            // Sample damage across the entire 526400-byte journal, not
            // just its trailing digest. No corrupted sample may pass the
            // read-only evidence gate or reach the RAM rollback rehearsal.
            for(size_t sample=0;sample<32;sample++) {
                const size_t offset=(sample*record.size())/32;
                auto damaged=record;
                damaged[offset]^=static_cast<uint8_t>(1u<<(sample%8u));
                auto badEvidence=evidence;
                badEvidence.journal.bytes=&damaged;
                const auto check=phase17::inspectRecoveryEvidence(badEvidence,signer);
                assert(check.code==EvidenceCode::InvalidJournal);
                assert(!check.canRunRamRehearsal());
                assert(!check.mayWriteInstalledGame());
                assert(phase17::rehearse(source,damaged,signer).code==
                       phase17::ResultCode::BadJournal);
            }
            for(const size_t cut:{size_t(1),size_t(16),size_t(32),
                                  size_t(64),size_t(4096)}) {
                auto truncated=record;
                truncated.resize(record.size()-cut);
                auto badEvidence=evidence;
                badEvidence.journal.bytes=&truncated;
                assert(phase17::inspectRecoveryEvidence(badEvidence,signer).code==
                       EvidenceCode::InvalidJournal);
                assert(phase17::rehearse(source,truncated,signer).code==
                       phase17::ResultCode::BadJournal);
            }
            auto missing=evidence;
            missing.backup.state=RecordState::Missing;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::MissingBackup);
            missing=evidence;missing.journal.state=RecordState::Missing;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::MissingJournal);
            missing=evidence;missing.restoredReceipt=RecordState::Readable;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::TerminalReceiptPresent);
            missing=evidence;missing.rolledbackReceipt=RecordState::Readable;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::TerminalReceiptPresent);
            missing=evidence;missing.restoredReceipt=RecordState::NotChecked;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::StorageError);
            missing=evidence;missing.journal.state=RecordState::Unreadable;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::StorageError);
            auto stale=source;stale.back()^=1;
            missing=evidence;missing.live.bytes=&stale;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::LiveChanged);
            missing.live.bytes=&plan.after;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::LiveChanged);
            auto wrongBackup=source;wrongBackup[0x20]^=1;
            missing=evidence;missing.backup.bytes=&wrongBackup;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::BackupMismatch);
            auto corruptRecord=record;corruptRecord.back()^=1;
            missing=evidence;missing.journal.bytes=&corruptRecord;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::InvalidJournal);
            corruptRecord.pop_back();
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::InvalidJournal);
            auto incompatible=plan;
            incompatible.after[incompatible.slots.target+
                               rogue_live::HeaderSize+42]^=0x01;
            const auto incompatibleRecord=phase14::encodeJournal(incompatible);
            missing=evidence;missing.journal.bytes=&incompatibleRecord;
            assert(phase17::inspectRecoveryEvidence(missing,signer).code==
                   EvidenceCode::InvalidAuthentication);
            FakeSigner detachedSigner{};
            assert(phase17::inspectRecoveryEvidence(evidence,detachedSigner).code==
                   EvidenceCode::InvalidAuthentication);
            // If an unexpected header byte changes DURING a staged RAM
            // transaction, refusal is mandatory; never auto-undo unknown data.
            {
                phase17::Shadow tampered(source);
                tampered.tamperOnFirstWrite=true;
                tampered.tamperOffset=plan.slots.active+0x20;
                assert(phase14::execute(tampered,signer,plan)==
                       phase14::Status::RecoveryRequired);
                assert(tampered.journalPresent && !tampered.receiptWritten);
                const int writesBefore=tampered.writes;
                assert(phase14::recover(tampered,signer,plan)==
                       phase14::Status::RecoveryRequired);
                assert(tampered.writes==writesBefore);
            }
            // RAM IO refuses out-of-bounds requests, including overflow
            // arithmetic and a null data pointer, without touching state.
            {
                phase17::Shadow bounded(source);
                bounded.journalPresent=true;
                bounded.journal=record;
                const auto originalBytes=bounded.live;
                assert(!bounded.write(bounded.live.size(),nullptr,1));
                assert(!bounded.write(bounded.live.size()+1,originalBytes.data(),1));
                assert(!bounded.write(0,originalBytes.data(),bounded.live.size()+1));
                assert(bounded.writes==0 && bounded.live==originalBytes);
            }
            // A full shadow write with a FAILED completion receipt must be
            // reported unresolved, then recoverable on a new RAM backend.
            {
                phase17::Shadow first(source);
                first.failReceipt=true;
                assert(phase14::execute(first,signer,plan)==
                       phase14::Status::RecoveryRequired);
                assert(first.live==plan.after);
                assert(first.journalPresent && !first.receiptWritten);
                phase17::Shadow restarted(first.live);
                restarted.journalPresent=true;
                restarted.journal=first.journal;
                assert(phase14::recover(restarted,signer,plan)==
                       phase14::Status::RolledBack);
                assert(restarted.live==source && restarted.receiptWritten);
            }
            // Failed rollback receipt must not erase the immutable journal
            // or suppress the next explicit read/verify/recovery attempt.
            {
                phase17::Shadow first(plan.after);
                first.journalPresent=true;
                first.journal=record;
                first.failReceipt=true;
                assert(phase14::recover(first,signer,plan)==
                       phase14::Status::RecoveryRequired);
                assert(first.live==source && !first.receiptWritten);
                phase17::Shadow retry(first.live);
                retry.journalPresent=true;
                retry.journal=record;
                assert(phase14::recover(retry,signer,plan)==
                       phase14::Status::RolledBack);
                assert(retry.live==source && retry.receiptWritten);
            }
            const auto report=phase17::rehearse(source,record,signer);
            assert(report.code==phase17::ResultCode::Passed);
            assert(report.allByteExact);
            assert(report.normalRestores==1);
            assert(report.explicitRollbacks==1);
            assert(report.shortWriteRollbacks==4);
            assert(report.crashRestarts==4);
            assert(report.failedRollbackRetries==4);
            assert(report.interruptedRollbackRetries==4);
            assert(report.foreignChangeBlocks==1);
            // The RAM-only harness does not alter input sources or immutable records.
            assert(source==plan.before);
            assert(record==phase14::encodeJournal(plan));
            // Mutation of the journal digest fails BEFORE any shadow write.
            auto modified=record;modified.back()^=1;
            assert(phase17::rehearse(source,modified,signer).code==
                   phase17::ResultCode::BadJournal);
            modified=record;modified.pop_back();
            assert(phase17::rehearse(source,modified,signer).code==
                   phase17::ResultCode::BadJournal);
            modified=source;modified[0x30]^=1;
            assert(phase17::rehearse(modified,record,signer).code==
                   phase17::ResultCode::InvalidBefore);
            FakeSigner failed{};
            failed.hasReadHandle=true;failed.failAt=1;
            assert(phase17::rehearse(source,record,failed).code==
                   phase17::ResultCode::InvalidSignedPlan);
            FakeSigner noFile{};
            assert(phase17::rehearse(source,record,noFile).code==
                   phase17::ResultCode::InvalidSignedPlan);
            ++cases;
        }
    }
    std::cout<<"PASS Phase17 "<<cases<<" plan types, RAM-only restore, "
             <<"explicit undo, 4 short writes, 4 crash/restart rollbacks, 8 interrupted recovery retries, "
             <<"foreign-byte refusal, MAC/journal corruption guards. "
             <<"NO LIVE WRITE ON 3DS.\n";
}
