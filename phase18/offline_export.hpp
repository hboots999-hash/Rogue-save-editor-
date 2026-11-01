// Phase 18: offline SD-import copy to separately named, verified SD draft.
// NEVER accepts a game archive; NEVER writes to source import or live title.
// GPL-3.0-or-later. Reuses Phase10 exclusive no-overwrite SDMC functions.
#pragma once
#include "offline_engine.hpp"
#include <cstring>
#include <memory>
#include <string>

namespace rogue18 {
struct ExportResult {
    bool ok=false;
    std::string reason,backupName,draftName;
    size_t changed=0;
};
// Strict Phase10 import location, not a caller-selected game-save destination.
// On native 3DS the root is limited to this application's own SD folder.
// This API never consumes a title ID or an archive handle.
inline bool approvedPaths(const char* root,const char* sourcePath){
    if(!root || !sourcePath)return false;
    const std::string dir(root);
    if(dir.empty() || dir.find("..")!=std::string::npos ||
       dir.back()=='/' || dir.find('\\')!=std::string::npos)return false;
#ifdef __3DS__
    if(dir!="sdmc:/3ds/PKSM-Rogue")return false;
#endif
    return std::string(sourcePath)==dir+"/imports/00000001.sav";
}
inline ExportResult exportCopied(const uint8_t* expectedCopy,size_t n,
                                 const char* sourcePath,const char* root,
                                 const Request& req,
                                 const Evidence& evidence=Evidence{}){
    if(!expectedCopy || n!=h168::Size || !approvedPaths(root,sourcePath))
        return {false,"Only the fixed offline SD import path is supported"};
    auto read=h168::readExact(sourcePath);
    if(!read || std::memcmp(read.get(),expectedCopy,n)!=0)
        return {false,"Stale or missing offline source: blocked"};
    const auto prepared=propose(expectedCopy,n,req,evidence);
    if(!prepared.ok)return {false,"Edit rejected: "+prepared.reason};
    if(!verifyExact(expectedCopy,n,prepared.candidate.data(),
                    prepared.candidate.size(),req,evidence))
        return {false,"Byte-exact candidate proof failed"};
    read=h168::readExact(sourcePath);
    if(!read || std::memcmp(read.get(),expectedCopy,n)!=0)
        return {false,"Stale source before backup"};
    if(!h168::directory(root))return {false,"Offline SD folder unavailable"};
    const std::string backups=std::string(root)+"/backups";
    if(!h168::directory(backups.c_str()))return {false,"Backup directory unavailable"};
    std::string backup,reason;
    if(!h168::uniqueWrite(backups,"phase18_original_",expectedCopy,backup,reason))
        return {false,"Immutable backup creation/readback failed: "+reason};
    read=h168::readExact(sourcePath);
    if(!read || std::memcmp(read.get(),expectedCopy,n)!=0)
        return {false,"Stale after backup: draft not created",backup};
    const std::string drafts=std::string(root)+"/drafts";
    if(!h168::directory(drafts.c_str()))
        return {false,"Draft directory unavailable",backup};
    std::string draft;
    if(!h168::uniqueWrite(drafts,"phase18_draft_",prepared.candidate.data(),
                           draft,reason))
        return {false,"Draft creation/readback failed: "+reason,backup};
    read=h168::readExact(sourcePath);
    if(!read || std::memcmp(read.get(),expectedCopy,n)!=0)
        return {false,"Source changed after export: do not use draft",backup,draft};
    auto stored=h168::readExact((drafts+"/"+draft).c_str());
    if(!stored || !verifyExact(expectedCopy,n,stored.get(),h168::Size,req,evidence))
        return {false,"Stored draft failed byte-exact proof",backup,draft};
    return {true,"OFFLINE SD draft only; NEVER restore without separate release gates",
            backup,draft,prepared.changedOffsets.size()};
}
} // namespace rogue18
