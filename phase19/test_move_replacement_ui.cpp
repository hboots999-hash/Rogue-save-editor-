// Strict read-only Phase19 six-party, move replacement review and API gate tests.
// Generated fictional metadata and synthetic save only; never edit files.
#include "move_browser.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
using namespace phase19;
static void enterReview(MoveBrowser& b,const Demo& d) {
    assert(b.page==MovePage::Slots);
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::Catalog);
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::Review);
}
static void confirmOne(MoveBrowser& b,const Demo& d) {
    assert(b.page==MovePage::Review);
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::Review); // A pressed without release
    b.releaseApproval();
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::ConfirmOne);
}
static void confirmTwo(MoveBrowser& b,const Demo& d) {
    assert(b.page==MovePage::ConfirmOne);
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::ConfirmOne);
    b.releaseApproval();
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::ConfirmTwo);
}
int main(){
    static Demo d{};
    populateMock(d);
    d.partyCount=6;
    for(unsigned i=3;i<6;i++){
        d.party[i]=d.party[0];
        std::snprintf(d.party[i].name,sizeof(d.party[i].name),
                      "Test Mon %u",i+1);
        d.party[i].species+=i;
    }
    const MoveCatalog& catalog=syntheticMoveCatalog();
    static MoveBrowser b{};
    assert(b.beginPartySelection(catalog,d));
    assert(b.page==MovePage::Party);
    for(unsigned i=0;i<5;i++)assert(b.browse(d,MoveKey::Down));
    assert(b.partySlot==5);
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::Slots && b.partySlot==5);
    assert(b.browse(d,MoveKey::B));
    assert(b.page==MovePage::Party);
    assert(b.browse(d,MoveKey::B)==false);
    // No party and all eggs: picker remains safe and never opens a slot.
    d.partyCount=0;
    assert(b.beginPartySelection(catalog,d));
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::Party);
    d.partyCount=6;
    d.party[1].egg=true;
    assert(b.beginPartySelection(catalog,d));
    assert(b.browse(d,MoveKey::Down));
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::Party);
    d.party[1].egg=false;

    // Every occupied slot can be selected without L.
    for(unsigned party=0;party<6;party++){
        assert(b.initialize(catalog,d,party));
        for(unsigned slot=0;slot<4;slot++){
            assert(b.page==MovePage::Slots);
            assert(b.slot==slot);
            assert(b.browse(d,MoveKey::A));
            assert(b.page==MovePage::Catalog);
            assert(b.browse(d,MoveKey::B));
            assert(b.page==MovePage::Slots);
            if(slot<3)assert(b.browse(d,MoveKey::Down));
        }
    }
    // Full two-stage review with B cancel on every page and X-only final.
    assert(b.initialize(catalog,d,0));
    enterReview(b,d);
    MoveSelectionIntentV1 request{};
    assert(describeMoveSelection(b,d,request));
    assert(request.version==MoveSelectionIntentVersion);
    assert(request.partySlot==0 && request.moveSlot==0);
    assert(request.species==d.party[0].species);
    assert(request.originalMoveId==d.party[0].moves[0]);
    assert(request.replacementMoveId!=request.originalMoveId);
    assert(request.replacementBasePP!=0);
    assert(!MoveReplacementGateV1::mayApply(request));
    assert(!MoveReplacementGateV1::productionPreview);
    assert(!MoveReplacementGateV1::offlineExport);
    assert(!MoveReplacementGateV1::installedWrite);
    assert(b.browse(d,MoveKey::B));assert(b.page==MovePage::Catalog);
    assert(b.browse(d,MoveKey::A));confirmOne(b,d);
    assert(b.browse(d,MoveKey::B));assert(b.page==MovePage::Catalog);
    assert(b.browse(d,MoveKey::A));confirmOne(b,d);confirmTwo(b,d);
    assert(b.browse(d,MoveKey::A)); // A cannot activate final
    assert(b.page==MovePage::ConfirmTwo);
    assert(b.browse(d,MoveKey::B));assert(b.page==MovePage::Catalog);
    assert(b.browse(d,MoveKey::A));confirmOne(b,d);confirmTwo(b,d);
    assert(b.browse(d,MoveKey::X));
    assert(b.page==MovePage::ConfirmTwo); // must release
    b.releaseApproval();
    assert(b.browse(d,MoveKey::X));
    assert(b.page==MovePage::Notice);
    assert(b.notice==MoveNotice::ReplacementBlocked);
    assert(!MoveReplacementGateV1::mayApply(request));
    assert(b.browse(d,MoveKey::B));
    assert(b.page==MovePage::Catalog);

    // Stale money is irrelevant; stale source or party moves/PP cancels.
    assert(b.browse(d,MoveKey::A));assert(b.page==MovePage::Review);
    d.sourceSha256[0]^=1;
    assert(!b.activeParty(d));
    assert(!b.browse(d,MoveKey::A));
    assert(b.page==MovePage::Notice);
    d.sourceSha256[0]^=1;
    assert(b.initialize(catalog,d,0));
    enterReview(b,d);
    d.party[0].moves[0]++;
    assert(!b.activeParty(d));
    assert(!b.browse(d,MoveKey::A));
    d.party[0].moves[0]--;
    assert(b.initialize(catalog,d,0));enterReview(b,d);
    d.party[0].pp[0]++;
    assert(!b.activeParty(d));
    d.party[0].pp[0]--;
    // Explicit catalogue conflicts must not turn into a purported intent.
    assert(b.initialize(catalog,d,0));
    assert(b.browse(d,MoveKey::A));
    unsigned conflictOrdinal=0;
    for(unsigned i=0;i<b.matchCount;i++) {
        unsigned idx=0;assert(b.matchAt(i,idx));
        if(b.catalog->records[idx].evidence==MoveEvidence::ConflictingPP) {
            conflictOrdinal=i;break;
        }
    }
    b.selected=conflictOrdinal;
    assert(b.browse(d,MoveKey::A));
    assert(b.page==MovePage::Review);
    assert(!describeMoveSelection(b,d,request));
    // Repeated openings with no allocation growth / stale approval.
    for(unsigned i=0;i<350;i++){
        assert(b.beginPartySelection(catalog,d));
        assert(b.browse(d,MoveKey::A));
        assert(b.page==MovePage::Slots);
        assert(b.browse(d,MoveKey::B));
        assert(b.page==MovePage::Party);
    }
    std::cout<<"PASS six-party, four-slots, two-stage cancellation, stale fingerprint, fail-closed Apply\n";
}
