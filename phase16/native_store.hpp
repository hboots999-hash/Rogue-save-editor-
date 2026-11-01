// SPDX-License-Identifier: GPL-3.0-or-later
// Phase16 native SD-only immutable full-container backup and recovery journal.
// NOT the Phase14 live restore journal path. Never overwrites or removes files.
// Critical: readback does not by itself prove power-loss durability.
#pragma once
#include "rehearsal.hpp"
#include <3ds.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include "offline.hpp"

namespace phase16 {
class NativeStore {
    static constexpr const char* Root="sdmc:/3ds/PKSM-Rogue";
    static constexpr const char* Folder="sdmc:/3ds/PKSM-Rogue/phase16";
    static constexpr const char* Backup="/3ds/PKSM-Rogue/phase16/original_container.bin";
    static constexpr const char* Journal="/3ds/PKSM-Rogue/phase16/rehearsal_only.rec";
    static constexpr size_t Chunk=32768;
    static State presence(const char* path){
        const std::string mounted=std::string("sdmc:")+path;
        struct stat s{};
        errno=0;
        if(::stat(mounted.c_str(),&s)==0)
            return S_ISREG(s.st_mode)?State::Present:State::Error;
        return errno==ENOENT?State::Missing:State::Error;
    }
    static bool readAt(const char* path,phase14::Bytes& out){
        out.clear();
        FS_Archive sd{};
        ::Result rc=FSUSER_OpenArchive(&sd,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""));
        if(R_FAILED(rc))return false;
        Handle fd=0;
        const FS_Path fp=fsMakePath(PATH_ASCII,path);
        rc=FSUSER_OpenFile(&fd,sd,fp,FS_OPEN_READ,0);
        if(R_FAILED(rc)){(void)FSUSER_CloseArchive(sd);return false;}
        u64 size=0;rc=FSFILE_GetSize(fd,&size);
        bool ok=R_SUCCEEDED(rc) && size>=64 && size<=2*phase14::MaxContainer+64;
        if(ok){
            out.resize(static_cast<size_t>(size));
            for(size_t pos=0;pos<out.size();pos+=Chunk){
                const u32 len=static_cast<u32>(std::min(Chunk,out.size()-pos));
                u32 got=0;
                rc=FSFILE_Read(fd,&got,pos,out.data()+pos,len);
                if(R_FAILED(rc)||got!=len){ok=false;break;}
            }
        }
        const bool closeOK=R_SUCCEEDED(FSFILE_Close(fd));
        const bool archiveOK=R_SUCCEEDED(FSUSER_CloseArchive(sd));
        if(!ok||!closeOK||!archiveOK){out.clear();return false;}
        return true;
    }
    static bool createAt(const char* path,const phase14::Bytes& bytes){
        if(bytes.size()<64 || bytes.size()>2*phase14::MaxContainer+64)
            return false;
        if(!h168::directory(Root)||!h168::directory(Folder))return false;
        // Native FSUSER_CreateFile refuses an existing name, including a
        // partial file after a crash. We never delete or truncate it.
        FS_Archive sd{};
        ::Result rc=FSUSER_OpenArchive(&sd,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""));
        if(R_FAILED(rc))return false;
        const FS_Path fp=fsMakePath(PATH_ASCII,path);
        rc=FSUSER_CreateFile(sd,fp,0,bytes.size());
        if(R_FAILED(rc)){(void)FSUSER_CloseArchive(sd);return false;}
        Handle fd=0;
        rc=FSUSER_OpenFile(&fd,sd,fp,FS_OPEN_WRITE,0);
        if(R_FAILED(rc)){(void)FSUSER_CloseArchive(sd);return false;}
        bool writtenOK=true;
        for(size_t pos=0;pos<bytes.size();pos+=Chunk){
            const u32 len=static_cast<u32>(std::min(Chunk,bytes.size()-pos));
            u32 written=0;
            // Flushed each chunk. Preserve partial data on any failure.
            rc=FSFILE_Write(fd,&written,pos,bytes.data()+pos,len,FS_WRITE_FLUSH);
            if(R_FAILED(rc)||written!=len){writtenOK=false;break;}
        }
        const bool closeOK=R_SUCCEEDED(FSFILE_Close(fd));
        const bool archiveOK=R_SUCCEEDED(FSUSER_CloseArchive(sd));
        if(!writtenOK||!closeOK||!archiveOK)return false;
        phase14::Bytes reopened;
        return readAt(path,reopened) && reopened==bytes;
    }
public:
    State backupState()const{return presence(Backup);}
    State journalState()const{return presence(Journal);}
    bool createBackup(const phase14::Bytes& source)const{return createAt(Backup,source);}
    bool createJournal(const phase14::Bytes& journal)const{return createAt(Journal,journal);}
    bool readBackup(phase14::Bytes& bytes)const{return readAt(Backup,bytes);}
    bool readJournal(phase14::Bytes& bytes)const{return readAt(Journal,bytes);}
};
} // namespace phase16
