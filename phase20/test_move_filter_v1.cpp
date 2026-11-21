// SPDX-License-Identifier: GPL-3.0-or-later
// No private ROM/save content or installed-game functions are required.
#include "phase20/move_filter_v1.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace phase20;
int main(){
    static phase19::MoveRecord records[]{
        {53,15,phase19::MoveEvidence::Candidate,"Flamethrower"},
        {58,0,phase19::MoveEvidence::ConflictingPP,"Ice Beam"},
        {85,15,phase19::MoveEvidence::Candidate,"Thunderbolt"},
        {150,30,phase19::MoveEvidence::Candidate,"Splash"},
        {165,0,phase19::MoveEvidence::Special,"Struggle"},
        {347,15,phase19::MoveEvidence::Candidate,"Calm Mind"},
        {900,0,phase19::MoveEvidence::Dummy,"Dummy"}
    };
    static phase19::MoveCatalog catalog{};
    catalog.records=records;
    catalog.count=sizeof(records)/sizeof(records[0]);
    catalog.origin=phase19::CatalogOrigin::ExactRomReviewed;
    assert(phase19::validMoveCatalog(catalog));
    static phase19::Demo data{};
    phase19::populateMock(data);
    // For this test we need a fictional synthetic party but an exact-origin
    // catalog; it is intentionally not an authorisation to write.
    static phase19::MoveBrowser browser{};
    assert(browser.initialize(catalog,data,0));
    assert(browser.browse(data,phase19::MoveKey::A));
    assert(browser.page==phase19::MovePage::Catalog);
    MoveListFilter filter{};
    assert(filter.apply(browser) && browser.matchCount==7);
    assert(filter.next(browser) && filter.mode==MoveListMode::MatchingPP);
    assert(browser.matchCount==4 && browser.selected==0);
    assert(filter.next(browser) && filter.mode==MoveListMode::PPConflict);
    assert(browser.matchCount==1);
    assert(browser.selectedMove()->id==58);
    assert(filter.next(browser) && filter.mode==MoveListMode::Reviewed43);
    assert(browser.matchCount==3); // 53, 85, 347 are inside the 43-ID set
    for(unsigned i=0;i<browser.matchCount;i++){
        unsigned j=0;assert(browser.matchAt(i,j));
        assert(rogue18::move_v3::selectedOrdinary(catalog.records[j].id));
    }
    assert(filter.next(browser) && filter.mode==MoveListMode::All);
    assert(browser.matchCount==7);
    // Preserve search text while filtering and do not expose old ordinal.
    std::strcpy(browser.query,"BEAM");
    assert(filter.apply(browser) && browser.matchCount==1);
    assert(browser.selectedMove()->id==58);
    filter.mode=MoveListMode::Reviewed43;
    assert(filter.apply(browser) && browser.matchCount==0);
    assert(browser.selected==0 && browser.selectedMove()==nullptr);
    browser.page=phase19::MovePage::Review;
    assert(!filter.next(browser));
    browser.page=phase19::MovePage::Catalog;
    assert(filter.next(browser));
    assert(filter.mode==MoveListMode::All && browser.matchCount==1);
    catalog.origin=phase19::CatalogOrigin::Synthetic;
    filter.mode=MoveListMode::Reviewed43;
    assert(filter.apply(browser) && browser.matchCount==0);
    assert(!isExperimentalDraftCandidate(browser,data));
    assert(!ppVariants(nullptr,browser,58).verified);
    assert(!ppVariants(nullptr,browser,936).verified);
    assert(std::strcmp(filterLabel(MoveListMode::PPConflict),"PP CONFLICT")==0);
    std::cout<<"PASS read-only move filters; search retention, 43-ID provenance, invalid PP fallback\\n";
}
}
