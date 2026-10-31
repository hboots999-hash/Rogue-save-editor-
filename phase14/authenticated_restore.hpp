// SPDX-License-Identifier: GPL-3.0-or-later
// Original Phase 14 implementation; see REFERENCES.md for protocol attribution.
#pragma once
#include "live_header.hpp"
#include "restore_preflight.hpp"
#include <algorithm>
#include <array>
#include <vector>
extern "C" {
#include "vendor/sha256.h"
}
namespace phase14 {
using Bytes = std::vector<uint8_t>;
using Digest = std::array<uint8_t,32>;
using Mac = std::array<uint8_t,16>;
constexpr size_t MaxContainer = 1024*1024;
constexpr bool LiveWritesEnabled = false; // Deliberately no build flag override.
inline Digest hash(const uint8_t* data,size_t size) {
    Digest d{}; SHA256_CTX c{}; sha256_init(&c);
    sha256_update(&c,data,size); sha256_final(&c,d.data()); return d;
}
inline Digest slotHash(const Bytes& image,size_t offset) {
    if(offset>image.size() || image.size()-offset<rogue_live::SlotSize) return {};
    return hash(image.data()+offset+0x30,rogue_live::SlotSize-0x30);
}
// A service success alone is insufficient. Exhaustion is a hard failure.
// Signer is bound to the target archive/file on the CURRENT console.
template<class Signer> bool stableMac(Signer& signer,const Digest& digest,Mac& out) {
    Mac previous{},next{};
    for(unsigned i=0;i<10;i++) {
        if(!signer.calculate(digest,next)) return false;
        if(i && next==previous) {out=next;return true;}
        previous=next;
    }
    return false;
}
struct Layout {size_t active=0, target=0; uint32_t counter=0;};
inline bool layout(const Bytes& b,Layout& l) {
    using namespace rogue_live;
    if(b.size()<ContainerMin || b.size()>MaxContainer) return false;
    const auto a=parseHeader(b.data(),HeaderSize,0);
    const auto z=parseHeader(b.data()+SlotSize,HeaderSize,SlotSize);
    // Stronger than the read-only Phase 11 selector: no unknown-size/foreign
    // slot fallback, equal/rollover counters or uninitialized recovery slot.
    if(!a.valid || !z.valid || a.title!=ExpectedTitle || z.title!=ExpectedTitle ||
       a.timesSaved==z.timesSaved) return false;
    const auto chosen=a.timesSaved>z.timesSaved?a:z;
    if(chosen.timesSaved==UINT32_MAX) return false;
    if(std::memcmp(b.data()+0x40,b.data()+SlotSize+0x40,16)) return false;
    l.active=chosen.offset; l.target=l.active?0:SlotSize; l.counter=chosen.timesSaved;
    return true;
}
template<class Signer> bool authenticate(const Bytes& image,Signer& signer,Layout& l) {
    if(!layout(image,l)) return false;
    for(size_t off:{size_t(0),size_t(rogue_live::SlotSize)}) {
        Mac actual{}; h168::Info info{};
        if(!stableMac(signer,slotHash(image,off),actual) ||
           std::memcmp(actual.data(),image.data()+off+0x10,actual.size()) ||
           !h168::inspect(image.data()+off+rogue_live::HeaderSize,h168::Size,info))
            return false;
    }
    return true;
}
struct Plan {Bytes before,after; Layout slots{}; rogue_restore::Operation op{}; unsigned item=0;};
template<class Signer> bool prepare(const Bytes& container,const Bytes& raw,
        rogue_restore::Operation op,unsigned item,Signer& signer,Plan& out) {
    out={}; Layout l{};
    if(raw.size()!=h168::Size || !authenticate(container,signer,l) ||
       !rogue_restore::validate(container.data()+l.active+rogue_live::HeaderSize,
                                raw.data(),op,item).ok) return false;
    Plan p{container,container,l,op,item};
    // Stage into the older slot. The currently authenticated slot is untouched.
    std::copy_n(container.data()+l.active,rogue_live::HeaderSize,p.after.data()+l.target);
    h168::w32(p.after.data(),l.target+0x34,l.counter+1);
    std::copy(raw.begin(),raw.end(),p.after.begin()+l.target+rogue_live::HeaderSize);
    Mac tag{};
    if(!stableMac(signer,slotHash(p.after,l.target),tag)) return false;
    std::copy(tag.begin(),tag.end(),p.after.begin()+l.target+0x10);
    out=std::move(p); return true;
}
// A loaded journal/plan is untrusted. Rebuild all bytes before any mutation.
template<class Signer> bool validatePlan(const Plan& p,Signer& signer) {
    Layout l{};
    if(!layout(p.before,l) || p.after.size()!=p.before.size() ||
       p.slots.active!=l.active || p.slots.target!=l.target || p.slots.counter!=l.counter)
        return false;
    Bytes raw(p.after.begin()+l.target+rogue_live::HeaderSize,
              p.after.begin()+l.target+rogue_live::SlotSize);
    Plan rebuilt;
    return prepare(p.before,raw,p.op,p.item,signer,rebuilt) && rebuilt.after==p.after;
}
enum class Status {Blocked, Restored, RolledBack, RecoveryRequired};
// Storage contract: fresh whole-container reads; exact-count flushed writes;
// exclusive immutable journal containing BOTH images + metadata, readback;
// unresolved() fails closed on IO errors. finish() durably records a result.
// No native implementation is permitted to authorize writes in this phase.
template<class IO> bool writeSlot(IO& io,const Bytes& image,size_t off) {
    using namespace rogue_live;
    // Header excluding CMAC first follows payload. CMAC is the final commit.
    return io.write(off+HeaderSize,image.data()+off+HeaderSize,RawSize) &&
           io.write(off,image.data()+off,0x10) &&
           io.write(off+0x20,image.data()+off+0x20,HeaderSize-0x20) &&
           io.write(off+0x10,image.data()+off+0x10,16);
}
// Accept a partial write only when every target byte is old OR intended new;
// unrelated title/header/tail changes require manual recovery, never overwrite.
inline bool belongsToTransaction(const Bytes& current,const Plan& p) {
    if(current.size()!=p.before.size())return false;
    for(size_t i=0;i<current.size();i++)
        if(current[i]!=p.before[i] && current[i]!=p.after[i])return false;
    return true;
}
template<class IO,class Signer> Status rollback(IO& io,Signer& signer,const Plan& p) {
    Bytes now; Layout l{};
    if(!io.read(now) || !belongsToTransaction(now,p))return Status::RecoveryRequired;
    if(now!=p.before && !writeSlot(io,p.before,p.slots.target))return Status::RecoveryRequired;
    if(!io.read(now) || now!=p.before || !authenticate(now,signer,l) ||
       !io.finish(Status::RolledBack)) return Status::RecoveryRequired;
    return Status::RolledBack;
}
template<class IO,class Signer> Status execute(IO& io,Signer& signer,const Plan& p) {
    if(!io.writesAllowed() || io.unresolved() || !validatePlan(p,signer))return Status::Blocked;
    Bytes now;
    if(!io.read(now) || now!=p.before || !io.persist(p) || !io.verifyJournal(p))return Status::Blocked;
    // Last full-container freshness check AFTER durable recovery creation.
    if(!io.read(now) || now!=p.before)return Status::RecoveryRequired;
    Layout l{};
    if(!writeSlot(io,p.after,p.slots.target) || !io.read(now) || now!=p.after ||
       !authenticate(now,signer,l))return rollback(io,signer,p);
    if(!io.finish(Status::Restored))return Status::RecoveryRequired;
    return Status::Restored;
}
// Explicit restart recovery, never automatic replay of a candidate edit.
template<class IO,class Signer> Status recover(IO& io,Signer& signer,const Plan& p) {
    if(!io.writesAllowed() || !io.unresolved() || !io.verifyJournal(p) ||
       !validatePlan(p,signer))return Status::Blocked;
    return rollback(io,signer,p);
}
} // namespace phase14
