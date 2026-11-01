// Compile-only ARM11 cross-check. Never linked into a release 3DS CIA.
#include "move_replacement_v1.hpp"
namespace phase18_move_native {
static_assert(!rogue18::move_v1::ExactRomMoveTableApproved,
              "Unverified ROM move list must remain off");
bool productionDisabled(const uint8_t* raw,size_t n,
                        const rogue18::move_v1::Request& req){
    const auto proposal=rogue18::move_v1::preview(raw,n,req);
    return !proposal.ok &&
           !rogue18::move_v1::capability().mayPreviewReplacement &&
           !rogue18::move_v1::mayWriteInstalledGame(req);
}
#if defined(ROGUE18_SYNTHETIC_MOVE_TEST)
bool compileResearchOnly(const uint8_t* raw,size_t n,
                         const rogue18::move_v1::Request& req,
                         const rogue18::move_v1::ResearchMove* rows,size_t count){
    // Compile representation/memory/stack ONLY; never call on hardware.
    const auto candidate=rogue18::move_v1::previewResearch(raw,n,req,rows,count);
    return candidate.ok &&
           rogue18::move_v1::verifyResearch(raw,n,candidate.candidate.data(),
                                           candidate.candidate.size(),req,rows,count);
}
#endif
} // namespace phase18_move_native
