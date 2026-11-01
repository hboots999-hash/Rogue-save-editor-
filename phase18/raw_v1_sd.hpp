// Phase18 SD-only transport, separate from the read-only parser.
// Only the fixed copied import and immutable offline backup/draft exporter
// may use these utilities. No live game archive, no Phase17 restore.
#pragma once
#include "raw_v1.hpp"
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <new>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __3DS__
#include <3ds.h>
#endif
namespace rogue18 { namespace raw_v1 {
// Host-test-only fault injector. Never compiled into a native 3DS build.
// Simulates failures of the *real* exclusive-name/SD-copy write path,
// preserving partially created files and enforcing fail-closed results.
#if defined(ROGUE18_TEST_IO_FAULTS) && !defined(__3DS__)
enum class IoFault : uint8_t {
    None, OpenNoSpace, WriteNoSpace, PartialThenNoSpace,
    FlushFailure, CloseFailure, ReadbackFailure
};
struct IoFaultConfig {
    IoFault stage=IoFault::None;
    std::string nameContains{};
    unsigned writes=0;
};
inline IoFaultConfig ioFault{};
inline bool targetFault(const std::string& name) {
    return ioFault.stage!=IoFault::None &&
           name.find(ioFault.nameContains)!=std::string::npos;
}
#endif
struct FreeFILE{void operator()(FILE* f) const {if(f)fclose(f);}};
inline std::unique_ptr<uint8_t[]> readExact(const char* path){
    if(!path)return {};
#if defined(ROGUE18_TEST_IO_FAULTS) && !defined(__3DS__)
    if(ioFault.stage==IoFault::ReadbackFailure && targetFault(path))return {};
#endif
    std::unique_ptr<FILE,FreeFILE> in(fopen(path,"rb"));
    if(!in)return {};
    std::unique_ptr<uint8_t[]> b(new(std::nothrow) uint8_t[Size]);
    if(!b || fread(b.get(),1,Size,in.get())!=Size || fgetc(in.get())!=EOF ||
       ferror(in.get()))return {};
    return b;
}
inline bool directory(const char* path){
    if(mkdir(path,0777)!=0 && errno!=EEXIST)return false;
    struct stat st{};
    return stat(path,&st)==0 && S_ISDIR(st.st_mode);
}
inline std::string ioError(const char* operation,int code){
    char text[96];
    std::snprintf(text,sizeof(text),"%s (errno %d)",operation,code);
    return std::string(text);
}
#ifdef __3DS__
// The 3DS SDMC devoptab returned a native negative FS result for
// open(O_CREAT|O_EXCL). Use the native SDMC FS archive API for the
// exclusive create on HARDWARE; never alter live game save archives.
// FSUSER_CreateFile fails if the exact filename already exists.
// Only a separate, explicitly imported SD copy can be written as a draft.
inline std::string fsResult(const char* operation,::Result rc){
    char tmp[96];
    std::snprintf(tmp,sizeof(tmp),"%s FS=0x%08lX",operation,
                  static_cast<unsigned long>(static_cast<uint32_t>(rc)));
    return std::string(tmp);
}
inline bool nativeSdmcCreateAndWrite(const std::string& full,
                                    const uint8_t* bytes,std::string& error){
    const std::string prefix="sdmc:";
    if(!bytes || full.compare(0,prefix.size(),prefix)!=0 ||
       full.size()<=prefix.size() || full[prefix.size()]!='/'){
        error="Invalid SDMC-only target";return false;
    }
    FS_Archive archive{};
    ::Result rc=FSUSER_OpenArchive(&archive,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""));
    if(R_FAILED(rc)){error=fsResult("SDMC archive",rc);return false;}
    const FS_Path path=fsMakePath(PATH_ASCII,full.c_str()+prefix.size());
    // Pre-create at the required size. Never open with CREATE/TRUNCATE.
    rc=FSUSER_CreateFile(archive,path,0,Size);
    if(R_FAILED(rc)){
        error=fsResult("CreateFile",rc);
        (void)FSUSER_CloseArchive(archive);return false;
    }
    Handle file=0;
    rc=FSUSER_OpenFile(&file,archive,path,FS_OPEN_WRITE,0);
    if(R_FAILED(rc)){
        error=fsResult("OpenFile",rc);
        (void)FSUSER_CloseArchive(archive);return false;
    }
    uint32_t written=0;
    rc=FSFILE_Write(file,&written,0,bytes,Size,FS_WRITE_FLUSH);
    const ::Result closed=FSFILE_Close(file);
    const ::Result archived=FSUSER_CloseArchive(archive);
    if(R_FAILED(rc)){error=fsResult("WriteFile",rc);return false;}
    if(written!=Size){error="WriteFile short count";return false;}
    if(R_FAILED(closed)){error=fsResult("CloseFile",closed);return false;}
    if(R_FAILED(archived)){error=fsResult("CloseArchive",archived);return false;}
    return true;
}
#endif

// Preserve every pre-existing file; failures are reported by stage and errno.
// Any partial file is left at its unique name for inspection; never overwritten.
inline bool uniqueWrite(const std::string& folder,const char* prefix,
                        const uint8_t* bytes,std::string& filename,
                        std::string& error){
    error.clear();
    if(!bytes){error="Null source";return false;}
    for(unsigned i=1;i<=99;i++){
        char base[64];
        std::snprintf(base,sizeof(base),"%s%03u.sav",prefix,i);
        const std::string full=folder+"/"+base;
#ifdef __3DS__
        // stat is used only to skip existing paths. The actual create is
        // atomic in FSUSER_CreateFile; no saved file can be overwritten.
        struct stat existing{};
        if(::stat(full.c_str(),&existing)==0)continue;
        if(!nativeSdmcCreateAndWrite(full,bytes,error)){
            // If another application created a file after stat, do not
            // delete/retry/overwrite that name. Fail closed.
            return false;
        }
#else
        errno=0;
#if defined(ROGUE18_TEST_IO_FAULTS) && !defined(__3DS__)
        const bool inject=targetFault(full);
        if(inject)ioFault.writes=0;
        const bool rejectCreate=inject && ioFault.stage==IoFault::OpenNoSpace;
        if(rejectCreate)errno=ENOSPC;
        int fd=rejectCreate?-1:::open(full.c_str(),O_CREAT|O_EXCL|O_WRONLY,0666);
#else
        int fd=::open(full.c_str(),O_CREAT|O_EXCL|O_WRONLY,0666);
#endif
        if(fd<0){
            const int e=errno;
            if(e==EEXIST)continue;
            error=ioError("open",e);return false;
        }
        size_t count=0;
        while(count<Size){
            errno=0;
            ssize_t n=0;
#if defined(ROGUE18_TEST_IO_FAULTS) && !defined(__3DS__)
            if(inject && (ioFault.stage==IoFault::WriteNoSpace ||
                (ioFault.stage==IoFault::PartialThenNoSpace && ioFault.writes++>0))) {
                errno=ENOSPC;n=-1;
            } else if(inject && ioFault.stage==IoFault::PartialThenNoSpace) {
                n=::write(fd,bytes+count,(Size-count)/2u);
            } else n=::write(fd,bytes+count,Size-count);
#else
            n=::write(fd,bytes+count,Size-count);
#endif
            if(n<=0){
                const int e=errno;
                error=ioError("write",e);
                (void)::close(fd);return false;
            }
            count+=size_t(n);
        }
        errno=0;
        int synced=::fsync(fd);
#if defined(ROGUE18_TEST_IO_FAULTS) && !defined(__3DS__)
        if(inject && ioFault.stage==IoFault::FlushFailure){
            errno=EIO;synced=-1;
        }
#endif
        const int syncError=errno;
        errno=0;
        int closed=::close(fd);
#if defined(ROGUE18_TEST_IO_FAULTS) && !defined(__3DS__)
        if(inject && ioFault.stage==IoFault::CloseFailure){
            errno=EIO;closed=-1;
        }
#endif
        const int closeError=errno;
        if(synced!=0){
            error=ioError("fsync",syncError);return false;
        }
        if(closed!=0){
            error=ioError("close",closeError);return false;
        }
#endif
        const auto back=readExact(full.c_str());
        if(!back){error="readback open/size";return false;}
        if(std::memcmp(back.get(),bytes,Size)!=0){
            error="readback content mismatch";return false;
        }
        filename=base;
        return true;
    }
    error="all 99 backup names occupied";
    return false;
}
} // namespace raw_v1
} // namespace rogue18
