// SPDX-License-Identifier: GPL-3.0-or-later
// Phase20 read-only catalogue filters and ROM variant-PP presentation.
// This wrapper never edits a Pokémon or changes the underlying catalog;
// the 43-ID whitelist is a research gate, not gameplay approval.
#pragma once
#include "phase20/move_catalog_bridge_v1.hpp"
#include "phase18/move_replacement_v3.hpp"
#include "move_browser.hpp"
#include <cstdint>
namespace phase20 {
enum class MoveListMode : uint8_t {
    All=0, MatchingPP=1, PPConflict=2, Reviewed43=3
};
inline const char* filterLabel(MoveListMode mode){
    switch(mode){
        case MoveListMode::All:return "ALL";
        case MoveListMode::MatchingPP:return "PP MATCH";
        case MoveListMode::PPConflict:return "PP CONFLICT";
        case MoveListMode::Reviewed43:return "43 RESEARCH";
    }
    return "INVALID";
}
inline bool filterAllows(const phase19::MoveRecord& rec,
                         phase19::CatalogOrigin origin,MoveListMode mode){
    switch(mode){
        case MoveListMode::All:return true;
        case MoveListMode::MatchingPP:
            return rec.evidence==phase19::MoveEvidence::Candidate &&
                   rec.basePP>=1u && rec.basePP<=64u;
        case MoveListMode::PPConflict:
            return rec.evidence==phase19::MoveEvidence::ConflictingPP;
        case MoveListMode::Reviewed43:
            return origin==phase19::CatalogOrigin::ExactRomReviewed &&
                   rec.evidence==phase19::MoveEvidence::Candidate &&
                   rogue18::move_v3::selectedOrdinary(rec.id);
    }
    return false;
}
struct MoveListFilter {
    MoveListMode mode=MoveListMode::All;
    void reset(){mode=MoveListMode::All;}
    bool apply(phase19::MoveBrowser& browser) const{
        if(!browser.ready || !browser.catalog ||
           browser.catalog->count>phase19::MaxMoveRecords)return false;
        // Browser.recount() only recomputes the NAME search on its own bounded
        // alphabetical index. Filter in place, preserving that order.
        browser.recount();
        unsigned kept=0;
        for(unsigned i=0;i<browser.matchCount;i++){
            const unsigned index=browser.matches[i];
            if(index>=browser.catalog->count)continue;
            const auto& rec=browser.catalog->records[index];
            if(filterAllows(rec,browser.catalog->origin,mode))
                browser.matches[kept++]=static_cast<uint16_t>(index);
        }
        browser.matchCount=kept;
        browser.selected=0u; // never point at stale selection across a filter
        browser.confirmationReleased=false;
        return true;
    }
    bool next(phase19::MoveBrowser& browser){
        if(browser.page!=phase19::MovePage::Catalog || !browser.ready)
            return false;
        const unsigned old=static_cast<unsigned>(mode);
        mode=static_cast<MoveListMode>((old+1u)%4u);
        return apply(browser);
    }
};
// Only the EXACT authenticated original packed catalogue can resolve BOTH
// mainline and revised PP for a move. Never select a ruleset by assumption.
struct MovePPVariants {
    bool verified=false;
    uint8_t mainline=0,revised=0;
    bool conflict=false;
};
inline MovePPVariants ppVariants(const LocalMoveProvider* provider,
                                 const phase19::MoveBrowser& browser,
                                 unsigned id){
    MovePPVariants pp{};
    if(!provider || !provider->loaded || !browser.catalog ||
       browser.catalog!=provider->verifiedMetadata() ||
       browser.catalog->origin!=phase19::CatalogOrigin::ExactRomReviewed ||
       id==0 || id>=rogue18::move_v2::RecordCount)return pp;
    const size_t at=size_t(id)*rogue18::move_v2::PackedRowBytes;
    pp.verified=true;
    pp.mainline=provider->packed[at+13u];
    pp.revised=provider->packed[at+14u];
    pp.conflict=pp.mainline!=pp.revised;
    return pp;
}
inline bool isExperimentalDraftCandidate(
    const phase19::MoveBrowser& browser,const phase19::Demo& display){
    const auto* selected=browser.selectedMove();
    return !display.synthetic && browser.activeParty(display) &&
           browser.catalog &&
           browser.catalog->origin==phase19::CatalogOrigin::ExactRomReviewed &&
           selected && selected->evidence==phase19::MoveEvidence::Candidate &&
           rogue18::move_v3::selectedOrdinary(selected->id) &&
           browser.reviewBlocker(display)==phase19::MoveNotice::ReplacementBlocked;
}
static_assert(!rogue18::move_v3::MayWriteInstalledGame,
              "Filters are read-only and 43 candidate IDs are not live permissions");
} // namespace phase20
