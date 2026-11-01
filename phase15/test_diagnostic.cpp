// SPDX-License-Identifier: GPL-3.0-or-later
// Deterministic, nonplayable save fixture. Mock MAC != console CMAC.
#include "diagnostic.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
using phase14::Bytes;
struct FakeDevice {
    Bytes live;
    int reads=0,macCalls=0,readFailAt=0,macFailAt=0;
    bool mutateSecond=false,unstableMac=false,staleFirstMac=false;
    explicit FakeDevice(Bytes b):live(std::move(b)){}
    bool read(Bytes& dst){
        ++reads;
        if(readFailAt==reads)return false;
        dst=live;
        if(mutateSecond&&reads==2)dst.back()^=1;
        return true;
    }
    bool calculate(const phase14::Digest& digest,phase14::Mac& mac){
        ++macCalls;
        if(macFailAt==macCalls)return false;
        const auto hash=phase14::hash(digest.data(),digest.size());
        std::copy_n(hash.begin(),mac.size(),mac.begin());
        if(unstableMac)mac[0]^=static_cast<uint8_t>(macCalls);
        if(staleFirstMac&&macCalls==1)mac[0]^=0x80;
        return true;
    }
};
static Bytes makeContainer(const Bytes& raw){
    Bytes b(rogue_live::ContainerMin+128,0x5a);
    for(size_t off:{size_t(0),size_t(rogue_live::SlotSize)}){
        std::fill_n(b.begin()+off,rogue_live::HeaderSize,0);
        std::memcpy(b.data()+off,".SAV",4);
        h168::w32(b.data(),off+0x34,off?11:12);
        h168::w32(b.data(),off+0x38,uint32_t(rogue_live::ExpectedTitle));
        h168::w32(b.data(),off+0x3c,uint32_t(rogue_live::ExpectedTitle>>32));
        std::fill_n(b.begin()+off+0x40,16,0x43);
        h168::w32(b.data(),off+0x50,rogue_live::HeaderSize);
        h168::w32(b.data(),off+0x54,rogue_live::RawSize);
        std::copy(raw.begin(),raw.end(),b.begin()+off+rogue_live::HeaderSize);
        FakeDevice fake(b);
        phase14::Mac mac{};
        assert(phase14::stableMac(fake,phase14::slotHash(b,off),mac));
        std::copy(mac.begin(),mac.end(),b.begin()+off+0x10);
    }
    return b;
}
static phase15::Report check(const Bytes& c,phase15::Code expected){
    FakeDevice d(c);
    const auto r=phase15::run(d);
    assert(r.code==expected);
    return r;
}
int main(int argc,char** argv){
    assert(argc==2);
    std::ifstream f(argv[1],std::ios::binary);
    Bytes raw((std::istreambuf_iterator<char>(f)),{});
    assert(raw.size()==h168::Size);
    h168::Info info{};
    assert(h168::inspect(raw.data(),raw.size(),info));
    const Bytes baseline=makeContainer(raw);
    auto r=check(baseline,phase15::Code::Pass);
    assert(r.stable && r.slots[0].header && r.slots[1].header);
    assert(r.slots[0].raw && r.slots[1].raw);
    assert(r.slots[0].cmac && r.slots[1].cmac);
    assert(r.activeSlot==0 && r.latestCounter==12);
    assert(r.containerBytes==baseline.size());
    FakeDevice first(baseline);first.readFailAt=1;
    assert(phase15::run(first).code==phase15::Code::FirstReadFailed);
    FakeDevice second(baseline);second.readFailAt=2;
    assert(phase15::run(second).code==phase15::Code::SecondReadFailed);
    FakeDevice changed(baseline);changed.mutateSecond=true;
    assert(phase15::run(changed).code==phase15::Code::SnapshotChanged);
    auto bad=baseline;
    bad.resize(rogue_live::ContainerMin-1);
    check(bad,phase15::Code::ContainerSizeInvalid);
    bad=baseline;bad.resize(phase14::MaxContainer+1);
    check(bad,phase15::Code::ContainerSizeInvalid);
    bad=baseline;bad[0]='X';check(bad,phase15::Code::HeaderInvalid);
    bad=baseline;bad[0x38]^=1;check(bad,phase15::Code::ForeignTitle);
    bad=baseline;bad[rogue_live::SlotSize+0x40]^=1;
    check(bad,phase15::Code::CidMismatch);
    bad=baseline;h168::w32(bad.data(),rogue_live::SlotSize+0x34,12);
    check(bad,phase15::Code::CounterInvalid);
    bad=baseline;bad[rogue_live::HeaderSize+0x100]^=1;
    check(bad,phase15::Code::RawInvalid);
    bad=baseline;bad[0x10]^=1;
    check(bad,phase15::Code::CmacMismatch);
    bad=baseline;bad[rogue_live::SlotSize+0x10]^=1;
    check(bad,phase15::Code::CmacMismatch);
    FakeDevice service(baseline);service.macFailAt=1;
    assert(phase15::run(service).code==phase15::Code::CmacServiceFailed);
    FakeDevice unstable(baseline);unstable.unstableMac=true;
    assert(phase15::run(unstable).code==phase15::Code::CmacServiceFailed);
    FakeDevice stale(baseline);stale.staleFirstMac=true;
    assert(phase15::run(stale).code==phase15::Code::Pass);
    // Every failure mode must preserve the original bytes without any writes.
    assert(baseline==makeContainer(raw));
    std::cout<<"PASS Phase15: fresh RO reads, header/title/CID/counter/raw, stable MAC, faults. MAC is TEST DOUBLE only.\n";
}
