// Phase 9.2: synthetic ONLY offline draft exporter.
// Exact byte-for-byte toy fixture allowlist, not a ROM compatibility detector.
// Never opens installed-game archives, extdata or performs restore.
#pragma once
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <new>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include "rogue_bridge.h"
#include "rogue_numeric_edit.h"
#include "rogue_pokemon_edit.h"
#include "rogue_moves_items_edit.h"
#include "edit_values.hpp"

namespace rogue_sandbox {
constexpr size_t Size = ROGUE_RAW_SAVE_SIZE;
struct Result {
    bool ok = false;
    std::string message;
    size_t changed = 0;
};

inline void w16(uint8_t* p,uint16_t v) { p[0]=uint8_t(v);p[1]=uint8_t(v>>8); }
inline void w32(uint8_t* p,uint32_t v) { w16(p,uint16_t(v));w16(p+2,uint16_t(v>>16)); }
inline uint16_t r16(const uint8_t* p) { return uint16_t(p[0]|uint16_t(p[1])<<8); }
inline uint32_t r32(const uint8_t* p) { return uint32_t(r16(p))|(uint32_t(r16(p+2))<<16); }

// Reproduce the wholly synthetic Phase 7 test fixture exactly. The full
// 131072-byte equality check prevents a real/candidate save from being used
// for writes merely because it shares a profile marker or has valid checksums.
inline std::unique_ptr<uint8_t[]> syntheticBytes() {
    std::unique_ptr<uint8_t[]> b(new(std::nothrow) uint8_t[Size]());
    if (!b) return {};
    const uint32_t key=0x1234abcd;
    w32(b.get()+0x4c,key);
    w32(b.get()+4096+0x4a8,40000u^key);
    w32(b.get()+4096+0x234,1);
    uint8_t mon[104]={},plain[48]={};
    const uint32_t pid=2400,ot=0x44556677;
    w32(mon,pid);w32(mon+4,ot);mon[0x13]=2;
    w16(plain,25);w16(plain+12,33);
    plain[20]=20;
    const uint8_t evs[]={4,252,0,252,0,0};
    std::memcpy(plain+24,evs,sizeof(evs));
    uint32_t ivs=0;
    for (unsigned i=0;i<6;i++) ivs|=uint32_t(10)<<(5*i);
    w32(plain+40,ivs);
    uint32_t monChecksum=0;
    for(unsigned i=0;i<48;i+=2)monChecksum+=r16(plain+i);
    w16(mon+28,uint16_t(monChecksum));
    for(unsigned i=0;i<48;i+=4)w32(mon+32+i,r32(plain+i)^(pid^ot));
    std::memcpy(b.get()+4096+0x238,mon,sizeof(mon));
    const uint16_t ids[]={113,1,5}, qty[]={10,5,5};
    for(unsigned i=0;i<3;i++){
        w16(b.get()+4096+0x578+4*i,ids[i]);
        w16(b.get()+4096+0x578+4*i+2,uint16_t(qty[i]^(key&0xffff)));
    }
    const uint8_t anchor[]={6,0,2,0xdf,0x6b,1};
    std::memcpy(b.get()+40960+0xe00,anchor,sizeof(anchor));
    for(unsigned i=0;i<14;i++){
        uint8_t* sector=b.get()+i*4096;
        uint32_t total=0;
        for(unsigned pos=0;pos<0xff4;pos+=4)total+=r32(sector+pos);
        w16(sector+0xff4,uint16_t(i));
        w16(sector+0xff6,uint16_t((total>>16)+total));
        w32(sector+0xff8,0x08012025);
        w32(sector+0xffc,0);
    }
    return b;
}

inline bool exactSynthetic(const uint8_t* source,size_t n) {
    if(!source || n!=Size)return false;
    auto expected=syntheticBytes();
    return expected && std::memcmp(source,expected.get(),Size)==0;
}

inline std::unique_ptr<uint8_t[]> readExact(const char* path) {
    if(!path)return {};
    struct FileClose {void operator()(FILE* f)const {if(f)std::fclose(f);}};
    std::unique_ptr<FILE,FileClose> f(std::fopen(path,"rb"));
    if(!f)return {};
    std::unique_ptr<uint8_t[]> b(new(std::nothrow) uint8_t[Size]);
    if(!b || std::fread(b.get(),1,Size,f.get())!=Size ||
       std::fgetc(f.get())!=EOF || std::ferror(f.get()))return {};
    return b;
}

inline bool directory(const char* path) {
    if(::mkdir(path,0777)!=0 && errno!=EEXIST)return false;
    struct stat st{};
    return ::stat(path,&st)==0 && S_ISDIR(st.st_mode);
}

// Returns false on ANY write, close or readback failure. Failed partial
// files are left at their unique paths for inspection; never overwritten.
inline bool writeVerifiedUnique(const std::string& dir,const char* prefix,
                                const uint8_t* data,std::string& filename) {
    if(!data || !prefix)return false;
    for(unsigned i=1;i<=99;i++){
        char base[40];
        std::snprintf(base,sizeof(base),"%s%03u.sav",prefix,i);
        const std::string full=dir+"/"+base;
        int fd=::open(full.c_str(),O_WRONLY|O_CREAT|O_EXCL,0666);
        if(fd<0){if(errno==EEXIST)continue;return false;}
        size_t count=0;bool good=true;
        while(count<Size){
            ssize_t n=::write(fd,data+count,Size-count);
            if(n<=0){good=false;break;}
            count+=size_t(n);
        }
        if(::close(fd)!=0)good=false;
        if(!good)return false;
        auto checked=readExact(full.c_str());
        if(!checked || std::memcmp(checked.get(),data,Size)!=0)return false;
        filename=base;
        return true;
    }
    return false;
}

inline Result exportOne(const uint8_t* expectedOriginal,const char* importPath,
                        const char* root,Edit kind,unsigned index=0,const Values& values=Values{}) {
    if(!expectedOriginal || !importPath || !root)
        return {false,"Invalid export arguments",0};
    // Hard safety gate: no draft may be written for ANY ordinary game save.
    if(!exactSynthetic(expectedOriginal,Size))
        return {false,"ROM unverified: real saves READ ONLY",0};
    if(!valid(kind,values))
        return {false,"Invalid proposed values: BLOCKED",0};
    auto fresh=readExact(importPath);
    if(!fresh || std::memcmp(fresh.get(),expectedOriginal,Size)!=0)
        return {false,"SD import changed/missing: BLOCKED",0};
    std::unique_ptr<uint8_t[]> draft(new(std::nothrow) uint8_t[Size]);
    if(!draft)return {false,"Out of memory: BLOCKED",0};

    rogue_preview before{};
    if(!rogue_preview_copy(fresh.get(),Size,&before))
        return {false,"Invalid candidate: BLOCKED",0};
    // Use the Phase 7 vetted buffer-only routines. Never write a file
    // unless a fresh parsed snapshot also confirms the requested change.
    size_t changed=0;
    if(kind==Edit::Money || kind==Edit::ExistingItem){
        rogue_numeric_request request{};
        request.expected_money=before.money;
        if(kind==Edit::Money){
            request.kind=ROGUE_EDIT_MONEY;
            request.new_money=values.money;
        }else{
            rogue_item_candidate item{};
            std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
                rogue_snapshot_open(fresh.get(),Size),rogue_snapshot_close);
            if(!snap || !rogue_snapshot_item(snap.get(),index,&item))
                return {false,"Item selection invalid: BLOCKED",0};
            request.kind=ROGUE_EDIT_EXISTING_QUANTITY;
            request.slot_index=item.slot_index;
            request.expected_item_id=item.item_id;
            request.new_quantity=values.quantity;
        }
        if(rogue_make_numeric_draft(fresh.get(),Size,&request,draft.get(),Size,&changed)
            !=ROGUE_NUMERIC_OK)
            return {false,"Numeric editor rejected draft: BLOCKED",0};
    }else if(kind==Edit::AllIV || kind==Edit::OffensiveEV || kind==Edit::FirstMove){
        // The allowlisted test fixture has EXACTLY one toy Pokemon.
        // Do not extend this selection rule to a real save/profile.
        if(index!=0 || before.party_count!=1)
            return {false,"Toy Pokemon selection invalid: BLOCKED",0};
        rogue_party_candidate initial{};
        std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
            rogue_snapshot_open(fresh.get(),Size),rogue_snapshot_close);
        if(!snap || !rogue_snapshot_party(snap.get(),index,&initial)
           || initial.species_id!=25 || initial.moves[0]!=33)
            return {false,"Toy Pokemon has unexpected data: BLOCKED",0};
        if(kind==Edit::FirstMove){
            rogue_move_request req{};
            req.party_index=0;
            req.move_index=values.moveSlot;
            req.expected_species=initial.species_id;
            req.expected_personality=2400; // Exact fixture: not a guessed ROM PID
            req.expected_move=initial.moves[values.moveSlot];
            req.replacement_move=values.moveId;  // User-selected test-only ID
            req.replacement_pp=values.movePP;
            if(rogue_make_move_draft(fresh.get(),Size,&req,draft.get(),Size,&changed)
               !=ROGUE_EXT_OK)
                return {false,"Move editor rejected draft: BLOCKED",0};
        }else{
            rogue_pokemon_stats_request req{};
            req.party_index=0;
            req.expected_species=initial.species_id;
            req.expected_personality=2400; // Exact toy fixture PID
            if(kind==Edit::AllIV){
                req.change_ivs=1;
                std::memcpy(req.ivs,values.ivs,sizeof(req.ivs));
            }else{
                req.change_evs=1;
                // HP, Atk, Def, Speed, SpAtk, SpDef: 508 valid total
                std::memcpy(req.evs,values.evs,sizeof(req.evs));
            }
            if(rogue_make_pokemon_stats_draft(fresh.get(),Size,&req,draft.get(),Size,&changed)
               !=ROGUE_MON_EDIT_OK)
                return {false,"Pokemon stats editor rejected draft: BLOCKED",0};
        }
    }else return {false,"Unknown edit operation: BLOCKED",0};

    if(changed==0 || changed>Size)
        return {false,"Draft changed-byte count invalid: BLOCKED",0};
    rogue_preview after{};
    if(!rogue_preview_copy(draft.get(),Size,&after))
        return {false,"Draft parser reload failed",0};
    if(after.profile!=before.profile || after.party_count!=before.party_count ||
       after.occupied_bag_slots!=before.occupied_bag_slots)
        return {false,"Draft structural changes: BLOCKED",0};
    if(kind==Edit::Money && after.money!=values.money)
        return {false,"Money readback mismatch",0};
    if(kind==Edit::ExistingItem){
        rogue_item_candidate item{};
        std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
            rogue_snapshot_open(draft.get(),Size),rogue_snapshot_close);
        if(!snap || !rogue_snapshot_item(snap.get(),index,&item) || item.quantity!=values.quantity)
            return {false,"Item readback mismatch",0};
    }
    if(kind==Edit::AllIV || kind==Edit::OffensiveEV || kind==Edit::FirstMove){
        rogue_party_candidate mon{};
        std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
            rogue_snapshot_open(draft.get(),Size),rogue_snapshot_close);
        if(!snap || !rogue_snapshot_party(snap.get(),index,&mon) || mon.species_id!=25)
            return {false,"Pokemon draft cannot be decoded: BLOCKED",0};
        if(kind==Edit::AllIV){
            if(std::memcmp(mon.ivs,values.ivs,sizeof(values.ivs))!=0)
                return {false,"IV readback mismatch: BLOCKED",0};
        }else if(kind==Edit::OffensiveEV){
            if(std::memcmp(mon.evs,values.evs,sizeof(values.evs))!=0)
                return {false,"EV readback mismatch: BLOCKED",0};
        }else if(mon.moves[values.moveSlot]!=values.moveId ||
                 mon.move_pp[values.moveSlot]!=values.movePP)
            return {false,"Move readback mismatch: BLOCKED",0};
    }
    // Check input A SECOND time immediately before creating an untouched backup.
    fresh=readExact(importPath);
    if(!fresh || std::memcmp(fresh.get(),expectedOriginal,Size)!=0)
        return {false,"SD import changed before backup: BLOCKED",0};
    const std::string rootDir(root);
    if(!directory(rootDir.c_str()))
        return {false,"App folder not usable: BLOCKED",0};
    const std::string backupDir=rootDir+"/backups";
    if(!directory(backupDir.c_str()))
        return {false,"Backup directory unavailable: BLOCKED",0};
    std::string backup;
    if(!writeVerifiedUnique(backupDir,"import_",expectedOriginal,backup))
        return {false,"Untouched backup FAILED: draft BLOCKED",0};
    // Never export even a synthetic draft if input changes after backup.
    fresh=readExact(importPath);
    if(!fresh || std::memcmp(fresh.get(),expectedOriginal,Size)!=0)
        return {false,"SD import changed after backup: BLOCKED",0};
    const std::string draftDir=rootDir+"/drafts";
    if(!directory(draftDir.c_str()))
        return {false,"Draft directory unavailable: BLOCKED",0};
    std::string output;
    if(!writeVerifiedUnique(draftDir,"rogue_",draft.get(),output))
        return {false,"Draft export/readback FAILED",0};
    return {true,"SYNTHETIC DRAFT: "+output,changed};
}
} // namespace rogue_sandbox
