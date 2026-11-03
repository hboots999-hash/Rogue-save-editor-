// SPDX-License-Identifier: GPL-3.0-or-later
// Exact-ROM MOVE-V3 experimental OFFLINE DRAFT exporter.
// Writes ONLY new backup and separate candidate files beneath a fixed SD
// directory. Never opens/writes installed GBA VC archive or live save.
#pragma once
#include "move_replacement_v3.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace rogue18 { namespace move_export_v3 {
constexpr uint32_t ContractVersion=3;
constexpr bool MayRestoreInstalledGame=false;
constexpr const char* FixedSdDraftDir="sdmc:/3ds/PKSM-Rogue/drafts";
struct Receipt {
    bool ok=false;
    uint32_t version=ContractVersion;
    Digest256 originalSha{},draftSha{};
    uint32_t counter=0;
    uint16_t previousMove=0,newMove=0;
    unsigned partySlot=0,moveSlot=0;
    size_t changedBytes=0;
    char backupFile[192]{};
    char draftFile[192]{};
};
inline bool tokenSafe(const char* token){
    if(!token)return false;
    for(size_t i=0;i<=24;i++){
        const char ch=token[i];
        if(!ch)return i>=1;
        if(i==24 || !((ch>='0' && ch<='9') ||
             (ch>='A' && ch<='Z') || (ch>='a' && ch<='z') ||
              ch=='_' || ch=='-'))return false;
    }
    return false;
}
inline bool writeExclusive(const char* path,const uint8_t* bytes,size_t n){
    if(!path || !bytes || !n)return false;
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifdef O_NOFOLLOW
    flags|=O_NOFOLLOW;
#endif
    const int fd=::open(path,flags,0600);
    if(fd<0)return false;
    bool ok=true;
    for(size_t offset=0;offset<n;){
        const ssize_t wrote=::write(fd,bytes+offset,n-offset);
        if(wrote<=0){ok=false;break;}
        offset+=size_t(wrote);
    }
    if(ok && ::fsync(fd)!=0)ok=false;
    if(::close(fd)!=0)ok=false;
    if(!ok)::unlink(path);
    return ok;
}
inline bool verifyFile(const char* path,const uint8_t* bytes,size_t n){
    if(!path || !bytes || !n)return false;
    int flags=O_RDONLY;
#ifdef O_NOFOLLOW
    flags|=O_NOFOLLOW;
#endif
    const int fd=::open(path,flags);
    if(fd<0)return false;
    struct stat st{};
    bool ok=(::fstat(fd,&st)==0 && S_ISREG(st.st_mode) &&
             st.st_size==static_cast<off_t>(n));
    uint8_t chunk[1024]{};
    size_t position=0;
    while(ok && position<n){
        const size_t want=std::min(size_t(1024),n-position);
        const ssize_t got=::read(fd,chunk,want);
        if(got<=0 || std::memcmp(chunk,bytes+position,size_t(got))!=0){
            ok=false;break;
        }
        position+=size_t(got);
    }
    if(::close(fd)!=0)ok=false;
    return ok && position==n;
}
inline bool isDirectory(const char* path){
    struct stat st{};
    return path && ::stat(path,&st)==0 && S_ISDIR(st.st_mode);
}
namespace detail {
// Only accepted as production destination when exactly the SD folder
// above. Synthetic regression tests may use disposable /tmp directories.
inline bool allowedRoot(const char* root,bool testOnly){
    if(!root || !isDirectory(root))return false;
    if(std::strcmp(root,FixedSdDraftDir)==0)return true;
#if defined(ROGUE18_TEST_MOVE_V2)
    if(testOnly && std::strncmp(root,"/tmp/pksm_v3_",13)==0 &&
       std::strstr(root,"..")==nullptr)return true;
#else
    (void)testOnly;
#endif
    return false;
}
inline bool writeVerified(const uint8_t* source,size_t n,
                          const move_v3::Selection& selected,
                          const Result& result,
                          const char* root,const char* token,
                          Receipt& report,bool testOnly){
    report=Receipt{};
    if(!source || n!=raw_v1::Size || !tokenSafe(token) ||
       !allowedRoot(root,testOnly) || !result.ok ||
       result.candidate.size()!=n ||
       sourceDigest(source,n)!=selected.original.source.expectedOriginalSha256 ||
       result.changedOffsets.empty() || result.changedOffsets.size()>8)
        return false;

    // No scalar multi-edit, installed title, archive, or CMAC operations.
    // Filename collisions are fatal, not an overwrite/retry.
    const int backupLength=std::snprintf(report.backupFile,
                sizeof(report.backupFile),"%s/move_%s_backup.sav",root,token);
    const int draftLength=std::snprintf(report.draftFile,
                sizeof(report.draftFile),"%s/move_%s_draft.sav",root,token);
    if(backupLength<0 || draftLength<0 ||
       size_t(backupLength)>=sizeof(report.backupFile) ||
       size_t(draftLength)>=sizeof(report.draftFile))return false;
    if(!writeExclusive(report.backupFile,source,n))return false;
    if(!verifyFile(report.backupFile,source,n))return false;
    if(!writeExclusive(report.draftFile,result.candidate.data(),n))
        return false;  // backup is retained even if export cannot proceed
    if(!verifyFile(report.draftFile,result.candidate.data(),n) ||
       !verifyFile(report.backupFile,source,n)){
        ::unlink(report.draftFile); // never touch backup
        return false;
    }
    raw_v1::Info info{};
    if(!validate(source,n,info)){
        ::unlink(report.draftFile);
        return false;
    }
    report.originalSha=sourceDigest(source,n);
    report.draftSha=sourceDigest(result.candidate.data(),n);
    report.counter=info.banks[info.active].counter;
    report.previousMove=selected.original.source.expectedMoveId;
    report.newMove=selected.original.replacementId;
    report.partySlot=selected.original.source.partySlot;
    report.moveSlot=selected.original.source.moveSlot;
    report.changedBytes=result.changedOffsets.size();
    report.ok=true;
    return true;
}
inline bool exportChecked(const uint8_t* source,size_t n,
                          const move_v2::Catalog& db,
                          const move_v3::Selection& selected,
                          const char* root,const char* token,
                          Receipt& report,bool testOnly){
    const auto proposed=move_v3::preview(source,n,db,selected);
    if(!proposed.ok ||
       !move_v3::verify(source,n,proposed.candidate.data(),
                        proposed.candidate.size(),db,selected))
        return false;
    return writeVerified(source,n,selected,proposed,root,token,report,testOnly);
}
} // namespace detail

inline bool exportOfflineDraft(const uint8_t* source,size_t n,
                               const move_v2::Catalog& db,
                               const move_v3::Selection& selected,
                               const char* token,Receipt& report){
    // Production cannot specify an arbitrary path, only the exact SD
    // draft directory. The user/app must ensure it exists beforehand.
    return detail::exportChecked(source,n,db,selected,
                                 FixedSdDraftDir,token,report,false);
}
#if defined(ROGUE18_TEST_MOVE_V2)
inline bool exportSyntheticToTemp(const uint8_t* source,size_t n,
                                  const move_v3::Selection& selected,
                                  uint8_t fabricatedPP,
                                  const char* temp,const char* token,
                                  Receipt& report){
    const auto candidate=move_v3::previewSynthetic(source,n,selected,
                                                    fabricatedPP);
    const auto independent=move_v2::previewSynthetic(
        source,n,selected.original,
        selected.original.replacementId,fabricatedPP);
    if(!candidate.ok || !independent.ok ||
       candidate.candidate!=independent.candidate)return false;
    return detail::writeVerified(source,n,selected,candidate,
                                 temp,token,report,true);
}
#endif
inline bool mayWriteInstalledGame(const Receipt&){return false;}
}} // namespace rogue18::move_export_v3
