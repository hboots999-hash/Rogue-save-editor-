// SPDX-License-Identifier: GPL-3.0-or-later
// Phase 15: OPTIONAL SD-only 256-byte scratch probe, unique name, readback.
// Not a recovery journal, never touches the GBA VC archive or private saves.
#pragma once
#include <3ds.h>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include "offline.hpp"
namespace phase15 {
struct SdReceipt {
    bool ok=false;
    std::string message;
};
inline std::string sdFsError(const char* where,::Result result){
    char s[90];std::snprintf(s,sizeof(s),"%s 0x%08lX",where,
        static_cast<unsigned long>(static_cast<uint32_t>(result)));
    return s;
}
inline SdReceipt scratchProbe(){
    const char* const root="sdmc:/3ds/PKSM-Rogue";
    if(!h168::directory(root))return {false,"SD root unavailable"};
    const char* const folder="sdmc:/3ds/PKSM-Rogue/phase15";
    if(!h168::directory(folder))return {false,"SD probe folder unavailable"};
    std::array<uint8_t,256> payload{};
    constexpr char marker[]="PKSM-ROGUE-PHASE15-SD-PROBE-ONLY";
    std::memcpy(payload.data(),marker,sizeof(marker));
    for(size_t i=sizeof(marker);i<payload.size();i++)
        payload[i]=uint8_t((i*73u+17u)&0xffu);
    for(unsigned number=1;number<=99;number++){
        char name[40];std::snprintf(name,sizeof(name),"probe_%03u.bin",number);
        const std::string full=std::string(folder)+"/"+name;
        struct stat statInfo{};
        errno=0;
        if(::stat(full.c_str(),&statInfo)==0)continue;
        if(errno!=ENOENT)return {false,"SD probe path stat failure"};
        const std::string absolute=full.substr(std::strlen("sdmc:"));
        FS_Archive sd{};
        ::Result rc=FSUSER_OpenArchive(&sd,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""));
        if(R_FAILED(rc))return {false,sdFsError("SD OpenArchive",rc)};
        const FS_Path fp=fsMakePath(PATH_ASCII,absolute.c_str());
        // Native create is exclusive: never truncate, delete or reuse names.
        rc=FSUSER_CreateFile(sd,fp,0,payload.size());
        if(R_FAILED(rc)){
            (void)FSUSER_CloseArchive(sd);
            return {false,sdFsError("SD CreateFile",rc)};
        }
        Handle fd=0;
        rc=FSUSER_OpenFile(&fd,sd,fp,FS_OPEN_WRITE,0);
        if(R_FAILED(rc)){
            (void)FSUSER_CloseArchive(sd);
            return {false,sdFsError("SD OpenFile",rc)};
        }
        u32 written=0;
        rc=FSFILE_Write(fd,&written,0,payload.data(),payload.size(),FS_WRITE_FLUSH);
        const ::Result closeRc=FSFILE_Close(fd);
        const ::Result archiveRc=FSUSER_CloseArchive(sd);
        if(R_FAILED(rc)||written!=payload.size())
            return {false,sdFsError("SD write/flush",rc)};
        if(R_FAILED(closeRc))return {false,sdFsError("SD close",closeRc)};
        if(R_FAILED(archiveRc))return {false,sdFsError("SD close archive",archiveRc)};
        // New independent handles: full exact-length and byte-for-byte readback.
        FS_Archive checkArchive{};
        rc=FSUSER_OpenArchive(&checkArchive,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""));
        if(R_FAILED(rc))return {false,sdFsError("Readback archive",rc)};
        Handle checkFd=0;
        rc=FSUSER_OpenFile(&checkFd,checkArchive,fp,FS_OPEN_READ,0);
        if(R_FAILED(rc)){
            (void)FSUSER_CloseArchive(checkArchive);
            return {false,sdFsError("Readback open",rc)};
        }
        u64 size=0;u32 got=0;
        std::array<uint8_t,256> back{};
        const bool ok=R_SUCCEEDED(FSFILE_GetSize(checkFd,&size)) &&
            size==payload.size() &&
            R_SUCCEEDED(FSFILE_Read(checkFd,&got,0,back.data(),back.size())) &&
            got==payload.size() && back==payload;
        const bool closed=R_SUCCEEDED(FSFILE_Close(checkFd));
        const bool archived=R_SUCCEEDED(FSUSER_CloseArchive(checkArchive));
        if(!ok||!closed||!archived)
            return {false,"SD scratch readback mismatch"};
        return {true,std::string("SD SCRATCH VERIFIED: ")+name};
    }
    return {false,"All 99 probe filenames used"};
}
} // namespace phase15
