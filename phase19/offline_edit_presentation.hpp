// Phase19 user-facing offline-review control and presentation contract.
// UI only: NO filesystem, editor, title service, export or write capability.
// Phase20 must still independently authenticate and verify all edit requests.
#pragma once
#include "offline_review_ui.hpp"
#include "screen_text.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace phase19 {
enum class ApprovalStage : uint8_t { FirstReview, FinalReview, Approved, Cancelled };
enum class ApprovalEvent : uint8_t { None, Next, Approved, Cancelled };
struct OfflineApprovalGate {
    ApprovalStage stage=ApprovalStage::FirstReview;
    bool previousA=false, previousX=false;
    bool releasedBetween=false;
    void reset() { *this=OfflineApprovalGate{}; }
    // Call once per HID frame with HELD states; never pass repeat/key-release
    // events as approvals. Only a NEW A press reaches the second review.
    // The FINAL approval is a NEW X press after *all* controls are released.
    // A, X, SELECT or B held in combination cannot approve.
    ApprovalEvent poll(bool a,bool x,bool b,bool select) {
        const bool aEdge=a&&!previousA;
        const bool xEdge=x&&!previousX;
        previousA=a;previousX=x;
        if(stage==ApprovalStage::Approved || stage==ApprovalStage::Cancelled)
            return ApprovalEvent::None;
        if(b) {
            stage=ApprovalStage::Cancelled;
            return ApprovalEvent::Cancelled;
        }
        if(select || (a && x))return ApprovalEvent::None;
        if(stage==ApprovalStage::FirstReview && aEdge && !x) {
            stage=ApprovalStage::FinalReview;
            releasedBetween=false;
            return ApprovalEvent::Next;
        }
        if(stage==ApprovalStage::FinalReview) {
            if(!a && !x)releasedBetween=true;
            else if(releasedBetween && xEdge && !a) {
                stage=ApprovalStage::Approved;
                return ApprovalEvent::Approved;
            }
        }
        return ApprovalEvent::None;
    }
};
// A review is tied to the exact displayed source and intended field.
// On every re-import / reset, the caller must call cancel(). We ALSO
// revalidate at each frame to fail closed on unexpected source mutation.
struct OfflineReviewSession {
    ReviewTarget target{};
    OfflineApprovalGate gate{};
    bool active=false;
    bool begin(const Demo& d,const ReviewTarget& desired) {
        cancel();
        if(!reviewDisplayMatches(d,desired))return false;
        target=desired;
        active=true;
        return true;
    }
    void cancel() {
        gate.reset();
        target=ReviewTarget{};
        active=false;
    }
    ApprovalEvent poll(const Demo& d,bool a,bool x,bool b,bool select) {
        if(!active)return ApprovalEvent::None;
        if(!reviewDisplayMatches(d,target)) {
            cancel();
            return ApprovalEvent::Cancelled;
        }
        const auto out=gate.poll(a,x,b,select);
        if(out==ApprovalEvent::Cancelled)cancel();
        return out;
    }
};

// Export-result metadata: *display* only. Never implies a valid game save,
// successful disk sync/close, CMAC, restore or installed-title update.
struct OfflineResultNames {
    const char* source="00000001.sav"; // fixed application-owned import
    const char* backup=nullptr;
    const char* draft=nullptr;
    bool exporterReturnedSuccess=false;
};
inline bool validatedResultNames(const OfflineResultNames& r) {
    return r.exporterReturnedSuccess && r.source &&
           std::strcmp(r.source,"00000001.sav")==0 &&
           safeDraftBasename(r.backup) && safeDraftBasename(r.draft) &&
           std::strcmp(r.backup,r.draft)!=0;
}
template <typename Emit>
inline bool emitOfflineReview(const Demo& d,const ReviewTarget& target,
                              bool finalReview,Emit emit) {
    if(!reviewDisplayMatches(d,target))return false;
    char label[76]{}, line[76]{};
    if(!formatReviewTarget(d,target,label,sizeof(label)))return false;
    // Emit only short individually bounded rows; renderer can wrap each row.
    emit("OFFLINE COPY - NOT INSTALLED");
    emit("Source: imports/00000001.sav");
    emit("One edit per separate SD draft");
    emit(label);
    if(target.kind==ReviewField::IV || target.kind==ReviewField::EV ||
       target.kind==ReviewField::PP) {
        const auto& mon=d.party[target.slot];
        std::snprintf(line,sizeof(line),"Species ID: %u",mon.species);
        emit(line);
    }
    if(target.kind==ReviewField::PP){
        std::snprintf(line,sizeof(line),"Existing move ID: %u",target.moveId);
        emit(line);
    }
    if(target.kind==ReviewField::EV) {
        unsigned beforeTotal=0;
        for(unsigned v:d.party[target.slot].ev)beforeTotal+=v;
        // Subtraction safe: reviewDisplayMatches verified before==current stat.
        const unsigned afterTotal=beforeTotal-target.before+target.after;
        std::snprintf(line,sizeof(line),"EV total: %u -> %u / 510",beforeTotal,afterTotal);
        emit(line);
    }
    std::snprintf(line,sizeof(line),"Original: %u",target.before);emit(line);
    std::snprintf(line,sizeof(line),"New value: %u",target.after);emit(line);
    emit("Installed game: UNCHANGED");
    emit("Draft NOT game-tested.");
    if(finalReview) {
        emit("FINAL: release keys, press X");
        emit("B cancels, A cannot export");
    } else {
        emit("Review 1/2: A continues");
        emit("B cancels without output");
    }
    return true;
}
// Draws explicitly selected IV/EV/PP field with > marker in the field list.
// This is a visual list only; selecting a field cannot authorise an export.
template <typename Emit>
inline bool emitOfflineFields(const Demo& d,ReviewField kind,unsigned partySlot,
                              unsigned selectedField,Emit emit) {
    if(d.synthetic || d.partyStatus!=Availability::Ready ||
       partySlot>=d.partyCount || partySlot>=d.party.size() ||
       d.party[partySlot].egg)return false;
    if(kind!=ReviewField::IV && kind!=ReviewField::EV &&
       kind!=ReviewField::PP)return false;
    const unsigned n=kind==ReviewField::PP?4u:6u;
    if(selectedField>=n)return false;
    const auto& p=d.party[partySlot];
    const char* stats[]={"HP","Atk","Def","SpA","SpD","Spe"};
    char line[64]{};
    std::snprintf(line,sizeof(line),"Party #%u / species ID %u",partySlot+1u,p.species);
    emit(line);
    for(unsigned i=0;i<n;++i) {
        if(kind==ReviewField::PP) {
            std::snprintf(line,sizeof(line),"%c Move %u ID%u PP %u",
                          selectedField==i?'>':' ',i+1u,p.moves[i],p.pp[i]);
        }else{
            const unsigned value=kind==ReviewField::IV?p.iv[i]:p.ev[i];
            std::snprintf(line,sizeof(line),"%c %-3s %u",
                          selectedField==i?'>':' ',stats[i],value);
        }
        emit(line);
    }
    emit("Up/Down: select  A: edit");
    emit("B: back (no draft)");
    return true;
}
template <typename Emit>
inline bool emitOfflineResult(const OfflineResultNames& names,Emit emit) {
    if(!validatedResultNames(names)){
        emit("EXPORT UNVERIFIED / BLOCKED");
        emit("Inspect SD files for partials.");
        emit("Original import not approved.");
        emit("A/B: return (no restore)");
        return false;
    }
    emit("SEPARATE SD DRAFT CREATED");
    emit("Source: imports/00000001.sav");
    char line[88]{};
    std::snprintf(line,sizeof(line),"Backup: backups/%s",names.backup);emit(line);
    std::snprintf(line,sizeof(line),"Draft: drafts/%s",names.draft);emit(line);
    emit("Installed game: UNCHANGED");
    emit("One edit, NOT cumulative.");
    emit("Not game-tested. DO NOT RESTORE.");
    emit("A/B: return to imported view");
    return true;
}
} // namespace phase19
