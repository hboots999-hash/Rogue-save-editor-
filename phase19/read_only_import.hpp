// Phase19 guarded copied-SD-only reader backed by pinned Phase18 parser.
// No installed title archives, save restoration, proposal calls or file writes.
#pragma once
#include "adapter.hpp"
#include "phase18/offline_engine.hpp"
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <cstdint>
#include <cerrno>

namespace phase19 {
constexpr const char* OfflineImportPath="sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
// Native adapter deliberately depends on the audited Phase18 low-level
// READ-ONLY decoder, not editor_model.hpp: the latter imports the unverified
// PC research parser which has a 37-KiB stack frame on ARM11.
// Host CI cross-checks this v1 adapter against ModelApiVersion=1 SaveView.
// Neither editor proposals nor PC access can be linked by this translation unit.
constexpr uint32_t Phase18ReadModelApiV1=1u;
struct Phase18ReadView final : ReadOnlyProvider {
    const uint8_t* data;
    const size_t length;
    const h168::Info info;
    Phase18ReadView(const uint8_t* p,size_t n,const h168::Info& valid)
        :data(p),length(n),info(valid){}
    Availability summary(unsigned& money,unsigned& counter) const override {
        money=info.money;counter=info.banks[info.active].counter;
        return Availability::Ready;
    }
    Availability partyCount(unsigned& size) const override {
        size=info.partyCount;return size?Availability::Ready:Availability::Empty;
    }
    Availability partyAt(unsigned index,Pokemon& p) const override {
        rogue18::Mon m{};
        if(!rogue18::partyAt(data,length,info,index,m))
            return Availability::Invalid;
        p=Pokemon{};
        p.species=m.species;
        p.level=0; // Phase18 does not expose verified level or nickname.
        std::snprintf(p.name,sizeof(p.name),"Species %u",p.species);
        for(unsigned i=0;i<6;i++){p.iv[i]=m.ivs[i];p.ev[i]=m.evs[i];}
        for(unsigned i=0;i<4;i++){p.moves[i]=m.moves[i];p.pp[i]=m.pp[i];}
        p.ability=m.abilitySlot;
        p.nature=m.effectiveNature;
        p.egg=m.egg;
        return Availability::Ready;
    }
    Availability bagCount(unsigned& size) const override {
        size=info.items;return size?Availability::Ready:Availability::Empty;
    }
    Availability itemAt(unsigned index,Item& item) const override {
        unsigned id=0,qty=0;
        if(!h168::item(data,info,index,id,qty))
            return Availability::Invalid;
        item=Item{};
        item.id=id;item.quantity=qty;
        std::snprintf(item.name,sizeof(item.name),"Item %u",id);
        return Availability::Ready;
    }
    Availability pcBoxCount(unsigned& n) const override {
        n=0;return Availability::Unsupported;
    }
    Availability pcBoxSize(unsigned,unsigned& n) const override {
        n=0;return Availability::Unsupported;
    }
    Availability pcAt(unsigned,unsigned,Pokemon&) const override {
        return Availability::Unsupported;
    }
};
// Processes a caller-provided exact 128-KiB *copy*. Never writes or persists it.
// On validation error, leaves the currently visible snapshot unchanged.
inline bool inspectCopiedBuffer(const uint8_t* raw,size_t n,
                                Demo& destination,std::string& reason) {
    if(!raw || n!=h168::Size) {
        reason="Exact 128 KiB unwrapped H168 copy required.";
        return false;
    }
    h168::Info info{};
    if(!rogue18::validate(raw,n,info)) {
        reason="Unsupported H168 copy, counter, checksum or Pokémon values.";
        return false;
    }
    Phase18ReadView view(raw,n,info);
    // Stage in static storage: invalid input never changes current UI.
    // Never allocate a ~64 KiB Demo inside the 3DS main-thread stack.
    static Demo checked{};
    captureInto(view,checked);
    // An empty party or bag is a legitimate, fully validated save state.
    // Unsupported/invalid data is still rejected fail-closed.
    if((checked.partyStatus!=Availability::Ready &&
        checked.partyStatus!=Availability::Empty) ||
       (checked.bagStatus!=Availability::Ready &&
        checked.bagStatus!=Availability::Empty) ||
       checked.moneyStatus!=Availability::Ready) {
        reason="Copy parsed, but UI could not safely display its fields.";
        return false;
    }
    checked.pcStatus=Availability::Unsupported;
    checked.backupStatus=Availability::Unverified;
    checked.synthetic=false; // No real-data edit proposals, ever.
    checked.sourceModelApi=Phase18ReadModelApiV1;
    checked.sourceSha256=rogue18::sourceDigest(raw,n);
    destination=checked;
    reason="H168 structure checked; ROM identity unverified. READ ONLY.";
    return true;
}

// No live GBA VC title or mounted save archive is touched.
// Only this user-imported file is opened for rb and then closed.
// Detect wrapped 263,168-byte container and wrong-size files explicitly.
// File stream is supplied by the caller and is never written or closed here.
// Host tests can exercise this exact byte-reader without a 3DS or private save.
// Caller opens the fixed SD import with "rb", and closes it on return.
inline bool inspectReadOnlyStream(std::FILE* stream,Demo& destination,
                                  std::string& reason) {
    if(!stream) {
        reason="No offline copy stream was provided.";
        return false;
    }
    if(std::fseek(stream,0,SEEK_END)!=0) {
        reason="Cannot determine the offline copy size.";
        return false;
    }
    const long n=std::ftell(stream);
    if(n<0) {
        reason="Offline copy size read failed.";
        return false;
    }
    if(n!=static_cast<long>(h168::Size)) {
        reason=n==263168L?
          "Wrapped 263168-byte VC container: unsupported here.":
          "Wrong file size: requires exact 131072-byte copy.";
        return false;
    }
    if(std::fseek(stream,0,SEEK_SET)!=0) {
        reason="Cannot rewind the offline copy.";
        return false;
    }
    auto bytes=std::unique_ptr<uint8_t[]>(new(std::nothrow) uint8_t[h168::Size]);
    if(!bytes) {
        reason="Not enough memory to inspect the offline copy.";
        return false;
    }
    if(std::fread(bytes.get(),1,h168::Size,stream)!=h168::Size ||
       std::ferror(stream)) {
        reason="Offline SD copy could not be read in full.";
        return false;
    }
    // Keep previous visible snapshot if the file mutates during inspection.
    // This extra-byte check detects growth beyond the exact expected payload.
    if(std::fgetc(stream)!=EOF || std::ferror(stream)) {
        reason="Offline SD copy changed or failed during inspection.";
        return false;
    }
    return inspectCopiedBuffer(bytes.get(),h168::Size,destination,reason);
}

inline bool inspectFixedSDCopy(Demo& destination,std::string& reason) {
    errno=0;
    std::unique_ptr<FILE, int(*)(FILE*)> file(std::fopen(OfflineImportPath,"rb"),
                                              &std::fclose);
    if(!file) {
        if(errno==ENOENT)
            reason="No SD import at fixed path. No game save was opened.";
        else
            reason="Cannot read the SD import (filesystem error).";
        return false;
    }
    return inspectReadOnlyStream(file.get(),destination,reason);
}
} // namespace phase19
