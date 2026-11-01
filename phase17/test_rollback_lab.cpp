// SPDX-License-Identifier: GPL-3.0-or-later
// Nonplayable H168 synthetic save. Host-only shadow writer/rollback tests.
// NEVER contacts the installed game, SD card or privileged Nintendo services.
#include "rollback_lab.hpp"
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
            const auto report=phase17::rehearse(source,record,signer);
            assert(report.code==phase17::ResultCode::Passed);
            assert(report.allByteExact);
            assert(report.normalRestores==1);
            assert(report.explicitRollbacks==1);
            assert(report.shortWriteRollbacks==4);
            assert(report.crashRestarts==4);
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
             <<"explicit undo, 4 short writes, 4 crash/restart rollbacks, "
             <<"foreign-byte refusal, MAC/journal corruption guards. "
             <<"NO LIVE WRITE ON 3DS.\n";
}
