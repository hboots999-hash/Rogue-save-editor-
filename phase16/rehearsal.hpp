// SPDX-License-Identifier: GPL-3.0-or-later
// Phase 16 SD-only full-container recovery rehearsal. NO INSTALLED-GAME WRITES.
// The Phase 14 journal format is used verbatim, but stored only in a separate
// phase16 rehearsal namespace. A rehearsal must NEVER be treated as permission
// to restore the installed game.
#pragma once
#include "journal_codec.hpp"
#include <cstring>
#include <string>

namespace phase16 {
enum class State {
    Missing, Present, Error
};
enum class Code {
    NotRun, StorageError, AlreadyPresent, PartialRecord,
    FirstReadError, SecondReadError, LiveChanged, LiveAuthenticationFailed,
    EditUnsupported, PlannedAuthenticationFailed,
    BackupWriteFailed, JournalWriteFailed, RecheckReadError,
    LiveChangedAfterJournal, Created,
    MissingRecord, BackupReadFailed, JournalReadFailed,
    JournalInvalid, BackupMismatch, CandidateInvalid, LiveNowDifferent,
    Verified
};
inline const char* label(Code x) {
    switch(x){
        case Code::NotRun:return "NOT RUN";
        case Code::StorageError:return "SD STORAGE ERROR";
        case Code::AlreadyPresent:return "JOURNAL ALREADY EXISTS";
        case Code::PartialRecord:return "PARTIAL RECORD: BLOCKED";
        case Code::FirstReadError:return "FIRST LIVE READ FAILED";
        case Code::SecondReadError:return "SECOND LIVE READ FAILED";
        case Code::LiveChanged:return "LIVE CHANGED: BLOCKED";
        case Code::LiveAuthenticationFailed:return "LIVE CMAC FAILED";
        case Code::EditUnsupported:return "CANDIDATE UNSUPPORTED";
        case Code::PlannedAuthenticationFailed:return "CANDIDATE CMAC FAILED";
        case Code::BackupWriteFailed:return "BACKUP WRITE FAILED";
        case Code::JournalWriteFailed:return "JOURNAL WRITE FAILED";
        case Code::RecheckReadError:return "LIVE RECHECK FAILED";
        case Code::LiveChangedAfterJournal:return "LIVE CHANGED AFTER JOURNAL";
        case Code::Created:return "SD JOURNAL VERIFIED";
        case Code::MissingRecord:return "NO PHASE16 RECORD";
        case Code::BackupReadFailed:return "SD BACKUP READ FAILED";
        case Code::JournalReadFailed:return "SD JOURNAL READ FAILED";
        case Code::JournalInvalid:return "SD JOURNAL CORRUPT";
        case Code::BackupMismatch:return "BACKUP/JOURNAL MISMATCH";
        case Code::CandidateInvalid:return "PLAN/MAC INVALID";
        case Code::LiveNowDifferent:return "LIVE SAVED SINCE RECORD";
        case Code::Verified:return "RESTART VERIFICATION PASS";
    }
    return "UNKNOWN";
}
struct Result {
    Code code=Code::NotRun;
    size_t originalSize=0,journalSize=0;
    uint32_t currentCounter=0;
    bool liveSafe=false;
};

// Store contract: existence errors are DISTINCT from absence; filenames are
// FIXED in the separate Phase16 namespace. All create calls must be exclusive,
// flushed, closed, reopened and compared byte-for-byte. Never overwrite.
template<class Device,class Store>
Result createRehearsal(Device& device,Store& store) {
    Result result{};
    const State backup=store.backupState();
    const State journal=store.journalState();
    if(backup==State::Error || journal==State::Error)
        {result.code=Code::StorageError;return result;}
    if(backup==State::Present && journal==State::Present)
        {result.code=Code::AlreadyPresent;return result;}
    if(backup==State::Present || journal==State::Present)
        {result.code=Code::PartialRecord;return result;}

    phase14::Bytes original,again;
    if(!device.read(original)){result.code=Code::FirstReadError;return result;}
    if(!device.read(again)){result.code=Code::SecondReadError;return result;}
    if(original!=again){result.code=Code::LiveChanged;return result;}
    result.originalSize=original.size();
    phase14::Layout layout{};
    if(!phase14::authenticate(original,device,layout))
        {result.code=Code::LiveAuthenticationFailed;return result;}
    result.currentCounter=layout.counter;

    // Prepare only a narrow, synthetic-intent money +1 plan in RAM.
    // This is a real current-save-derived candidate but is NEVER installed,
    // and should NOT be exported as a playable or release-ready edit.
    rogue_restore::Validation proof{};
    auto raw=rogue_restore::prepare(
        original.data()+layout.active+rogue_live::HeaderSize,
        rogue_restore::Operation::MoneyPlusOne,0,proof);
    if(!raw || !proof.ok){result.code=Code::EditUnsupported;return result;}
    phase14::Bytes candidate(raw.get(),raw.get()+h168::Size);
    phase14::Plan plan{};
    if(!phase14::prepare(original,candidate,
            rogue_restore::Operation::MoneyPlusOne,0,device,plan) ||
       !phase14::validatePlan(plan,device)){
        result.code=Code::PlannedAuthenticationFailed;return result;
    }
    phase14::Layout futureLayout{};
    if(!phase14::authenticate(plan.after,device,futureLayout) ||
       futureLayout.counter!=layout.counter+1 ||
       futureLayout.active!=layout.target ||
       !std::equal(original.begin()+layout.active,
          original.begin()+layout.active+rogue_live::SlotSize,
          plan.after.begin()+layout.active)){
        result.code=Code::PlannedAuthenticationFailed;return result;
    }
    const auto bytes=phase14::encodeJournal(plan);
    result.journalSize=bytes.size();
    if(bytes.size()!=64+2*original.size()){
        result.code=Code::PlannedAuthenticationFailed;return result;
    }

    // An untouched independently verified FULL-CONTAINER recovery image first.
    // A partial backup left behind blocks all future attempts rather than
    // being overwritten or silently deleted.
    if(!store.createBackup(original)){
        result.code=Code::BackupWriteFailed;return result;
    }
    if(!store.createJournal(bytes)){
        result.code=Code::JournalWriteFailed;return result;
    }

    phase14::Bytes finalRead;
    if(!device.read(finalRead)){
        result.code=Code::RecheckReadError;return result;
    }
    if(finalRead!=original){
        result.code=Code::LiveChangedAfterJournal;return result;
    }
    result.liveSafe=true;
    result.code=Code::Created;
    return result;
}

// Intentionally no device writes or recovery call here. Restart opens the two
// existing SD files independently and confirms the current live GBA container
// has NOT been modified by this application.
template<class Device,class Store>
Result verifyAfterRestart(Device& device,Store& store) {
    Result result{};
    const State backup=store.backupState(), journal=store.journalState();
    if(backup==State::Error || journal==State::Error)
        {result.code=Code::StorageError;return result;}
    if(backup==State::Missing && journal==State::Missing)
        {result.code=Code::MissingRecord;return result;}
    if(backup!=State::Present || journal!=State::Present)
        {result.code=Code::PartialRecord;return result;}
    phase14::Bytes original,record;
    if(!store.readBackup(original)){
        result.code=Code::BackupReadFailed;return result;
    }
    if(!store.readJournal(record)){
        result.code=Code::JournalReadFailed;return result;
    }
    result.originalSize=original.size();
    result.journalSize=record.size();
    phase14::Plan plan{};
    if(!phase14::decodeJournal(record,plan)){
        result.code=Code::JournalInvalid;return result;
    }
    if(plan.before!=original){
        result.code=Code::BackupMismatch;return result;
    }
    phase14::Layout layout{},futureLayout{};
    if(!phase14::validatePlan(plan,device) ||
       !phase14::authenticate(plan.before,device,layout) ||
       !phase14::authenticate(plan.after,device,futureLayout) ||
       futureLayout.active!=layout.target ||
       futureLayout.counter!=layout.counter+1){
        result.code=Code::CandidateInvalid;return result;
    }
    result.currentCounter=layout.counter;
    phase14::Bytes now,again;
    if(!device.read(now) || !device.read(again)){
        result.code=Code::RecheckReadError;return result;
    }
    if(now!=again){
        result.code=Code::LiveChanged;return result;
    }
    if(now!=original){
        result.code=Code::LiveNowDifferent;return result;
    }
    if(!phase14::authenticate(now,device,layout)){
        result.code=Code::LiveAuthenticationFailed;return result;
    }
    result.code=Code::Verified;result.liveSafe=true;
    return result;
}
} // namespace phase16
