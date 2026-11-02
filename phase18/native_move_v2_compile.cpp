// ARM11 compiler probe for release-locked exact-ROM v2 and v4 queue.
// Never executed or linked into CIA. No installed-game APIs.
#include "move_queue_v4.hpp"
#include "single_move_draft_v1.hpp"
namespace phase18_move_v2_native {
static_assert(!rogue18::move_v2::ApplyApproved,
              "Do not unlock without source/binary/runtime release gates");
bool compileReadOnly(const uint8_t* source,size_t n,
                     const rogue18::move_v2::Catalog& db){
    const auto snapshot=rogue18::v2::inspect(source,n);
    rogue18::move_v2::Request edit{};
    if(!rogue18::move_v2::begin(snapshot,source,n,0,0,edit))return false;
    rogue18::move_v2::Move record{};
    if(!rogue18::move_v2::lookup(db,53,record))return false;
    const auto result=rogue18::move_v2::preview(source,n,db,edit);
    return !result.ok && !rogue18::move_v2::mayRestoreInstalledGame(edit);
}
bool compileQueue(const uint8_t* source,size_t n,
                  const rogue18::move_v2::Catalog& catalog){
    const auto snapshot=rogue18::v2::inspect(source,n);
    rogue18::move_queue_v4::Queue queue{};
    if(!rogue18::move_queue_v4::begin(snapshot,queue))return false;
    const bool staged=rogue18::move_queue_v4::appendReplacement(
                          snapshot,source,n,catalog,queue,0,0,53);
    const auto result=rogue18::move_queue_v4::preview(source,n,queue);
    return staged && !result.ok &&
           !rogue18::move_queue_v4::mayWriteInstalledGame(queue);
}
bool compileSingleReview(const uint8_t* source,size_t n,
                         const rogue18::move_v2::Catalog& db,
                         const rogue18::move_queue_v4::Queue& queue){
    rogue18::move_queue_v4::SingleMoveReview review{};
    const bool valid=rogue18::move_queue_v4::reviewSingle(source,n,db,queue,review);
    rogue18::move_draft_v1::Transaction transaction{};
    rogue18::move_v2::Request request{};
    const bool exported=rogue18::move_draft_v1::exportToSd(
       source,n,source,n,db,request,transaction);
    return valid && !exported && !transaction.authorized;
}
#if defined(ROGUE18_TEST_MOVE_V2)
bool compileSynthetic(const uint8_t* source,size_t n,
                      const rogue18::move_v2::Request& edit){
    const auto check=rogue18::move_v2::previewSynthetic(
                             source,n,edit,501,15);
    return check.ok &&
           rogue18::move_v2::verifySynthetic(
              source,n,check.candidate.data(),check.candidate.size(),
              edit,501,15);
}
#endif
} // namespace phase18_move_v2_native
