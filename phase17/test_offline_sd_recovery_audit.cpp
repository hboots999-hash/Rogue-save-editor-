// SPDX-License-Identifier: GPL-3.0-or-later
// Host-only synthetic SD export observations. No real save or SD device.
#include "offline_sd_recovery_audit.hpp"
#include <cassert>
#include <iostream>
#include <cstdint>
using phase14::Bytes;
using namespace phase17;
int main() {
    Bytes original(131072u,0x21);
    Bytes candidate=original;
    candidate[0x230]=0x35; // synthetic byte-only change, NOT playable data
    Bytes observedImport=original,backup=original,draft=candidate;
    OfflineSdReviewEvidence e{};
    e.import={SdReadState::Complete,&observedImport};
    e.backup={SdReadState::Complete,&backup};
    e.draft={SdReadState::Complete,&draft};
    e.distinctNamesProven=true;
    e.exclusiveCreationProven=true;
    auto check=[&](RawExportReview expected) {
        const auto result=reviewOfflineSdOnly(original,candidate,e);
        assert(result.result==expected);
        assert(!result.canModifyInstalledGame());
        assert(!result.canRestoreAuthenticatedContainer());
        assert(!result.authorisesGameplayInjection());
        return result;
    };
    assert(check(RawExportReview::OfflineCopyReviewedOnly).rawCopyMatchesExpected);
    // Empty / unreadable SD: the original is never reinterpreted as an
    // empty-but-successful export.
    for(unsigned i=0;i<3;i++){
        auto saved=e;
        if(i==0)e.import.state=SdReadState::Missing;
        if(i==1)e.backup.state=SdReadState::Missing;
        if(i==2)e.draft.state=SdReadState::Missing;
        check(i==0?RawExportReview::ImportMissing:
              i==1?RawExportReview::BackupMissing:RawExportReview::DraftMissing);
        e=saved;
    }
    for(unsigned i=0;i<3;i++)for(auto state:{
        SdReadState::Unchecked,SdReadState::IOError,
        static_cast<SdReadState>(255u)}) {
        auto saved=e;
        if(i==0)e.import.state=state;
        if(i==1)e.backup.state=state;
        if(i==2)e.draft.state=state;
        check(RawExportReview::SDReadFailure);
        e=saved;
    }
    // Power loss, short native FSFILE_Write, partially created SD backups,
    // partial drafts, overlong data or a failed readback all block.
    for(unsigned which=0;which<3;which++){
        Bytes truncated=which==0?observedImport:which==1?backup:draft;
        auto saved=e;
        if(which==0)e.import.bytes=&truncated;
        if(which==1)e.backup.bytes=&truncated;
        if(which==2)e.draft.bytes=&truncated;
        for(size_t len:{size_t(0),size_t(1),size_t(32768),
                        size_t(131071),size_t(131073)}) {
            truncated.resize(len,0);
            check(RawExportReview::PartialOrWrongSize);
        }
        e=saved;
    }
    // Stale source fingerprint and full-byte changes outside the usual
    // sector checksums: a "valid raw layout" is not a fresh snapshot.
    observedImport.back()^=0x20;
    check(RawExportReview::StaleImport);
    observedImport=original;
    backup[17]^=0x01;
    check(RawExportReview::BackupMismatch);
    backup=original;
    draft[1024]^=0x20;
    check(RawExportReview::DraftMismatch);
    draft=candidate;
    // Written bytes alone cannot establish no-overwrite file creation.
    e.distinctNamesProven=false;
    check(RawExportReview::ReusedOrUnprovenNames);
    e.distinctNamesProven=true;
    e.exclusiveCreationProven=false;
    check(RawExportReview::ReusedOrUnprovenNames);
    e.exclusiveCreationProven=true;
    // A no-op "draft" is not proof of an applied scalar change.
    draft=original;
    const auto noChange=reviewOfflineSdOnly(original,original,e);
    assert(noChange.result==RawExportReview::NoChange);
    assert(!noChange.authorisesGameplayInjection());
    draft=candidate;
    assert(check(RawExportReview::OfflineCopyReviewedOnly).rawCopyMatchesExpected);
    std::cout<<"PASS Phase17 synthetic SD review: missing/read failure, "
       <<"partial writes, corrupt drafts, stale imports and exclusive "
       <<"file naming. RAW COPY ONLY; NO GAME RESTORE.\n";
}
