// SPDX-License-Identifier: GPL-3.0-or-later
// Native read/authentication adapter. No UI calls this; WRITE gate stays shut.
#pragma once
#include <3ds.h>
#include "authenticated_restore.hpp"
extern "C" ::Result rogueSvcControlService(int op,...);
namespace phase14 {
class NativePxi {
    Handle service=0;
    FSPXI_Archive archive{};
    FSPXI_File file{};
    bool archiveOpen=false,fileOpen=false;
public:
    ::Result last=0;
    NativePxi()=default;
    NativePxi(const NativePxi&)=delete;
    NativePxi& operator=(const NativePxi&)=delete;
    ~NativePxi(){close();}
    bool close() {
        bool ok=true;
        if(fileOpen) {last=FSPXI_CloseFile(service,file);ok=R_SUCCEEDED(last);fileOpen=false;}
        if(archiveOpen) {last=FSPXI_CloseArchive(service,archive);ok=R_SUCCEEDED(last)&&ok;archiveOpen=false;}
        if(service) {last=svcCloseHandle(service);ok=R_SUCCEEDED(last)&&ok;service=0;}
        return ok;
    }
    bool open() {
        if(service || archiveOpen || fileOpen)return false;
        last=rogueSvcControlService(0,&service,"PxiFS0");
        if(R_FAILED(last)||!service){close();return false;}
        const u32 path[]={u32(rogue_live::ExpectedTitle),u32(rogue_live::ExpectedTitle>>32),MEDIATYPE_SD,1};
        last=FSPXI_OpenArchive(service,&archive,ARCHIVE_SAVEDATA_AND_CONTENT,
                              {PATH_BINARY,sizeof(path),path});
        if(R_FAILED(last)){close();return false;} archiveOpen=true;
        const u32 fp[]={1,1,3,0,0};
        // No writable file handle is obtainable in this phase.
        last=FSPXI_OpenFile(service,&file,archive,{PATH_BINARY,sizeof(fp),fp},FS_OPEN_READ,0);
        if(R_FAILED(last)){close();return false;} fileOpen=true;return true;
    }
    bool read(Bytes& out) {
        out.clear();
        // A new file/archive session for each snapshot and final readback.
        if(!close() || !open())return false;
        u64 n=0;last=FSPXI_GetFileSize(service,file,&n);
        if(R_FAILED(last)||n<rogue_live::ContainerMin||n>MaxContainer)return false;
        Bytes b(static_cast<size_t>(n));u32 got=0;
        last=FSPXI_ReadFile(service,file,&got,0,b.data(),u32(n));
        if(R_FAILED(last)||got!=n)return false;
        out=std::move(b);return true;
    }
    bool calculate(const Digest& digest,Mac& mac) {
        if(!fileOpen)return false;
        mac.fill(0);
        last=FSPXI_CalcSavegameMAC(service,file,digest.data(),digest.size(),mac.data(),mac.size());
        return R_SUCCEEDED(last);
    }
    bool writesAllowed() const {return LiveWritesEnabled;}
    bool write(size_t offset,const uint8_t* data,size_t size) {
        // Source for the eventual checked native writer, permanently unreachable
        // until a separately reviewed release-gate change AND RW-open change.
        if(!LiveWritesEnabled || !fileOpen || !data || offset>MaxContainer ||
           size>MaxContainer-offset)return false;
        u32 written=0;
        last=FSPXI_WriteFile(service,file,&written,offset,data,u32(size),FS_WRITE_FLUSH);
        return R_SUCCEEDED(last)&&written==size;
    }
};
}
