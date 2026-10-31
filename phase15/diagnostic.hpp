// SPDX-License-Identifier: GPL-3.0-or-later
// Phase 15 H168 diagnostic: complete-container READ and console MAC verification.
// Never writes installed game archives. This is NOT restore authorisation.
#pragma once
#include "authenticated_restore.hpp"
#include <cstring>

namespace phase15 {
enum class Code : unsigned {
    NotRun, FirstReadFailed, SecondReadFailed, SnapshotChanged,
    ContainerSizeInvalid, HeaderInvalid, ForeignTitle, CidMismatch,
    CounterInvalid, RawInvalid, CmacServiceFailed, CmacMismatch, Pass
};
inline const char* label(Code c){
    switch(c){
        case Code::NotRun:return "NOT RUN";
        case Code::FirstReadFailed:return "FIRST LIVE READ FAILED";
        case Code::SecondReadFailed:return "SECOND LIVE READ FAILED";
        case Code::SnapshotChanged:return "LIVE DATA CHANGED";
        case Code::ContainerSizeInvalid:return "CONTAINER SIZE INVALID";
        case Code::HeaderInvalid:return "SLOT HEADER INVALID";
        case Code::ForeignTitle:return "OTHER GAME TITLE";
        case Code::CidMismatch:return "SD CID MISMATCH";
        case Code::CounterInvalid:return "SLOT COUNTERS UNSUPPORTED";
        case Code::RawInvalid:return "RAW SAVE CHECKSUM BAD";
        case Code::CmacServiceFailed:return "CMAC SERVICE FAILED";
        case Code::CmacMismatch:return "CMAC DOES NOT MATCH";
        case Code::Pass:return "BOTH SLOTS VERIFIED";
    }
    return "UNKNOWN";
}
struct SlotResult {
    bool header=false,raw=false,cmac=false;
    uint32_t counter=0;
};
struct Report {
    Code code=Code::NotRun;
    size_t containerBytes=0;
    SlotResult slots[2]{};
    bool stable=false;
    size_t activeSlot=0;
    uint32_t latestCounter=0;
};
// Device contract: read(Bytes&) obtains an independent, fresh, RO handle;
// calculate(Digest,Mac) uses the bound console MAC service on that handle.
// A fake signer in host tests does NOT prove 3DS CMAC correctness.
template<class Device> Report run(Device& device) {
    Report r{};
    phase14::Bytes first,second;
    if(!device.read(first)){r.code=Code::FirstReadFailed;return r;}
    if(!device.read(second)){r.code=Code::SecondReadFailed;return r;}
    r.containerBytes=second.size();
    if(first!=second){r.code=Code::SnapshotChanged;return r;}
    r.stable=true;
    if(second.size()<rogue_live::ContainerMin ||
       second.size()>phase14::MaxContainer) {
        r.code=Code::ContainerSizeInvalid;return r;
    }
    const size_t offs[2]={0,rogue_live::SlotSize};
    for(unsigned i=0;i<2;i++){
        const auto slot=rogue_live::parseHeader(
            second.data()+offs[i],rogue_live::HeaderSize,
            static_cast<uint32_t>(offs[i]));
        r.slots[i].header=slot.valid;
        r.slots[i].counter=slot.timesSaved;
        if(!slot.valid){r.code=Code::HeaderInvalid;return r;}
        if(slot.title!=rogue_live::ExpectedTitle){
            r.code=Code::ForeignTitle;return r;
        }
    }
    if(std::memcmp(second.data()+0x40,
                   second.data()+rogue_live::SlotSize+0x40,16)!=0){
        r.code=Code::CidMismatch;return r;
    }
    phase14::Layout layout{};
    if(!phase14::layout(second,layout)){
        r.code=Code::CounterInvalid;return r;
    }
    r.activeSlot=layout.active==0?0:1;
    r.latestCounter=layout.counter;
    for(unsigned i=0;i<2;i++){
        h168::Info parsed{};
        r.slots[i].raw=h168::inspect(
            second.data()+offs[i]+rogue_live::HeaderSize,
            rogue_live::RawSize,parsed);
        if(!r.slots[i].raw){
            r.code=Code::RawInvalid;return r;
        }
        phase14::Mac actual{};
        if(!phase14::stableMac(
              device,phase14::slotHash(second,offs[i]),actual)){
            r.code=Code::CmacServiceFailed;return r;
        }
        r.slots[i].cmac=std::memcmp(
            actual.data(),second.data()+offs[i]+0x10,actual.size())==0;
        if(!r.slots[i].cmac){
            r.code=Code::CmacMismatch;return r;
        }
    }
    r.code=Code::Pass;
    return r;
}
} // namespace phase15
