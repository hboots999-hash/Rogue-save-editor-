/* Phase 11: read-only PxiFS0 access to installed Emerald Rogue's GBA VC
 * NAND container. No FSPXI write commands and no live restore.
 * References: GPLv3 Checkpoint 5.3.0 GbaSave and 3ds source/fsstream.cpp;
 * also source 3dbrew.org/wiki/3DS_Virtual_Console#NAND_Savegame.
 */
#pragma once
#include <3ds.h>
#include <cstdint>
#include <memory>
#include <new>
#include <string>
#include "live_header.hpp"
#include "offline.hpp"

extern "C" ::Result rogueSvcControlService(int op,...); // SVC 0xB0, Luma-only

namespace rogue_live {
struct Outcome {
    bool ok=false;
    std::string stage;
    std::unique_ptr<uint8_t[]> save;
    h168::Info decoded{};
    uint32_t timesSaved=0;
};
inline std::string hexError(const char* stage,::Result value){
    char buf[92];
    std::snprintf(buf,sizeof(buf),"%s FS=0x%08lX",stage,
        static_cast<unsigned long>(static_cast<uint32_t>(value)));
    return std::string(buf);
}
inline Outcome loadInstalledReadOnly(){
    Outcome ret{};
    Handle service=0;
    ::Result rc=rogueSvcControlService(0,&service,"PxiFS0");
    if(R_FAILED(rc)||!service){
        ret.stage=hexError("PxiFS0 unavailable",rc);return ret;
    }
    const uint32_t archivePath[4]={
        static_cast<uint32_t>(ExpectedTitle),
        static_cast<uint32_t>(ExpectedTitle>>32),
        static_cast<uint32_t>(MEDIATYPE_SD),1u
    };
    FSPXI_Archive archive{};
    rc=FSPXI_OpenArchive(service,&archive,ARCHIVE_SAVEDATA_AND_CONTENT,
                         {PATH_BINARY,sizeof(archivePath),archivePath});
    if(R_FAILED(rc)){
        ret.stage=hexError("Open GBA archive",rc);
        svcCloseHandle(service);return ret;
    }
    const uint32_t gbaFilePath[5]={1,1,3,0,0};
    FSPXI_File file{};
    rc=FSPXI_OpenFile(service,&file,archive,
                      {PATH_BINARY,sizeof(gbaFilePath),gbaFilePath},FS_OPEN_READ,0);
    if(R_FAILED(rc)){
        ret.stage=hexError("Open GBA file RO",rc);
        FSPXI_CloseArchive(service,archive);
        svcCloseHandle(service);return ret;
    }
    uint8_t topData[HeaderSize]={},bottomData[HeaderSize]={};
    uint32_t got=0;
    Slot top{},bottom{},chosen{};
    rc=FSPXI_ReadFile(service,file,&got,0,topData,HeaderSize);
    if(R_SUCCEEDED(rc)&&got==HeaderSize)top=parseHeader(topData,got,0);
    // For the target GBA VC 128KiB, a second slot begins at +0x20200.
    // Never assume another game with a different configured save size matches.
    if(R_SUCCEEDED(rc)){
        got=0;
        rc=FSPXI_ReadFile(service,file,&got,SlotSize,bottomData,HeaderSize);
        if(R_SUCCEEDED(rc)&&got==HeaderSize)
            bottom=parseHeader(bottomData,got,SlotSize);
    }
    if(R_FAILED(rc)){
        ret.stage=hexError("Read GBA headers",rc);
    }else if(!selectLatest(top,bottom,chosen)){
        ret.stage="Not active H168 title or invalid slot";
    }else{
        auto b=std::unique_ptr<uint8_t[]>(new(std::nothrow) uint8_t[RawSize]);
        if(!b)ret.stage="Out of memory for GBA save";
        else{
            got=0;
            rc=FSPXI_ReadFile(service,file,&got,
                              chosen.offset+HeaderSize,b.get(),RawSize);
            if(R_FAILED(rc)||got!=RawSize)
                ret.stage=hexError("Read GBA raw save",rc);
            else if(!h168::inspect(b.get(),RawSize,ret.decoded))
                ret.stage="Live save failed dual-bank validation";
            else{
                ret.ok=true;
                ret.stage="LIVE READ OK (not modified)";
                ret.timesSaved=chosen.timesSaved;
                ret.save=std::move(b);
            }
        }
    }
    // No writes, and always close all privileged handles before returning.
    FSPXI_CloseFile(service,file);
    FSPXI_CloseArchive(service,archive);
    svcCloseHandle(service);
    return ret;
}
} // namespace rogue_live
