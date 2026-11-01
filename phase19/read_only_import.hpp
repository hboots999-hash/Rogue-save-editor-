// Phase19 guarded copied-SD-only reader backed by pinned Phase18 parser.
// No installed title archives, save restoration, proposal calls or file writes.
#pragma once
#include "adapter.hpp"
#include "phase18/offline_engine.hpp"
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <cstdint>

namespace phase19 {
constexpr const char* OfflineImportPath="sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
struct Phase18ReadView final : ReadOnlyProvider {
    const uint8_t* data;
    const size_t length;
    const h168::Info info;
    Phase18ReadView(const uint8_t* p,size_t n,const h168::Info& valid)
       :data(p),length(n),info(valid){}
    Availability summary(unsigned& money,unsigned& counter) const override {
        money=info.money;
        counter=info.banks[info.active].counter;
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
        p.level=0; // Phase18 Mon does not expose verified level or nickname.
        std::snprintf(p.name,sizeof(p.name),"Species %u",p.species);
        for(unsigned i=0;i<6;i++){
            p.iv[i]=m.ivs[i];p.ev[i]=m.evs[i];
        }
        for(unsigned i=0;i<4;i++) {
            p.moves[i]=m.moves[i];p.pp[i]=m.pp[i];
        }
        p.ability=m.abilitySlot; // Phase18 EX v2.1 Misc+8 bits 29-30, NOT Gmax bit.
        p.nature=m.effectiveNature; // already range-validated in Phase18.
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
        // No independently verified Emerald Rogue 2.1.2 item-ID name database.
        std::snprintf(item.name,sizeof(item.name),"Item %u",id);
        return Availability::Ready;
    }
    Availability pcBoxCount(unsigned& n) const override {
        n=0;return Availability::Unsupported; // no verified PC parser in PR #8.
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
        reason="Unsupported H168 copy or checksum/counter failure.";
        return false;
    }
    Phase18ReadView view(raw,n,info);
    // Fixed-size DTO is constructed into *static* caller-provided memory in
    // native app; no 60-KiB UI object on 3DS main thread stack.
    captureInto(view,destination);
    if(destination.partyStatus==Availability::Invalid ||
       destination.partyStatus==Availability::Unsupported ||
       destination.bagStatus==Availability::Invalid ||
       destination.bagStatus==Availability::Unsupported ||
       destination.moneyStatus!=Availability::Ready) {
        // Previously validated raw may still exceed UI boundaries. Clear
        // partially parsed view, never silently show mock as if it were real.
        std::fill_n(reinterpret_cast<uint8_t*>(&destination),sizeof(destination),uint8_t(0));
        destination.partyStatus=Availability::Invalid;
        destination.bagStatus=Availability::Invalid;
        destination.moneyStatus=Availability::Invalid;
        destination.pcStatus=Availability::Unsupported;
        reason="Copy parsed, but UI could not safely display its fields.";
        return false;
    }
    destination.pcStatus=Availability::Unsupported;
    destination.backupStatus=Availability::Unverified;
    destination.synthetic=false; // No real-data edit proposals, ever.
    reason="VALID COPY: party/bag/money READ ONLY. PC unsupported.";
    return true;
}

// No live GBA VC title or mounted save archive is touched.
// Only this user-imported file is opened for rb and then closed.
// Detect wrapped 263,168-byte container and wrong-size files explicitly.
inline bool inspectFixedSDCopy(Demo& destination,std::string& reason) {
    std::unique_ptr<FILE, int(*)(FILE*)> file(std::fopen(OfflineImportPath,"rb"),
                                              &std::fclose);
    if(!file) {
        reason="No offline SD import found. No game save was opened.";
        return false;
    }
    if(std::fseek(file.get(),0,SEEK_END)!=0){
        reason="Cannot read size of offline SD import.";
        return false;
    }
    const long n=std::ftell(file.get());
    if(n!=long(h168::Size)) {
        reason=n==263168?
          "Wrapped 263168-byte VC container: unsupported here.":
          "Wrong file size: requires exact 131072-byte copy.";
        return false;
    }
    if(std::fseek(file.get(),0,SEEK_SET)!=0) {
        reason="Cannot rewind offline SD import.";
        return false;
    }
    auto bytes=std::unique_ptr<uint8_t[]>(new(std::nothrow) uint8_t[h168::Size]);
    if(!bytes){
        reason="Not enough memory to inspect the offline copy.";
        return false;
    }
    if(std::fread(bytes.get(),1,h168::Size,file.get())!=h168::Size){
        reason="Offline SD copy could not be read in full.";
        return false;
    }
    file.reset();
    return inspectCopiedBuffer(bytes.get(),h168::Size,destination,reason);
}
} // namespace phase19
