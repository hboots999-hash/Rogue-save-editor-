// Phase18 v3 SD-only multi-edit export. This separate API cannot be used
// with installed game archives and never replaces the proven v2 export.
#pragma once
#include "multi_edit_v3.hpp"
#include "offline_export.hpp"
#include <exception>
#include <new>

namespace rogue18 { namespace v3 {
inline ExportResult exportCopiedBatch(const uint8_t* expectedCopy,size_t n,
                                      const char* sourcePath,const char* root,
                                      const Batch& batch){
    try {
    if(!expectedCopy || n!=raw_v1::Size ||
       !approvedPaths(root,sourcePath))
        return {false,"Only exact fixed offline SD import is supported"};
    auto fresh=raw_v1::readExact(sourcePath);
    if(!fresh || std::memcmp(fresh.get(),expectedCopy,n)!=0)
        return {false,"Stale/missing offline source: blocked"};
    fresh.reset();
    auto candidate=preview(expectedCopy,n,batch);
    if(!candidate.ok)return {false,"V3 batch blocked: "+candidate.reason};
    if(!verify(expectedCopy,n,candidate.candidate.data(),
               candidate.candidate.size(),batch))
        return {false,"V3 aggregate independent proof failed"};
    fresh=raw_v1::readExact(sourcePath);
    if(!fresh || std::memcmp(fresh.get(),expectedCopy,n)!=0)
        return {false,"Stale source before multi-edit backup"};
    fresh.reset();
    if(!raw_v1::directory(root))return {false,"Offline SD folder unavailable"};
    const std::string backups=std::string(root)+"/backups";
    if(!raw_v1::directory(backups.c_str()))
        return {false,"Immutable backup folder unavailable"};
    std::string backup,reason;
    if(!raw_v1::uniqueWrite(backups,"phase18_original_",expectedCopy,backup,reason))
        return {false,"V3 backup creation/readback uncertain"+
                (backup.empty()?std::string{}:std::string(" [attempted backups/")+backup+"]")+
                ": "+reason,backup};
    fresh=raw_v1::readExact(sourcePath);
    if(!fresh || std::memcmp(fresh.get(),expectedCopy,n)!=0)
        return {false,"Source changed after immutable backup: no batch draft",backup};
    fresh.reset();
    const std::string drafts=std::string(root)+"/drafts";
    if(!raw_v1::directory(drafts.c_str()))
        return {false,"Draft directory unavailable",backup};
    std::string draft;
    if(!raw_v1::uniqueWrite(drafts,"phase18_draft_",candidate.candidate.data(),
                            draft,reason))
        return {false,"V3 draft creation/readback uncertain"+
                (draft.empty()?std::string{}:std::string(" [attempted drafts/")+draft+"]")+
                ": "+reason,backup,draft};
    fresh=raw_v1::readExact(sourcePath);
    if(!fresh || std::memcmp(fresh.get(),expectedCopy,n)!=0)
        return {false,"Source changed after draft write: do not use batch draft",
                backup,draft};
    fresh.reset();
    auto readback=raw_v1::readExact((drafts+"/"+draft).c_str());
    if(!readback || !verify(expectedCopy,n,readback.get(),n,batch))
        return {false,"Stored batch draft failed independent proof",backup,draft};
    return {true,"V3 OFFLINE multi-edit draft only, NEVER restore to installed save",
            backup,draft,candidate.changedOffsets.size()};
    } catch(const std::bad_alloc&) {
        // No success can escape an allocator failure. Any already-created
        // uniquely named output remains on disk for manual inspection.
        return {false,"Batch export memory exhaustion: no approved draft; inspect SD files"};
    } catch(const std::exception&) {
        return {false,"Batch export exception: no approved draft; inspect SD files"};
    }
}
}} // namespace rogue18::v3
