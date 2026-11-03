// SPDX-License-Identifier: GPL-3.0-or-later
// Issue #15: entirely synthetic, NO SD/GBA/PKSM writer and no actual 3DS.
#include "installed_save_probe_private.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstring>
using phase14::Bytes;
struct FakeDevice {
    Bytes image{};
    unsigned reads=0, failAt=0, flipAt=0, truncateAt=0;
    bool writable=false,session=false,failMac=false,unstableMac=false;
    unsigned macCalls=0;
    bool writesAllowed()const{return writable;}
    bool read(Bytes& out){
        out.clear();++reads;
        if(reads==failAt)return false;
        session=true;out=image;
        if(reads==flipAt)out.back()^=0x40;
        if(reads==truncateAt)out.pop_back();
        return true;
    }
    bool calculate(const phase14::Digest& d,phase14::Mac& m){
        if(!session || failMac)return false;
        ++macCalls;
        const auto digest=phase14::hash(d.data(),d.size());
        std::copy_n(digest.begin(),m.size(),m.begin());
        if(unstableMac)m[0]^=static_cast<uint8_t>(macCalls);
        return true;
    }
};
static Bytes wrapped(const Bytes& raw,FakeDevice& d,bool secondActive){
    Bytes b(P17_SAVE_PROBE_CONTAINER_BYTES,0x5a);
    d.session=true;
    for(size_t slot:{size_t(0),size_t(rogue_live::SlotSize)}) {
        std::fill_n(b.begin()+slot,rogue_live::HeaderSize,0);
        std::memcpy(b.data()+slot,".SAV",4);
        h168::w32(b.data(),slot+0x34,
                  (slot!=0)==secondActive?49u:48u);
        h168::w32(b.data(),slot+0x38,
                  uint32_t(rogue_live::ExpectedTitle));
        h168::w32(b.data(),slot+0x3c,
                  uint32_t(rogue_live::ExpectedTitle>>32));
        std::fill_n(b.begin()+slot+0x40,16,0x43);
        h168::w32(b.data(),slot+0x50,rogue_live::HeaderSize);
        h168::w32(b.data(),slot+0x54,rogue_live::RawSize);
        std::copy(raw.begin(),raw.end(),
                  b.begin()+slot+rogue_live::HeaderSize);
        phase14::Mac tag{};
        assert(phase14::stableMac(d,phase14::slotHash(b,slot),tag));
        std::copy(tag.begin(),tag.end(),b.begin()+slot+0x10);
    }
    return b;
}
static void blocked(FakeDevice d,P17SaveProbeCodeV1 why){
    P17SaveProbeReportV1 out{};
    // Poison both output objects to confirm denied results cannot leak stale
    // authenticated reports or stale copied raw bytes to original PKSM.
    std::memset(&out,0x5a,sizeof(out));
    Bytes copy(19,0x64);
    assert(!phase17::inspectInstalledSaveCopyV1(d,out,copy));
    assert(copy.empty());
    assert(out.code==why);
    assert(p17_save_probe_report_shape_v1(&out));
    assert(out.capability_bits==0 && out.container_bytes==0 &&
           out.raw_bytes==0 && out.title_id==0);
}
int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(!phase14::LiveWritesEnabled,"write gate must remain false");
    static_assert(sizeof(P17SaveProbeReportV1)==128,"C ABI must be fixed");
    std::ifstream in(argv[1],std::ios::binary);
    const Bytes raw((std::istreambuf_iterator<char>(in)),{});
    assert(raw.size()==P17_SAVE_PROBE_RAW_BYTES);
    h168::Info parsed{};assert(h168::inspect(raw.data(),raw.size(),parsed));
    P17SaveProbeReportV1 out{};
    assert(p17_save_probe_init_v1(&out));
    assert(p17_save_probe_report_shape_v1(&out));
    for(bool secondActive:{false,true}) {
        FakeDevice device{};
        device.image=wrapped(raw,device,secondActive);
        const Bytes unchanged=device.image;
        Bytes copy{};
        assert(phase17::inspectInstalledSaveCopyV1(device,out,copy));
        assert(device.reads==3);
        assert(out.code==P17_SAVE_READ_ONLY_VERIFIED);
        assert(out.capability_bits==0 && out.raw_bytes==131072u &&
               out.container_bytes==263168u);
        assert(out.title_id==rogue_live::ExpectedTitle);
        assert(out.active_slot==(secondActive?1u:0u));
        assert(out.slot0_counter==(secondActive?48u:49u));
        assert(out.slot1_counter==(secondActive?49u:48u));
        assert(copy==raw && device.image==unchanged);
        assert(p17_save_probe_report_shape_v1(&out));
        const auto digest=phase14::hash(unchanged.data(),unchanged.size());
        const auto rawHash=phase14::hash(raw.data(),raw.size());
        assert(std::equal(digest.begin(),digest.end(),out.container_sha256));
        assert(std::equal(rawHash.begin(),rawHash.end(),
                          out.active_raw_sha256));
        copy[0]^=1;assert(device.image==unchanged);
        // Independent reads, not simply a purported "verified" marker.
        for(unsigned n=1;n<=3;n++){
            auto fail=device;fail.failAt=n;fail.reads=0;
            blocked(fail,P17_SAVE_READ_FAILED);
        }
        for(unsigned n=1;n<=3;n++){
            auto truncated=device;truncated.truncateAt=n;
            truncated.reads=0;
            blocked(truncated,P17_SAVE_WRONG_CONTAINER_SIZE);
        }
        for(unsigned n:{2u,3u}){
            auto changed=device;changed.flipAt=n;changed.reads=0;
            blocked(changed,P17_SAVE_UNSTABLE_SNAPSHOT);
        }
        auto rw=device;rw.writable=true;rw.reads=0;
        blocked(rw,P17_SAVE_AUTHENTICATION_FAILED);
        auto macFail=device;macFail.failMac=true;macFail.reads=0;
        blocked(macFail,P17_SAVE_AUTHENTICATION_FAILED);
        auto jitter=device;jitter.unstableMac=true;jitter.reads=0;
        jitter.macCalls=0;
        blocked(jitter,P17_SAVE_AUTHENTICATION_FAILED);
        for(size_t off:{size_t(0),size_t(rogue_live::SlotSize)}) {
            for(size_t pos:{size_t(0),size_t(0x10),size_t(0x38),
                            size_t(0x40),size_t(0x50),
                            size_t(rogue_live::HeaderSize+81)}) {
                auto bad=device;bad.reads=0;
                bad.image[off+pos]^=0x01;
                blocked(bad,P17_SAVE_AUTHENTICATION_FAILED);
            }
        }
        // Conservative wrapper-slot counter policy; independent of the
        // inner raw bank-counter semantics.
        for(const auto pair:{std::pair<uint32_t,uint32_t>{49,49},
                             {48,60},{60,48},{0,UINT32_MAX},
                             {UINT32_MAX-2u,UINT32_MAX-1u}}) {
            auto bad=device;bad.reads=0;
            h168::w32(bad.image.data(),0x34,pair.first);
            h168::w32(bad.image.data()+rogue_live::SlotSize,0x34,
                      pair.second);
            blocked(bad,P17_SAVE_AUTHENTICATION_FAILED);
        }
        // Shape is never cryptographic identity: these forged status reports
        // cannot give the PKSM UI native installed-title write permission.
        auto forged=out;forged.capability_bits=1;
        assert(!p17_save_probe_report_shape_v1(&forged));
        forged=out;forged.version=2;
        assert(!p17_save_probe_report_shape_v1(&forged));
        forged=out;forged.struct_size=127;
        assert(!p17_save_probe_report_shape_v1(&forged));
        forged=out;forged.title_id^=1;
        assert(!p17_save_probe_report_shape_v1(&forged));
        forged=out;forged.active_slot^=1;
        assert(!p17_save_probe_report_shape_v1(&forged));
        forged=out;forged.slot0_counter=60;
        assert(!p17_save_probe_report_shape_v1(&forged));
        forged=out;forged.reserved1[0]=1;
        assert(!p17_save_probe_report_shape_v1(&forged));
    }
    std::cout<<"PASS Phase17 PKSM Issue15 installed-save probe: 3 fresh "
             <<"read-only snapshots, authenticated copied active H168 bytes, "
             <<"wrong-size/CMAC/header/counter/failure denials, fixed C ABI. "
             <<"NO GAME WRITES.\n";
}
