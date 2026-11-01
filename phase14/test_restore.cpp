// SPDX-License-Identifier: GPL-3.0-or-later
#include "host_journal.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace phase14;
namespace fs=std::filesystem;
// Deterministic service DOUBLE, not console AES-CMAC. Host tests never prove
// native authentication. It binds each result to all hashed input bytes.
struct Signer {
    int calls=0,failAt=0;bool unstable=false,staleFirst=false;
    bool calculate(const Digest& d,Mac& m) {
        ++calls;if(calls==failAt)return false;
        auto h=hash(d.data(),d.size());std::copy_n(h.begin(),16,m.begin());
        if(unstable)m[0]^=uint8_t(calls);
        if(staleFirst && calls==1)m[0]^=0x80;
        return true;
    }
};
struct PowerLoss{};
struct Device:HostJournal {
    Bytes live;int writes=0,reads=0,failWrite=0,crashWrite=0,failRead=0,staleRead=0;
    bool allWritesFail=false,deny=false,badBackup=false,badVerify=false,badFinish=false;
    Device(const fs::path& p,const Bytes& b):HostJournal(p.string()),live(b){}
    bool writesAllowed(){return !deny;}
    bool read(Bytes& b){++reads;if(reads==failRead)return false;
        if(reads==staleRead)live.back()^=1;
        b=live;return true;}
    bool write(size_t off,const uint8_t* p,size_t n){
        ++writes;assert(off+n<=live.size());
        if(allWritesFail)return false;
        const bool partial=writes==failWrite;
        std::copy_n(p,partial?n/2:n,live.begin()+off);
        if(writes==crashWrite)throw PowerLoss{};
        return !partial;
    }
    bool persist(const Plan& p){return !badBackup&&HostJournal::persist(p);}
    bool verifyJournal(const Plan& p){return !badVerify&&HostJournal::verifyJournal(p);}
    bool finish(Status s){return !badFinish&&HostJournal::finish(s);}
};
static Bytes container(const Bytes& raw,Signer& s,bool topActive=false) {
    Bytes b(rogue_live::ContainerMin+128,0x5a);
    for(size_t off:{size_t(0),size_t(rogue_live::SlotSize)}){
        std::fill_n(b.begin()+off,rogue_live::HeaderSize,0);
        std::memcpy(b.data()+off,".SAV",4);
        h168::w32(b.data(),off+0x34,(off==0)==topActive?12:11);
        h168::w32(b.data(),off+0x38,uint32_t(rogue_live::ExpectedTitle));
        h168::w32(b.data(),off+0x3c,uint32_t(rogue_live::ExpectedTitle>>32));
        std::fill_n(b.begin()+off+0x40,16,0x43);
        h168::w32(b.data(),off+0x50,rogue_live::HeaderSize);
        h168::w32(b.data(),off+0x54,rogue_live::RawSize);
        std::copy(raw.begin(),raw.end(),b.begin()+off+rogue_live::HeaderSize);
        Mac tag{};assert(stableMac(s,slotHash(b,off),tag));
        std::copy(tag.begin(),tag.end(),b.begin()+off+0x10);
    }return b;
}
int main(int argc,char** argv) {
    assert(argc==2);static_assert(!LiveWritesEnabled,"Never enable live writes in this phase");
    // Published SHA-256 known-answer vectors (empty, abc, million a).
    auto hex=[](const Digest& d){std::string s;const char* h="0123456789abcdef";
        for(auto x:d){s+=h[x>>4];s+=h[x&15];}return s;};
    assert(hex(hash(nullptr,0))=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    assert(hex(hash(reinterpret_cast<const uint8_t*>("abc"),3))=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    Bytes million(1000000,'a');assert(hex(hash(million.data(),million.size()))=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    Bytes raw;assert(readFile(argv[1],raw));h168::Info info{};
    assert(h168::inspect(raw.data(),raw.size(),info));
    char temp[]="/tmp/phase14-XXXXXX";const char* root=mkdtemp(temp);assert(root);
    unsigned sequence=0;
    auto journal=[&](){return fs::path(root)/("recovery-"+std::to_string(++sequence));};
    Signer signer;const auto source=container(raw,signer);
    rogue_restore::Validation proof;
    auto candidate=rogue_restore::prepare(raw.data(),rogue_restore::Operation::MoneyPlusOne,0,proof);
    assert(candidate && proof.ok);Bytes edited(candidate.get(),candidate.get()+h168::Size);
    Plan p;assert(prepare(source,edited,rogue_restore::Operation::MoneyPlusOne,0,signer,p));
    assert(validatePlan(p,signer));
    // Domain, hash range and corruption checks, including excluded CMAC bytes.
    auto c=source;c[0x10]^=1;assert(slotHash(c,0)==slotHash(source,0));
    Layout l;assert(!authenticate(c,signer,l));
    c=source;c[0x30]^=1;assert(slotHash(c,0)!=slotHash(source,0));
    assert(!authenticate(c,signer,l));
    for(size_t off:{size_t(0),size_t(rogue_live::SlotSize)}) {
        for(size_t field:{size_t(0),size_t(0x38),size_t(0x40),size_t(0x50),size_t(0x54),size_t(0x200)}){
            c=source;c[off+field]^=1;assert(!authenticate(c,signer,l));
        }
    }
    c=source;h168::w32(c.data(),0x34,12);assert(!layout(c,l));
    c=source;h168::w32(c.data(),rogue_live::SlotSize+0x34,UINT32_MAX);assert(!layout(c,l));
    // QA-02: wrapper counters must be adjacent in the non-wrapping
    // transaction domain, irrespective of which physical slot is active.
    // Raw H168 internal bank counters follow a distinct validation policy.
    c=source;h168::w32(c.data(),0x34,10);assert(!layout(c,l)); // 10,12
    c=source;h168::w32(c.data(),0x34,14);assert(!layout(c,l)); // 14,12
    c=source;h168::w32(c.data(),0x34,15);assert(!layout(c,l)); // 15,12
    c=source;h168::w32(c.data(),0x34,0);
    h168::w32(c.data(),rogue_live::SlotSize+0x34,UINT32_MAX);
    assert(!layout(c,l)); // explicit rollover refusal
    c=source;h168::w32(c.data(),0x34,0);
    h168::w32(c.data(),rogue_live::SlotSize+0x34,1);
    assert(layout(c,l) && l.counter==1 && l.active==rogue_live::SlotSize);
    c=source;h168::w32(c.data(),0x34,UINT32_MAX-1);
    h168::w32(c.data(),rogue_live::SlotSize+0x34,UINT32_MAX);
    assert(!layout(c,l)); // high counter cannot be incremented safely
    c=source;h168::w32(c.data(),0x34,UINT32_MAX-2u);
    h168::w32(c.data(),rogue_live::SlotSize+0x34,UINT32_MAX-1u);
    assert(!layout(c,l)); // would stage forbidden UINT32_MAX
    // Normal two-slot orientation must work with the real observed 49/48.
    c=source;h168::w32(c.data(),0x34,49);
    h168::w32(c.data(),rogue_live::SlotSize+0x34,48);
    assert(layout(c,l) && l.counter==49 && l.active==0);
    c=source;h168::w32(c.data(),0x34,48);
    h168::w32(c.data(),rogue_live::SlotSize+0x34,49);
    assert(layout(c,l) && l.counter==49 && l.active==rogue_live::SlotSize);
    // Full wrapper selector matrix. A failed second inspection must not
    // accidentally reuse the selected offset/counter from the prior success.
    // All test records are synthetic and NEVER used to authorise native IO.
    struct CounterPair { uint32_t a,b; bool accepted; size_t active; };
    const CounterPair pairs[]={
        {48,49,true,rogue_live::SlotSize},
        {49,48,true,0},
        {0,1,true,rogue_live::SlotSize},
        {1,0,true,0},
        {49,49,false,0},
        {0,0,false,0},
        {48,60,false,0},
        {60,48,false,0},
        {50,48,false,0},
        {48,50,false,0},
        {UINT32_MAX,0,false,0},
        {0,UINT32_MAX,false,0},
        {UINT32_MAX-2u,UINT32_MAX-1u,false,0},
        {UINT32_MAX-1u,UINT32_MAX,false,0}
    };
    for(const auto& pair:pairs){
        c=source;
        h168::w32(c.data(),0x34,pair.a);
        h168::w32(c.data(),rogue_live::SlotSize+0x34,pair.b);
        l={rogue_live::SlotSize,0,12345}; // poisoned prior result
        const bool valid=layout(c,l);
        assert(valid==pair.accepted);
        if(valid){
            assert(l.active==pair.active);
            assert(l.target==(pair.active?0:rogue_live::SlotSize));
            assert(l.counter==std::max(pair.a,pair.b));
        }else {
            assert(l.active==0 && l.target==0 && l.counter==0);
        }
    }
    // A structurally selectable layout can still fail CMAC/sector proofs.
    // No stale "authenticated" layout may survive such a failure.
    c=source;
    l={rogue_live::SlotSize,0,49};
    c[rogue_live::SlotSize+rogue_live::HeaderSize+10]^=1;
    assert(!authenticate(c,signer,l));
    assert(l.active==0 && l.target==0 && l.counter==0);
    c=source;
    c[0x40]^=1; // CID disagreement even though both headers still exist
    l={rogue_live::SlotSize,0,49};
    assert(!layout(c,l));
    assert(l.active==0 && l.target==0 && l.counter==0);
    c=source;c.resize(rogue_live::ContainerMin-1);assert(!layout(c,l));
    c.resize(MaxContainer+1);assert(!layout(c,l));
    Signer unstable;unstable.unstable=true;Mac m{};
    assert(!stableMac(unstable,slotHash(source,0),m) && unstable.calls==10);
    Signer failed;failed.failAt=2;assert(!stableMac(failed,slotHash(source,0),m));
    Signer stale;stale.staleFirst=true;assert(stableMac(stale,slotHash(source,0),m)&&stale.calls==3);
    assert(!prepare(source,raw,rogue_restore::Operation::MoneyPlusOne,0,signer,p));
    assert(prepare(source,edited,rogue_restore::Operation::MoneyPlusOne,0,signer,p));
    auto tampered=p;tampered.after.back()^=1;assert(!validatePlan(tampered,signer));
    tampered=p;tampered.slots.target=p.slots.active;assert(!validatePlan(tampered,signer));
    // Both slot directions and both allowed Phase 13 edit operations.
    for(bool top:{false,true})for(auto op:{rogue_restore::Operation::MoneyPlusOne,rogue_restore::Operation::ItemQuantityPlusOne}){
        auto b=container(raw,signer,top);auto draft=rogue_restore::prepare(raw.data(),op,0,proof);assert(draft);
        Plan plan;assert(prepare(b,Bytes(draft.get(),draft.get()+h168::Size),op,0,signer,plan));
        Device io(journal(),b);assert(execute(io,signer,plan)==Status::Restored);
        assert(io.live==plan.after && io.writes==4);
        assert(std::equal(b.begin()+plan.slots.active,b.begin()+plan.slots.active+rogue_live::SlotSize,
                          io.live.begin()+plan.slots.active));
        assert(execute(io,signer,plan)==Status::Blocked);
        Plan reloaded;assert(io.load(reloaded));assert(recover(io,signer,reloaded)==Status::RolledBack);
        assert(io.live==b);
    }
    // Every write fails partially, then rollback restores exact full container.
    for(int fail=1;fail<=4;fail++){
        Device io(journal(),source);io.failWrite=fail;
        assert(execute(io,signer,p)==Status::RolledBack);assert(io.live==source);
    }
    // Abrupt termination after each write: new journal reader, no RAM plan.
    for(int step=1;step<=4;step++){
        const auto path=journal();Device io(path,source);io.crashWrite=step;
        try{execute(io,signer,p);assert(false);}catch(const PowerLoss&){}
        Device restart(path,io.live);Plan loaded;assert(restart.load(loaded));
        assert(execute(restart,signer,loaded)==Status::Blocked && restart.writes==0);
        assert(recover(restart,signer,loaded)==Status::RolledBack && restart.live==source);
    }
    // Denial, backup failure, journal mismatch, stale source: zero mutation.
    for(int mode=0;mode<7;mode++){
        Device io(journal(),source);
        io.deny=mode==0;io.badBackup=mode==1;io.badVerify=mode==2;
        if(mode==3)io.failRead=1;
        if(mode==4)io.staleRead=1;
        if(mode==5)io.failRead=2;
        if(mode==6)io.staleRead=2;
        const auto result=execute(io,signer,p);
        assert(result==Status::Blocked || result==Status::RecoveryRequired);assert(io.writes==0);
    }
    {Device io(journal(),source);io.failRead=3;assert(execute(io,signer,p)==Status::RolledBack);assert(io.live==source);}
    {Device io(journal(),source);io.badFinish=true;assert(execute(io,signer,p)==Status::RecoveryRequired);
     io.badFinish=false;assert(recover(io,signer,p)==Status::RolledBack);}
    {Device io(journal(),source);io.failWrite=1;io.allWritesFail=true;
     // No bytes changed, rollback can confirm original without another write.
     assert(execute(io,signer,p)==Status::RolledBack);}
    {Device io(journal(),source);assert(io.persist(p));io.live=p.after;io.allWritesFail=true;
     assert(recover(io,signer,p)==Status::RecoveryRequired);assert(io.unresolved());}
    {Device io(journal(),source);assert(io.persist(p));io.live.back()^=1;
     assert(recover(io,signer,p)==Status::RecoveryRequired && io.writes==0);}
    // Truncated/tampered recovery records and exclusive non-overwrite.
    {const auto path=journal();HostJournal j(path);assert(j.persist(p));assert(!j.persist(p));
     Bytes record;assert(readFile(path,record));Plan parsed;assert(decodeJournal(record,parsed));
     for(size_t i:{size_t(0),size_t(8),size_t(12),size_t(32),record.size()-1}){
        auto broken=record;broken[i]^=1;assert(!decodeJournal(broken,parsed));}
     record.pop_back();assert(!decodeJournal(record,parsed));}
    fs::remove_all(root);
    std::cout<<"PASS Phase14: SHA vectors; authenticated plan; both slots/edits; prewrite gates; partial-write rollback; restart recovery; journal corruption. Console CMAC NOT tested.\n";
}
