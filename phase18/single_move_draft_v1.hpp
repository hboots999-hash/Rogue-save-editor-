// SPDX-License-Identifier: GPL-3.0-or-later
// Phase18 single-move copied-save draft transaction. There is NO installed
// game archive, VC restore or CMAC API here. Release entrypoint is locked.
// Test-only proof/writer exercises real filesystem exclusive-create/verify.
#pragma once
#include "move_replacement_v2.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#if defined(ROGUE18_TEST_MOVE_V2)
#include <cerrno>
#include <cstdio>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace rogue18 { namespace move_draft_v1 {
constexpr uint32_t ContractVersion=1;
constexpr bool ReleaseDraftExportEnabled=false;
struct Evidence {
    uint32_t version=ContractVersion;
    Digest256 sourceSha{},draftSha{};
    uint32_t originalCounter=0;
    uint8_t party=0,slot=0;
    uint16_t oldMove=0,newMove=0;
    uint8_t oldPP=0,newPP=0;
    size_t changedCount=0;
};
struct Transaction {
    Evidence evidence{};
    // No path, user token or caller-supplied approval flag.
    bool authorized=false;
};
inline bool mayExport(const Transaction&){return false;}
// The actual real-save implementation is NOT connected until the ROM
// replacement rules and an independently loaded/resaved draft are proved.
// This API can safely be called by production Phase20 without touching SD.
inline bool exportToSd(const uint8_t*,size_t,const uint8_t*,size_t,
                       const move_v2::Catalog&,const move_v2::Request&,
                       Transaction& result){
    result=Transaction{};
    return false;
}

#if defined(ROGUE18_TEST_MOVE_V2)
inline bool verifyGenerated(const uint8_t* original,size_t n,
                            const uint8_t* draft,size_t len,
                            const move_v2::Request& request,
                            uint16_t moveId,uint8_t chosenPP,Evidence& out){
    out=Evidence{};
    if(!original || !draft || n!=raw_v1::Size || len!=n ||
       request.version!=move_v2::ContractVersion ||
       request.catalogDigest!=move_v2::CatalogSha ||
       !move_v2::verifySynthetic(original,n,draft,len,request,moveId,chosenPP))
        return false;
    raw_v1::Info before{},after{};
    if(!validate(original,n,before) || !validate(draft,len,after) ||
       before.active!=after.active ||
       before.banks[before.active].counter!=after.banks[after.active].counter)
        return false;
    size_t count=0;
    for(size_t i=0;i<n;i++)if(original[i]!=draft[i])++count;
    if(count==0 || count>8)return false;
    out.sourceSha=sourceDigest(original,n);
    out.draftSha=sourceDigest(draft,len);
    out.originalCounter=before.banks[before.active].counter;
    out.party=uint8_t(request.source.partySlot);
    out.slot=uint8_t(request.source.moveSlot);
    out.oldMove=request.source.expectedMoveId;
    out.newMove=moveId;
    out.oldPP=request.source.expectedPP;
    out.newPP=chosenPP;
    out.changedCount=count;
    return true;
}
// Host-only test tool for a mock SD directory, NOT for user release:
// root is from the test harness, never an installed game archive path.
// A real exporter requires a future separately reviewed versioned adapter.
inline bool safeTag(const char* tag){
    if(!tag || !tag[0])return false;
    size_t n=0;
    for(;tag[n] && n<32;n++){
        const char c=tag[n];
        if(!((c>='0' && c<='9') ||
             (c>='a' && c<='z') || (c>='A' && c<='Z') ||
             c=='-' || c=='_'))return false;
    }
    return n>0 && n<=24 && !tag[n];
}
inline bool writeNew(const std::string& filename,const uint8_t* data,size_t n){
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#if defined(O_NOFOLLOW)
    flags|=O_NOFOLLOW;
#endif
    const int fd=::open(filename.c_str(),flags,0600);
    if(fd<0)return false;
    bool good=true;
    for(size_t off=0;off<n;){
        const ssize_t written=::write(fd,data+off,n-off);
        if(written<=0){good=false;break;}
        off+=size_t(written);
    }
    if(good && ::fsync(fd)!=0)good=false;
    if(::close(fd)!=0)good=false;
    if(!good)::unlink(filename.c_str());
    return good;
}
inline bool readBack(const std::string& filename,const uint8_t* expected,size_t n){
    int flags=O_RDONLY;
#if defined(O_NOFOLLOW)
    flags|=O_NOFOLLOW;
#endif
    const int fd=::open(filename.c_str(),flags);
    if(fd<0)return false;
    struct stat st{};
    bool good=(::fstat(fd,&st)==0 && S_ISREG(st.st_mode) &&
               st.st_size==static_cast<off_t>(n));
    uint8_t chunk[4096]{};
    size_t at=0;
    while(good && at<n){
        const size_t needed=std::min(sizeof(chunk),n-at);
        const ssize_t got=::read(fd,chunk,needed);
        if(got<=0 || std::memcmp(chunk,expected+at,size_t(got))!=0){
            good=false;break;
        }
        at+=size_t(got);
    }
    if(::close(fd)!=0)good=false;
    return good && at==n;
}
inline bool exportSyntheticToMockSd(const char* root,const char* token,
                                    const uint8_t* original,size_t n,
                                    const uint8_t* draft,size_t len,
                                    const move_v2::Request& request,
                                    uint16_t moveId,uint8_t basePP,
                                    Evidence& output){
    output=Evidence{};
    if(!root || !safeTag(token) ||
       !verifyGenerated(original,n,draft,len,request,moveId,basePP,output))
        return false;
    // Fail closed for accidental archive-like input. Actual production
    // path is fixed in Phase20 and still has no writer attached.
    const std::string dir(root);
    if(dir.empty() || dir.find("..")!=std::string::npos ||
       dir.rfind("sdmc:",0)==0 || dir.rfind("romfs:",0)==0 ||
       dir.rfind("nand:",0)==0)return false;
    struct stat st{};
    if(::stat(dir.c_str(),&st)!=0 || !S_ISDIR(st.st_mode))return false;
    const std::string base=dir+"/move_"+token;
    const std::string backup=base+"_backup.sav";
    const std::string candidate=base+"_draft.sav";
    if(!writeNew(backup,original,n))return false;
    if(!readBack(backup,original,n))return false;
    if(!writeNew(candidate,draft,len))return false; // backup remains intact
    if(!readBack(candidate,draft,len)){
        ::unlink(candidate.c_str());
        return false;
    }
    // Fresh readback proves both exact SHA values, not merely memcpy:
    if(!readBack(backup,original,n) ||
       sourceDigest(original,n)!=output.sourceSha ||
       sourceDigest(draft,len)!=output.draftSha){
        ::unlink(candidate.c_str());
        return false;
    }
    return true;
}
#endif
}} // namespace rogue18::move_draft_v1
