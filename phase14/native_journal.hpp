// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <3ds.h>
#include <cerrno>
#include <sys/stat.h>
#include "journal_codec.hpp"
#include "native_pxi.hpp"
namespace phase14 {
// One fixed recovery record blocks all later transactions, even completed ones.
// Deliberately no automatic cleanup, rename, overwrite or multi-record guessing.
// Parent /3ds/PKSM-Rogue/recovery must be created during explicit setup.
class NativeJournal {
    static constexpr const char* Path="/3ds/PKSM-Rogue/recovery/phase14.rec";
    static bool readAt(const std::string& path,Bytes& bytes) {
        bytes.clear(); FS_Archive sd{}; Handle file=0;
        if(R_FAILED(FSUSER_OpenArchive(&sd,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""))))return false;
        if(R_FAILED(FSUSER_OpenFile(&file,sd,fsMakePath(PATH_ASCII,path.c_str()),FS_OPEN_READ,0))){
            FSUSER_CloseArchive(sd);return false;
        }
        u64 size=0;u32 got=0;
        bool ok=R_SUCCEEDED(FSFILE_GetSize(file,&size)) && size<=2*MaxContainer+64;
        if(ok){bytes.resize(size);ok=R_SUCCEEDED(FSFILE_Read(file,&got,0,bytes.data(),u32(size)))&&got==size;}
        const bool closed=R_SUCCEEDED(FSFILE_Close(file));
        const bool archived=R_SUCCEEDED(FSUSER_CloseArchive(sd));
        if(!ok||!closed||!archived){bytes.clear();return false;}return true;
    }
    static bool createAt(const std::string& path,const Bytes& bytes) {
        FS_Archive sd{};Handle file=0;
        if(bytes.empty()||bytes.size()>2*MaxContainer+64 ||
           R_FAILED(FSUSER_OpenArchive(&sd,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""))))return false;
        const auto fp=fsMakePath(PATH_ASCII,path.c_str());
        // CreateFile must succeed EXCLUSIVELY; never open an existing record RW.
        if(R_FAILED(FSUSER_CreateFile(sd,fp,0,bytes.size()))){FSUSER_CloseArchive(sd);return false;}
        if(R_FAILED(FSUSER_OpenFile(&file,sd,fp,FS_OPEN_WRITE,0))){FSUSER_CloseArchive(sd);return false;}
        u32 written=0;
        const bool ok=R_SUCCEEDED(FSFILE_Write(file,&written,0,bytes.data(),bytes.size(),FS_WRITE_FLUSH))&&written==bytes.size();
        const bool closed=R_SUCCEEDED(FSFILE_Close(file));
        const bool archived=R_SUCCEEDED(FSUSER_CloseArchive(sd));
        Bytes back;return ok&&closed&&archived&&readAt(path,back)&&back==bytes;
    }
public:
    bool unresolved()const {
        // Includes partial/corrupt records and IO errors. This is only an early
        // gate; exclusive FSUSER_CreateFile also prevents a stat/create race.
        const std::string mounted=std::string("sdmc:")+Path;
        struct stat s{};if(::stat(mounted.c_str(),&s)==0)return true;
        return errno!=ENOENT;
    }
    bool persist(const Plan& p){return createAt(Path,encodeJournal(p));}
    bool verifyJournal(const Plan& p)const{Bytes b;return readAt(Path,b)&&b==encodeJournal(p);}
    bool load(Plan& p)const{Bytes b;return readAt(Path,b)&&decodeJournal(b,p);}
    bool finish(Status s) {
        if(s!=Status::Restored&&s!=Status::RolledBack)return false;
        const std::string path=std::string(Path)+(s==Status::Restored?".restored":".rolledback");
        Bytes receipt{uint8_t(s)},existing;
        if(readAt(path,existing))return existing==receipt;
        return createAt(path,receipt);
    }
};
// All transaction and journal code compiles natively, but execute/recover
// immediately reject this backend because NativePxi::writesAllowed() is false.
class NativeBackend final:public NativePxi,public NativeJournal {};
}
