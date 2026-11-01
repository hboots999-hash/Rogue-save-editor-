// SPDX-License-Identifier: GPL-3.0-or-later
// Phase20 opt-in OFFLINE COPIED-SAVE editor. No installed-title archives,
// no CMAC/restore, no arbitrary SD paths, no support for ROM-unknown edits.
// The existing Phase19 UI remains an immutable read-only library.
#pragma once
#include "phase20/read_only_v1.hpp"
#include "phase18/offline_export.hpp"
#include "phase18/multi_export_v3.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <new>
#include <exception>
#include <string>

namespace phase20 {
enum class OfflineKind : uint8_t { Money, ExistingItem, IV, EV, PP };
struct EditChoice {
    OfflineKind kind=OfflineKind::Money;
    unsigned slot=0;        // existing party or occupied bag index
    unsigned field=0;       // stat index 0..5 or move slot 0..3
    unsigned value=0;
};
inline bool sameVisibleSource(const phase19::Demo& display,
                              const CopyObservation& captured,
                              const rogue18::v2::ReadSnapshot& current) {
    const auto& v=current.data;
    return captured.structurallyReadable &&
           !captured.canModifyInstalledGame && !captured.canRestore &&
           !display.synthetic && display.sourceModelApi==rogue18::ModelApiVersion &&
           current.version==rogue18::v2::ContractVersion &&
           v.valid && v.apiVersion==rogue18::ModelApiVersion &&
           v.confidence==rogue18::DataConfidence::StructurallyValidated &&
           v.capabilities.copiedSaveOnly && !v.capabilities.liveWrite &&
           !v.capabilities.restore && !v.capabilities.pcEdit &&
           captured.sourceSha256==display.sourceSha256 &&
           captured.sourceSha256==v.sourceSha256 &&
           captured.activeCounter==display.activeCounter &&
           captured.activeCounter==v.activeCounter &&
           captured.money==display.money && captured.money==v.money &&
           captured.party==display.partyCount &&
           captured.items==display.bagCount &&
           captured.party==v.party.size() && captured.items==v.items.size();
}
inline bool requestForCopy(const uint8_t* raw,size_t n,
                           const phase19::Demo& display,
                           const CopyObservation& captured,
                           const EditChoice& choice,
                           rogue18::Request& output,std::string& reason) {
    output=rogue18::Request{};
    if(!raw || n!=CopiedRawBytes || !rogue18::hasDigest(captured.sourceSha256)) {
        reason="No exact copied-save input/fingerprint; export blocked.";return false;
    }
    const auto view=rogue18::v2::inspect(raw,n);
    if(!sameVisibleSource(display,captured,view)) {
        reason="Displayed snapshot differs from imported SD copy; re-import first.";
        return false;
    }
    bool ok=false;
    switch(choice.kind) {
        case OfflineKind::Money:
            if(choice.slot!=0 || choice.field!=0 || choice.value>999999u ||
               choice.value==display.money)break;
            ok=rogue18::v2::begin(view,rogue18::Kind::Money,0,output);
            if(ok)output.money=choice.value;
            break;
        case OfflineKind::ExistingItem:
            if(choice.field!=0 || choice.slot>=display.bagCount ||
               choice.value<1u || choice.value>999u ||
               choice.value==display.bag[choice.slot].quantity)break;
            if(choice.slot>=view.data.items.size() ||
               view.data.items[choice.slot].itemId!=display.bag[choice.slot].id ||
               view.data.items[choice.slot].quantity!=display.bag[choice.slot].quantity)break;
            ok=rogue18::v2::begin(view,rogue18::Kind::ExistingQuantity,
                                  choice.slot,output);
            if(ok)output.quantity=static_cast<uint16_t>(choice.value);
            break;
        case OfflineKind::IV:
        case OfflineKind::EV:
        case OfflineKind::PP: {
            if(choice.slot>=display.partyCount || choice.slot>=view.data.party.size())break;
            const auto& m=display.party[choice.slot];
            const auto& original=view.data.party[choice.slot].value;
            if(original.egg || m.egg || original.species!=m.species ||
               original.abilitySlot!=m.ability || original.effectiveNature!=m.nature)break;
            for(unsigned i=0;i<6;i++)
                if(original.ivs[i]!=m.iv[i] || original.evs[i]!=m.ev[i])return false;
            for(unsigned i=0;i<4;i++)
                if(original.moves[i]!=m.moves[i] || original.pp[i]!=m.pp[i])return false;
            if(choice.kind==OfflineKind::PP) {
                if(choice.field>=4 || !m.moves[choice.field] || choice.value>64u ||
                   choice.value==m.pp[choice.field])break;
                ok=rogue18::v2::beginMove(view,choice.slot,choice.field,output);
                if(ok)output.pp=static_cast<uint8_t>(choice.value);
            } else {
                if(choice.field>=6)break;
                const bool iv=choice.kind==OfflineKind::IV;
                if(choice.value>(iv?31u:252u) ||
                   choice.value==(iv?m.iv[choice.field]:m.ev[choice.field]))break;
                if(!iv) {
                    unsigned other=0;
                    for(unsigned i=0;i<6;i++)
                        if(i!=choice.field)other+=m.ev[i];
                    if(other>510u || choice.value>510u-other)break;
                }
                ok=rogue18::v2::begin(view,iv?rogue18::Kind::IVs:
                                     rogue18::Kind::EVs,choice.slot,output);
                if(ok)output.stats[choice.field]=static_cast<uint8_t>(choice.value);
            }
            break;
        }
    }
    if(!ok || !rogue18::v2::availableForOfflinePreview(output) ||
       !rogue18::hasDigest(output.expectedDigest) ||
       rogue18::v2::mayRestoreInstalledGame(output)) {
        reason="Value/field/identity unsupported or unchanged; no draft created.";
        return false;
    }
    const auto preview=rogue18::v2::preview(raw,n,output);
    if(!preview.ok || preview.candidate.size()!=n ||
       !rogue18::v2::verify(raw,n,preview.candidate.data(),
                           preview.candidate.size(),output)) {
        reason=preview.ok?"Candidate exact-byte verification failed.":
                          "Candidate rejected: "+preview.reason;
        return false;
    }
    reason="Validated separate offline RAM candidate; installed game untouched.";
    return true;
}

// Host tests may pass a temporary fixed-import folder; native 3DS transport
// independently restricts source/root to the one application-owned SD folder.
// Caller must obtain an explicit two-confirmation before invoking this.
inline rogue18::ExportResult exportGuarded(const uint8_t* raw,size_t n,
    const phase19::Demo& display,const CopyObservation& captured,
    const EditChoice& choice,const char* sourcePath,const char* root) {
    // Full Phase18 model/preview/export uses heap vectors and std::string.
    // The 3DS must not crash out of the UI if those allocations fail.
    // Any partially created independent SD output is retained, never deleted
    // or falsely treated as an approved export.
    try {
        rogue18::Request request{};
        std::string reason;
        if(!requestForCopy(raw,n,display,captured,choice,request,reason))
            return {false,reason};
        return rogue18::exportCopied(raw,n,sourcePath,root,request);
    } catch (const std::bad_alloc&) {
        return {false,"Out of 3DS memory; original import untouched. Check SD draft directory."};
    } catch (const std::exception&) {
        return {false,"Offline export exception; review separate SD files; original import untouched."};
    }
}

inline rogue18::ExportResult exportFixedSDCopy(const phase19::Demo& display,
    const CopyObservation& captured,const EditChoice& choice) {
    // Fixed path READ and immutable distinct SD draft/backup WRITE only.
    // This code does NOT open or identify an installed title, ever.
    auto source=rogue18::raw_v1::readExact(phase19::OfflineImportPath);
    if(!source)return {false,"Offline import absent, truncated or unreadable."};
    return exportGuarded(source.get(),CopiedRawBytes,display,captured,choice,
                         phase19::OfflineImportPath,"sdmc:/3ds/PKSM-Rogue");
}

enum class EditStage : uint8_t {
    Idle, Category, Field, Value, ConfirmOne, ConfirmTwo, Result
};
struct OfflineFlow {
    EditStage stage=EditStage::Idle;
    EditChoice choice{};
    phase19::Screen origin=phase19::Screen::Money;
    unsigned category=0,original=0,proposed=0;
    bool succeeded=false;
    char result[256]{};
};
inline const char* kindName(OfflineKind kind) {
    switch(kind) {
        case OfflineKind::Money:return "Money";
        case OfflineKind::ExistingItem:return "Existing item quantity";
        case OfflineKind::IV:return "Pokemon IV";
        case OfflineKind::EV:return "Pokemon EV";
        case OfflineKind::PP:return "Existing move PP";
    }
    return "Unsupported";
}
inline unsigned oldValue(const phase19::Demo& d,const EditChoice& c) {
    switch(c.kind) {
        case OfflineKind::Money:return d.money;
        case OfflineKind::ExistingItem:
            return c.slot<d.bagCount?d.bag[c.slot].quantity:0u;
        case OfflineKind::IV:
            return c.slot<d.partyCount && c.field<6?d.party[c.slot].iv[c.field]:0u;
        case OfflineKind::EV:
            return c.slot<d.partyCount && c.field<6?d.party[c.slot].ev[c.field]:0u;
        case OfflineKind::PP:
            return c.slot<d.partyCount && c.field<4?d.party[c.slot].pp[c.field]:0u;
    }
    return 0;
}
inline unsigned maxValue(const phase19::Demo& d,const EditChoice& c){
    switch(c.kind){
        case OfflineKind::Money:return 999999;
        case OfflineKind::ExistingItem:return 999;
        case OfflineKind::IV:return 31;
        case OfflineKind::PP:return 64;
        case OfflineKind::EV:{
            if(c.slot>=d.partyCount || c.field>=6)return 0;
            unsigned other=0;
            for(unsigned i=0;i<6;i++)if(i!=c.field)other+=d.party[c.slot].ev[i];
            return other>510u?0u:std::min(252u,510u-other);
        }
    }
    return 0;
}
inline unsigned fieldCount(OfflineKind kind){
    return kind==OfflineKind::IV||kind==OfflineKind::EV?6u:
           kind==OfflineKind::PP?4u:1u;
}
inline bool offlineVisible(const phase19::State& state,
                           const CopyObservation& observation) {
    return observation.structurallyReadable &&
           !observation.canModifyInstalledGame && !observation.canRestore &&
           !state.demo.synthetic && !state.demoPreviewEnabled &&
           state.demo.sourceSha256==observation.sourceSha256 &&
           state.demo.activeCounter==observation.activeCounter;
}
// ADDITIVE copied-save v3 queue. Never changes the current imported data or
// any installed title. Pending choices are locked against the same ORIGINAL.
enum class BatchStage : uint8_t { Browse, Review, ConfirmOne, ConfirmTwo, Result };
struct BatchFlow {
    bool collecting=false,success=false;
    BatchStage stage=BatchStage::Browse;
    rogue18::v3::Batch pending{};
    unsigned page=0;
    char message[256]{};
};
inline bool batchReady(const BatchFlow& q) {
    return q.collecting && !q.pending.edits.empty();
}
inline rogue18::Kind mapKind(OfflineKind k) {
    switch(k) {
        case OfflineKind::Money:return rogue18::Kind::Money;
        case OfflineKind::ExistingItem:return rogue18::Kind::ExistingQuantity;
        case OfflineKind::IV:return rogue18::Kind::IVs;
        case OfflineKind::EV:return rogue18::Kind::EVs;
        case OfflineKind::PP:return rogue18::Kind::Move;
    }
    return rogue18::Kind::Nature; // v3 explicitly denies unsupported kinds.
}
inline bool batchSource(const phase19::Demo& d,const CopyObservation& o,
                        const BatchFlow& batch,const uint8_t* raw,size_t n,
                        rogue18::v2::ReadSnapshot& view) {
    if(!raw || n!=CopiedRawBytes)return false;
    view=rogue18::v2::inspect(raw,n);
    return sameVisibleSource(d,o,view) &&
           batch.pending.expectedDigest==view.data.sourceSha256 &&
           batch.pending.expectedCounter==view.data.activeCounter &&
           batch.pending.version==rogue18::v3::ContractVersion &&
           !rogue18::v3::mayWriteInstalledGame(batch.pending);
}
inline rogue18::ExportResult exportBatchGuarded(
    const uint8_t* raw,size_t n,const phase19::Demo& display,
    const CopyObservation& observation,const BatchFlow& queued,
    const char* input,const char* root) {
    try {
        rogue18::v2::ReadSnapshot view{};
        if(!batchReady(queued) || !batchSource(display,observation,queued,raw,n,view))
            return {false,"Pending edit batch no longer matches the imported original."};
        const auto candidate=rogue18::v3::preview(raw,n,queued.pending);
        if(!candidate.ok || !rogue18::v3::verify(raw,n,
              candidate.candidate.data(),candidate.candidate.size(),queued.pending))
            return {false,"Multi-edit draft rejected by independent full-byte proof."};
        return rogue18::v3::exportCopiedBatch(raw,n,input,root,queued.pending);
    } catch(const std::bad_alloc&) {
        return {false,"Out of memory; no batch draft approved."};
    } catch(const std::exception&) {
        return {false,"Batch exception; no draft approved."};
    }
}
inline rogue18::ExportResult exportFixedBatch(const phase19::Demo& d,
    const CopyObservation& o,const BatchFlow& batch) {
    auto source=rogue18::raw_v1::readExact(phase19::OfflineImportPath);
    if(!source)return {false,"Missing or truncated original SD import."};
    return exportBatchGuarded(source.get(),CopiedRawBytes,d,o,batch,
               phase19::OfflineImportPath,"sdmc:/3ds/PKSM-Rogue");
}
using BatchSubmit=rogue18::ExportResult(*)(
    const phase19::Demo&,const CopyObservation&,const BatchFlow&,void*);
inline bool batchShortcut(const phase19::State& s,
    const CopyObservation& observed,BatchFlow& q,
    const uint8_t* syntheticSource=nullptr,size_t syntheticSize=0) {
    if(!offlineVisible(s,observed) ||
       (s.screen!=phase19::Screen::Home &&
        s.screen!=phase19::Screen::Money &&
        s.screen!=phase19::Screen::Bag &&
        s.screen!=phase19::Screen::Party &&
        s.screen!=phase19::Screen::Pokemon))return false;
    try {
        if(q.collecting) {
            q.stage=batchReady(q)?BatchStage::Review:BatchStage::Browse;
            q.page=0;
            if(!batchReady(q))
                std::snprintf(q.message,sizeof(q.message),
                    "Queue is empty. Select values with A to stage them.");
            return true;
        }
        auto disk=syntheticSource?std::unique_ptr<uint8_t[]>{}:
                  rogue18::raw_v1::readExact(phase19::OfflineImportPath);
        const uint8_t* raw=syntheticSource?syntheticSource:disk.get();
        const size_t n=syntheticSource?syntheticSize:CopiedRawBytes;
        if(!raw || n!=CopiedRawBytes)return false;
        const auto view=rogue18::v2::inspect(raw,n);
        if(!sameVisibleSource(s.demo,observed,view))return false;
        rogue18::v3::Batch pending{};
        if(!rogue18::v3::start(view,pending))return false;
        q=BatchFlow{};
        q.collecting=true;
        q.pending=std::move(pending);
        std::snprintf(q.message,sizeof(q.message),
                 "BATCH MODE: A to stage edits; Y to review and export.");
        return true;
    } catch(const std::exception&) {return false;}
}
inline bool addToBatch(const uint8_t* raw,size_t n,
    const phase19::Demo& display,const CopyObservation& observed,
    BatchFlow& q,const EditChoice& c,std::string& reason) {
    try {
        rogue18::v2::ReadSnapshot view{};
        if(!q.collecting || q.stage!=BatchStage::Browse ||
           !batchSource(display,observed,q,raw,n,view)) {
            reason="Original copied source changed; discard batch and re-import.";return false;
        }
        if(!rogue18::v3::append(view,q.pending,mapKind(c.kind),c.slot,c.field,c.value)) {
            reason="Duplicate, unchanged, unsupported or invalid batch edit.";return false;
        }
        const auto proof=rogue18::v3::preview(raw,n,q.pending);
        if(!proof.ok || proof.candidate.size()!=n ||
           !rogue18::v3::verify(raw,n,proof.candidate.data(),
                               proof.candidate.size(),q.pending)) {
            q.pending.edits.pop_back();
            reason="Combined values invalid (including EV budget); not queued.";
            return false;
        }
        reason="Staged to offline queue. Press Y to review entire batch.";
        return true;
    } catch(const std::exception&) {
        reason="Batch allocation/validation failed; no draft created.";
        return false;
    }
}
inline bool takeBatchKey(phase19::State& state,BatchFlow& q,
    const CopyObservation& observed,phase19::Key key,
    BatchSubmit submit=nullptr,void* context=nullptr) {
    using phase19::Key;
    if(q.stage==BatchStage::Browse)return false;
    if(!offlineVisible(state,observed)) {
        q=BatchFlow{};
        return true;
    }
    if(q.stage==BatchStage::Review){
        if(key==Key::B){q.stage=BatchStage::Browse;return true;}
        if(key==Key::X && !q.pending.edits.empty()) {
            q.pending.edits.pop_back();
            if(q.page>0 && q.page*5>=q.pending.edits.size())--q.page;
            if(q.pending.edits.empty())q.stage=BatchStage::Browse;
            return true;
        }
        if(key==Key::Right || key==Key::R) {
            if((q.page+1)*5<q.pending.edits.size())++q.page;
        }else if(key==Key::Left && q.page>0)--q.page;
        else if(key==Key::A && batchReady(q))q.stage=BatchStage::ConfirmOne;
        return true;
    }
    if(key==Key::B){
        q.stage=q.stage==BatchStage::Result?BatchStage::Browse:BatchStage::Review;
        return true;
    }
    if(key!=Key::A)return true;
    if(q.stage==BatchStage::ConfirmOne)q.stage=BatchStage::ConfirmTwo;
    else if(q.stage==BatchStage::ConfirmTwo) {
        try {
            const auto saved=submit?submit(state.demo,observed,q,context):
                 exportFixedBatch(state.demo,observed,q);
            q.success=saved.ok;
            if(q.success)
                std::snprintf(q.message,sizeof(q.message),
                   "ONE BATCH DRAFT: backups/%.42s drafts/%.42s. Original unchanged.",
                   saved.backupName.c_str(),saved.draftName.c_str());
            else
                std::snprintf(q.message,sizeof(q.message),
                    "BATCH BLOCKED: %.191s",saved.reason.c_str());
        } catch(const std::exception&) {
            q.success=false;
            std::snprintf(q.message,sizeof(q.message),
                "Batch export exception; no valid draft approved.");
        }
        q.stage=BatchStage::Result;
    } else if(q.stage==BatchStage::Result) {
        if(q.success)q=BatchFlow{};
        else q.stage=BatchStage::Review;
    }
    return true;
}
// Host-only QA may inject the SAME guarded export path with a temporary
// source directory. Production always defaults to the hard-coded native
// SDMC import + immutable backup/draft location.
using OfflineSubmit = rogue18::ExportResult(*)(
    const phase19::Demo&,const CopyObservation&,const EditChoice&,void*);
inline bool takeOfflineKey(phase19::State& state,OfflineFlow& flow,
                           const CopyObservation& observation,phase19::Key key,
                           OfflineSubmit submit=nullptr,void* testContext=nullptr,
                           BatchFlow* batch=nullptr,
                           const uint8_t* batchTestSource=nullptr,
                           size_t batchTestSize=0) {
    using phase19::Key;
    if(flow.stage==EditStage::Idle) {
        if(!offlineVisible(state,observation) || key!=Key::A)return false;
        flow=OfflineFlow{};
        if(state.screen==phase19::Screen::Money &&
           state.demo.moneyStatus==phase19::Availability::Ready) {
            flow.choice.kind=OfflineKind::Money;
            flow.stage=EditStage::Value;
        }else if(state.screen==phase19::Screen::Bag &&
                 state.demo.bagStatus==phase19::Availability::Ready &&
                 state.selected<state.demo.bagCount) {
            flow.choice.kind=OfflineKind::ExistingItem;
            flow.choice.slot=state.selected;
            flow.stage=EditStage::Value;
        }else if(state.screen==phase19::Screen::Pokemon && !state.fromPC &&
                 state.demo.partyStatus==phase19::Availability::Ready &&
                 state.mon<state.demo.partyCount &&
                 !state.demo.party[state.mon].egg) {
            flow.choice.slot=state.mon;
            flow.stage=EditStage::Category;
        }else return false; // Phase19's original read-only refusal applies.
        flow.origin=state.screen;
        flow.original=oldValue(state.demo,flow.choice);
        flow.proposed=flow.original;
        return true;
    }
    if(key==Key::Start)return false; // preserve native START release/exit
    if(!offlineVisible(state,observation)) {
        flow=OfflineFlow{};return true;
    }
    switch(flow.stage){
        case EditStage::Category:
            if(key==Key::B){flow=OfflineFlow{};break;}
            if(key==Key::R || key==Key::Right)flow.category=(flow.category+1u)%3u;
            if(key==Key::X || key==Key::Left)flow.category=(flow.category+2u)%3u;
            if(key==Key::A) {
                flow.choice.kind=flow.category==0?OfflineKind::IV:
                                 flow.category==1?OfflineKind::EV:OfflineKind::PP;
                flow.choice.field=0;
                flow.stage=EditStage::Field;
            }
            break;
        case EditStage::Field:
            if(key==Key::B){flow.stage=EditStage::Category;break;}
            if(key==Key::Up && flow.choice.field>0)--flow.choice.field;
            if(key==Key::Down && flow.choice.field+1<fieldCount(flow.choice.kind))
                ++flow.choice.field;
            if(key==Key::A) {
                if(flow.choice.kind==OfflineKind::PP &&
                   !state.demo.party[flow.choice.slot].moves[flow.choice.field]) {
                    std::snprintf(flow.result,sizeof(flow.result),
                                  "Empty move slot cannot be edited.");
                    flow.stage=EditStage::Result;
                    break;
                }
                flow.original=oldValue(state.demo,flow.choice);
                flow.proposed=flow.original;
                flow.stage=EditStage::Value;
            }
            break;
        case EditStage::Value:{
            if(key==Key::B) {
                if(flow.origin==phase19::Screen::Pokemon)flow.stage=EditStage::Field;
                else flow=OfflineFlow{};
                break;
            }
            const int delta=key==Key::Up?1:key==Key::Down?-1:
                            key==Key::Right||key==Key::R?10:
                            key==Key::Left?-10:0;
            const unsigned lo=flow.choice.kind==OfflineKind::ExistingItem?1u:0u;
            const unsigned hi=maxValue(state.demo,flow.choice);
            if(delta) {
                const int64_t v=static_cast<int64_t>(flow.proposed)+delta;
                flow.proposed=static_cast<unsigned>(std::max(int64_t(lo),
                                     std::min(int64_t(hi),v)));
            }
            if(key==Key::X)flow.proposed=flow.original;
            if(key==Key::A && flow.proposed!=flow.original &&
               flow.proposed>=lo && flow.proposed<=hi) {
                flow.choice.value=flow.proposed;
                flow.stage=EditStage::ConfirmOne;
            }
            break;
        }
        case EditStage::ConfirmOne:
            if(key==Key::B)flow.stage=EditStage::Value;
            if(key==Key::A && batch && batch->collecting &&
               batch->stage==BatchStage::Browse) {
                // Queueing is RAM-only; require final batch-wide double
                // confirmation before any SD file can be created.
                auto disk=batchTestSource?std::unique_ptr<uint8_t[]>{}:
                         rogue18::raw_v1::readExact(phase19::OfflineImportPath);
                const uint8_t* raw=batchTestSource?batchTestSource:disk.get();
                std::string reason;
                const bool ok=addToBatch(raw,batchTestSource?batchTestSize:CopiedRawBytes,
                                   state.demo,observation,*batch,flow.choice,reason);
                flow.succeeded=ok;
                std::snprintf(flow.result,sizeof(flow.result),"%s",reason.c_str());
                flow.stage=EditStage::Result;
                break;
            }
            if(key==Key::A)flow.stage=EditStage::ConfirmTwo;
            break;
        case EditStage::ConfirmTwo:
            if(key==Key::B)flow.stage=EditStage::Value;
            if(key==Key::A) {
                // Native SDMC import, SHA-locked preview and immutable SD
                // export can allocate multiple 128KiB vectors. Fail without
                // unwinding through the 3DS render/event loop on exhaustion.
                try {
                    const auto saved=submit ?
                        submit(state.demo,observation,flow.choice,testContext) :
                        exportFixedSDCopy(state.demo,observation,flow.choice);
                    flow.succeeded=saved.ok;
                    if(saved.ok)
                        std::snprintf(flow.result,sizeof(flow.result),
                          "OFFLINE COPY ONLY: backups/%s drafts/%s. Import unchanged. Not game-tested.",
                          saved.backupName.c_str(),saved.draftName.c_str());
                    else
                        std::snprintf(flow.result,sizeof(flow.result),
                          "EXPORT BLOCKED: %.215s",saved.reason.c_str());
                } catch (const std::bad_alloc&) {
                    flow.succeeded=false;
                    std::snprintf(flow.result,sizeof(flow.result),
                      "OUT OF MEMORY: no approved draft. Check separate SD output files.");
                } catch (const std::exception&) {
                    flow.succeeded=false;
                    std::snprintf(flow.result,sizeof(flow.result),
                      "EXPORT ERROR: no approved draft. Check separate SD output files.");
                }
                flow.stage=EditStage::Result;
            }
            break;
        case EditStage::Result:
            if(key==Key::A || key==Key::B)flow=OfflineFlow{};
            break;
        case EditStage::Idle:break;
    }
    return true;
}
} // namespace phase20
