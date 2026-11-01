// SPDX-License-Identifier: GPL-3.0-or-later
// Phase 17: RAM-ONLY dry run of the Phase 14 restore/rollback state machine.
// The ONLY mutable target is a std::vector owned by this class.
// This never obtains a write handle to the installed GBA VC save or SD card.
#pragma once
#include "journal_codec.hpp"
#include <cstring>
#include <string>

namespace phase17 {
using phase14::Bytes;
using phase14::Plan;
using phase14::Status;

struct SimulatedPowerLoss {};
enum class ResultCode {
    NotRun, InvalidBefore, BadJournal, InvalidSignedPlan,
    NormalRestoreFailed, NormalRecoveryFailed, PartialRollbackFailed,
    CrashRecoveryFailed, UnrelatedMutationAllowed, Passed
};
inline const char* label(ResultCode code) {
    switch(code){
        case ResultCode::NotRun:return "NOT RUN";
        case ResultCode::InvalidBefore:return "SOURCE MISMATCH";
        case ResultCode::BadJournal:return "JOURNAL INTEGRITY BAD";
        case ResultCode::InvalidSignedPlan:return "SIGNED PLAN INVALID";
        case ResultCode::NormalRestoreFailed:return "SHADOW RESTORE FAILED";
        case ResultCode::NormalRecoveryFailed:return "SHADOW UNDO FAILED";
        case ResultCode::PartialRollbackFailed:return "PARTIAL ROLLBACK FAILED";
        case ResultCode::CrashRecoveryFailed:return "CRASH RECOVERY FAILED";
        case ResultCode::UnrelatedMutationAllowed:return "FOREIGN CHANGE ALLOWED";
        case ResultCode::Passed:return "RAM ROLLBACK TESTS PASSED";
    }
    return "UNKNOWN";
}
struct Report {
    ResultCode code=ResultCode::NotRun;
    unsigned normalRestores=0;
    unsigned explicitRollbacks=0;
    unsigned shortWriteRollbacks=0;
    unsigned crashRestarts=0;
    unsigned failedRollbackRetries=0;
    unsigned interruptedRollbackRetries=0;
    unsigned foreignChangeBlocks=0;
    bool allByteExact=false;
};

// The immutable Phase 14-format journal is supplied IN MEMORY. persist()
// copies it into a private RAM field for checking; it NEVER reaches native
// FSUser or privileged GBA save services. Exposed class does not inherit
// NativePxi and has no way to acquire a system file handle.
class Shadow final {
public:
    Bytes live;
    Bytes journal;
    int writes=0, shortWriteAt=0, powerLossAt=0;
    bool journalPresent=false;
    bool receiptWritten=false;
    bool failReceipt=false; // Simulate lost/failed terminal receipt in RAM only.
    bool tamperOnFirstWrite=false;
    size_t tamperOffset=0x20; // synthetic-only byte, caller chooses untouched source slot
    explicit Shadow(Bytes initial):live(std::move(initial)) {}
    bool writesAllowed()const{return true;} // ONLY this private vector
    bool unresolved()const{return journalPresent;}
    bool read(Bytes& copy) {copy=live;return true;}
    bool persist(const Plan& p) {
        if(journalPresent)return false;
        journal=phase14::encodeJournal(p);
        if(journal.empty())return false;
        journalPresent=true;
        return verifyJournal(p);
    }
    bool verifyJournal(const Plan& p)const {
        if(!journalPresent)return false;
        Plan recovered{};
        return phase14::decodeJournal(journal,recovered) &&
            recovered.before==p.before && recovered.after==p.after &&
            recovered.slots.active==p.slots.active &&
            recovered.slots.target==p.slots.target &&
            recovered.slots.counter==p.slots.counter &&
            recovered.op==p.op && recovered.item==p.item;
    }
    bool write(size_t off,const uint8_t* bytes,size_t n) {
        if(!journalPresent || !bytes || off>live.size() ||
           n>live.size()-off)return false;
        ++writes;
        const bool partial=writes==shortWriteAt;
        const size_t count=partial?n/2:n;
        if(count)std::memcpy(live.data()+off,bytes,count);
        if(tamperOnFirstWrite && writes==1) {
            // Change an unrelated byte in the untouched source slot.
            // Never index beyond the simulated buffer, including fuzz tests.
            if(tamperOffset>=live.size())return false;
            live[tamperOffset]^=0x40;
        }
        if(writes==powerLossAt)throw SimulatedPowerLoss{};
        return !partial;
    }
    bool finish(Status status) {
        if(!journalPresent || failReceipt)return false;
        receiptWritten=status==Status::Restored || status==Status::RolledBack;
        return receiptWritten;
    }
};

inline bool remainsOriginal(const Shadow& shadow,const Bytes& before) {
    return shadow.live==before;
}
// For every call to the native signer, the caller MUST have a previously
// authenticated, still-open READ-ONLY file-bound Pxi service session.
// The operation never calls a native write, even when testing a "restore".
template<class Signer>
Report rehearse(const Bytes& original,const Bytes& encoded,Signer& signer) {
    Report result{};
    Plan plan{};
    if(!phase14::decodeJournal(encoded,plan)){
        result.code=ResultCode::BadJournal;return result;
    }
    if(plan.before!=original){
        result.code=ResultCode::InvalidBefore;return result;
    }
    if(!phase14::validatePlan(plan,signer)){
        result.code=ResultCode::InvalidSignedPlan;return result;
    }
    {
        Shadow copy(original);
        if(phase14::execute(copy,signer,plan)!=Status::Restored ||
           copy.live!=plan.after || !copy.receiptWritten){
            result.code=ResultCode::NormalRestoreFailed;return result;
        }
        result.normalRestores++;
        // A second deliberate explicit request to undo the successful
        // memory-only change. This tests independent recovery entrypoints.
        if(phase14::recover(copy,signer,plan)!=Status::RolledBack ||
           !remainsOriginal(copy,original)){
            result.code=ResultCode::NormalRecoveryFailed;return result;
        }
        result.explicitRollbacks++;
    }
    for(int failAt=1;failAt<=4;failAt++){
        Shadow copy(original);
        copy.shortWriteAt=failAt;
        if(phase14::execute(copy,signer,plan)!=Status::RolledBack ||
           !remainsOriginal(copy,original)){
            result.code=ResultCode::PartialRollbackFailed;return result;
        }
        result.shortWriteRollbacks++;
    }
    // Simulate abrupt process termination after each of the four native
    // write substeps; reconstruct an entirely NEW memory IO object using only
    // the immutable journal and the partial image left by the interruption.
    for(int crashAt=1;crashAt<=4;crashAt++){
        Shadow first(original);
        first.powerLossAt=crashAt;
        bool interrupted=false;
        try{(void)phase14::execute(first,signer,plan);}
        catch(const SimulatedPowerLoss&){interrupted=true;}
        if(!interrupted || !first.journalPresent){
            result.code=ResultCode::CrashRecoveryFailed;return result;
        }
        Shadow restarted(first.live);
        restarted.journalPresent=true;
        restarted.journal=first.journal;
        if(phase14::recover(restarted,signer,plan)!=Status::RolledBack ||
           !remainsOriginal(restarted,original)){
            result.code=ResultCode::CrashRecoveryFailed;return result;
        }
        result.crashRestarts++;
    }
    // Retry recovery after a short write DURING rollback itself. The first
    // recovery must fail closed and keep the immutable journal available.
    for(int failAt=1;failAt<=4;failAt++){
        Shadow first(plan.after);
        first.journalPresent=true;
        first.journal=encoded;
        first.shortWriteAt=failAt;
        if(phase14::recover(first,signer,plan)!=Status::RecoveryRequired ||
           !first.journalPresent){
            result.code=ResultCode::PartialRollbackFailed;return result;
        }
        Shadow retry(first.live);
        retry.journalPresent=true;
        retry.journal=encoded;
        if(phase14::recover(retry,signer,plan)!=Status::RolledBack ||
           !remainsOriginal(retry,original)){
            result.code=ResultCode::PartialRollbackFailed;return result;
        }
        result.failedRollbackRetries++;
    }
    // And a second abrupt process termination DURING the recovery path.
    // Restart a NEW in-memory backend from the interrupted rollback image.
    for(int crashAt=1;crashAt<=4;crashAt++){
        Shadow first(plan.after);
        first.journalPresent=true;
        first.journal=encoded;
        first.powerLossAt=crashAt;
        bool interrupted=false;
        try{(void)phase14::recover(first,signer,plan);}
        catch(const SimulatedPowerLoss&){interrupted=true;}
        if(!interrupted){
            result.code=ResultCode::CrashRecoveryFailed;return result;
        }
        Shadow retry(first.live);
        retry.journalPresent=true;
        retry.journal=encoded;
        if(phase14::recover(retry,signer,plan)!=Status::RolledBack ||
           !remainsOriginal(retry,original)){
            result.code=ResultCode::CrashRecoveryFailed;return result;
        }
        result.interruptedRollbackRetries++;
    }
    {
        Shadow bad(original);
        bad.powerLossAt=1;
        try{(void)phase14::execute(bad,signer,plan);}
        catch(const SimulatedPowerLoss&){}
        bad.live.back()^=0x80; // outside target slot, unrelated mutation
        Shadow restarted(bad.live);
        restarted.journalPresent=true;
        restarted.journal=bad.journal;
        if(phase14::recover(restarted,signer,plan)!=Status::RecoveryRequired ||
           restarted.writes!=0){
            result.code=ResultCode::UnrelatedMutationAllowed;return result;
        }
        result.foreignChangeBlocks++;
    }
    result.allByteExact=true;
    result.code=ResultCode::Passed;
    return result;
}
} // namespace phase17
