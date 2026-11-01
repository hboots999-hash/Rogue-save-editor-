// Phase19 guarded copied-SD-only reader backed by pinned Phase18 parser.
// No installed title archives, save restoration, proposal calls or file writes.
#pragma once
#include "adapter.hpp"
#include "phase18/editor_model.hpp"
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <cstdint>
#include <cerrno>

namespace phase19 {
constexpr const char* OfflineImportPath="sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
// Only the versioned Phase18 read-only SaveView is admitted to this UI.
// No Phase18 Request, propose, export, restoration or write API is called.
static_assert(rogue18::ModelApiVersion==1u,
              "Phase19 requires a reviewed Phase18 SaveView v1; audit before repinning");
struct Phase18ReadView final : ReadOnlyProvider {
    const rogue18::SaveView& view;
    explicit Phase18ReadView(const rogue18::SaveView& v):view(v){}
    Availability summary(unsigned& money,unsigned& counter) const override {
        money=view.money;
        counter=view.activeCounter;
        return Availability::Ready;
    }
    Availability partyCount(unsigned& size) const override {
        size=static_cast<unsigned>(view.party.size());
        return size?Availability::Ready:Availability::Empty;
    }
    Availability partyAt(unsigned index,Pokemon& p) const override {
        if(index>=view.party.size() || view.party[index].slot!=index)
            return Availability::Invalid;
        const rogue18::Mon& m=view.party[index].value;
        p=Pokemon{};
        p.species=m.species;
        p.level=0; // Not verified by Phase18 numeric read-only contract.
        std::snprintf(p.name,sizeof(p.name),"Species %u",p.species);
        for(unsigned i=0;i<6;i++){
            p.iv[i]=m.ivs[i];p.ev[i]=m.evs[i];
        }
        for(unsigned i=0;i<4;i++){
            p.moves[i]=m.moves[i];p.pp[i]=m.pp[i];
        }
        p.ability=m.abilitySlot; // 0,1,2; explicitly NOT the Gmax flag
        p.nature=m.effectiveNature;
        p.egg=m.egg;
        return Availability::Ready;
    }
    Availability bagCount(unsigned& size) const override {
        size=static_cast<unsigned>(view.items.size());
        return size?Availability::Ready:Availability::Empty;
    }
    Availability itemAt(unsigned index,Item& item) const override {
        if(index>=view.items.size() || view.items[index].slot!=index)
            return Availability::Invalid;
        const auto& i=view.items[index];
        item=Item{};
        item.id=i.itemId;item.quantity=i.quantity;
        std::snprintf(item.name,sizeof(item.name),"Item %u",item.id);
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
    rogue18::SaveView model{};
    try {
        model=rogue18::inspectReadOnly(raw,n,false); // No PC candidate inference.
    }catch(const std::bad_alloc&) {
        reason="Not enough memory to validate copied save.";
        return false;
    }
    if(!model.valid || model.apiVersion!=rogue18::ModelApiVersion ||
       model.confidence!=rogue18::DataConfidence::StructurallyValidated ||
       !rogue18::hasDigest(model.sourceSha256) ||
       !model.capabilities.copiedSaveOnly ||
       model.capabilities.liveWrite || model.capabilities.restore ||
       model.capabilities.pcEdit) {
        reason="Unsupported H168 copy, counter, checksum or model version.";
        return false;
    }
    if(model.party.size()>6u || model.items.size()>450u ||
       model.money>999999u) {
        reason="Validated copy exceeds read-only viewer bounds.";
        return false;
    }
    Phase18ReadView view(model);
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
    checked.sourceModelApi=model.apiVersion;
    checked.sourceSha256=model.sourceSha256;
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
