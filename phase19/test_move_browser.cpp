// Phase19 936-record portable move-browser tests. Synthetic IDs ONLY.
// No game ROM, metadata CSV, offline edit engine, SD writing or restore.
#include "move_browser.hpp"
#include "screen_text.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
using namespace phase19;
static const MoveRecord* find(const MoveBrowser& b,const char* text) {
    for(unsigned i=0;i<b.matchCount;i++){
        unsigned row=0;
        assert(b.matchAt(i,row));
        if(std::strcmp(b.catalog->records[row].name,text)==0)
            return &b.catalog->records[row];
    }
    return nullptr;
}
int main() {
    static Demo d{};
    populateMock(d);
    static MoveBrowser browser{}; // large index buffer must never be on ARM stack
    static_assert(sizeof(MoveBrowser)<4096,"browser must fit small bounded state");
    const auto& sample=syntheticMoveCatalog();
    assert(validMoveCatalog(sample));
    assert(browser.initialize(sample,d,0));
    assert(browser.page==MovePage::Slots && browser.slot==0);
    assert(browser.browse(d,MoveKey::Down));
    assert(browser.slot==1);
    assert(browser.browse(d,MoveKey::A));
    assert(browser.page==MovePage::Catalog);
    assert(browser.matchCount==sample.count);
    assert(find(browser,"Fiery Dance"));
    assert(browser.browse(d,MoveKey::Y));
    assert(browser.page==MovePage::Keyboard);
    const char* phrase="FIERY DANCE";
    for(unsigned i=0;phrase[i];i++){
        const char* index=std::strchr(MoveBrowser::Keyboard,phrase[i]);
        assert(index);
        const unsigned at=static_cast<unsigned>(index-MoveBrowser::Keyboard);
        // Exercise D-pad movement along keyboard, no L or touch required.
        while(browser.keyboardIndex/6u < at/6u)
            assert(browser.browse(d,MoveKey::Down));
        while(browser.keyboardIndex/6u > at/6u)
            assert(browser.browse(d,MoveKey::Up));
        while(browser.keyboardIndex%6u < at%6u)
            assert(browser.browse(d,MoveKey::Right));
        while(browser.keyboardIndex%6u > at%6u)
            assert(browser.browse(d,MoveKey::Left));
        assert(browser.keyboardIndex==at);
        assert(browser.browse(d,MoveKey::A));
    }
    assert(std::strcmp(browser.pending,"FIERY DANCE")==0);
    assert(browser.browse(d,MoveKey::Y)); // apply search, not any save
    assert(browser.page==MovePage::Catalog);
    assert(browser.matchCount==1);
    assert(std::strcmp(browser.selectedMove()->name,"Fiery Dance")==0);
    assert(browser.selectedMove()->id==9001); // deliberately fictional ID
    assert(browser.browse(d,MoveKey::A));
    assert(browser.page==MovePage::Review);
    assert(browser.browse(d,MoveKey::A));
    assert(browser.page==MovePage::Notice); // no approval/export
    assert(browser.browse(d,MoveKey::B));
    assert(browser.page==MovePage::Catalog);
    assert(browser.browse(d,MoveKey::Y));
    assert(browser.page==MovePage::Keyboard);
    assert(browser.browse(d,MoveKey::X));
    assert(std::strcmp(browser.pending,"FIERY DANC")==0);
    assert(browser.browse(d,MoveKey::B)); // cancel pending query
    assert(browser.page==MovePage::Catalog);
    assert(std::strcmp(browser.query,"FIERY DANCE")==0);
    // Clear search using keyboard '<', Y accepts. Empty query => all.
    assert(browser.browse(d,MoveKey::Y));
    browser.keyboardIndex=28u; // '<'
    for(unsigned i=0;i<11;i++)assert(browser.browse(d,MoveKey::A));
    assert(browser.queryLength==0);
    assert(browser.browse(d,MoveKey::Y));
    assert(browser.matchCount==sample.count);
    assert(browser.browse(d,MoveKey::R));
    assert(browser.selected==BrowserPageRows);
    assert(browser.browse(d,MoveKey::X));
    assert(browser.selected==0);
    assert(browser.browse(d,MoveKey::Right));
    assert(browser.selected==BrowserPageRows);
    assert(browser.browse(d,MoveKey::Left));
    assert(browser.selected==0);
    assert(browser.browse(d,MoveKey::B));
    assert(browser.page==MovePage::Slots);
    assert(!browser.browse(d,MoveKey::B)); // exits via caller

    // Large synthetic dataset: 936 indexes incl 807 matching PP, 40 conflicts,
    // 89 dummy/unknown/special; all remain READ ONLY in production.
    static std::array<MoveRecord,MaxMoveRecords> all{};
    for(unsigned i=0;i<MaxMoveRecords;i++){
        all[i].id=static_cast<uint16_t>(i+1);
        all[i].basePP=i<807?10:i<847?20:0;
        all[i].evidence=i<807?MoveEvidence::Candidate:
            i<847?MoveEvidence::ConflictingPP:
            i<877?MoveEvidence::Special:i<907?MoveEvidence::Dummy:MoveEvidence::Unknown;
        std::snprintf(all[i].name,sizeof(all[i].name),
                      "Research Move %04u",MaxMoveRecords-i);
    }
    MoveCatalog full{MoveCatalogApi,CatalogOrigin::Unauthenticated,MaxMoveRecords,
                     all.data(),{}};
    assert(validMoveCatalog(full));
    assert(browser.initialize(full,d,0));
    assert(browser.browse(d,MoveKey::A));
    assert(browser.matchCount==936u);
    unsigned first=0,last=0;
    assert(browser.matchAt(0,first));
    assert(browser.matchAt(935,last));
    assert(first==935 && last==0); // verified alphabetical order, not numeric
    for(unsigned i=0;i<200;i++)assert(browser.browse(d,MoveKey::R));
    assert(browser.selected==935u);
    assert(browser.browse(d,MoveKey::A));
    assert(browser.page==MovePage::Review);
    assert(browser.browse(d,MoveKey::A));
    assert(browser.page==MovePage::Notice);
    assert(!browser.matchAt(936,first));
    unsigned statuses[5]{};
    for(const auto& m:all)statuses[unsigned(m.evidence)]++;
    assert(statuses[0]==807 && statuses[1]==40 && statuses[2]==30 &&
           statuses[3]==30 && statuses[4]==29);
    assert(std::strcmp(evidenceLabel(MoveEvidence::ConflictingPP),
                       "PP CONFLICT - UNVERIFIED")==0);
    assert(std::strcmp(evidenceLabel(MoveEvidence::Special),
                       "SPECIAL - UNAVAILABLE")==0);
    // Source change / no party prevents retaining the old viewer selection.
    d.partyCount=0;
    assert(!browser.browse(d,MoveKey::A));
    assert(browser.page==MovePage::Notice);
    d.partyCount=6; d.partyStatus=Availability::Invalid;
    assert(!browser.activeParty(d));
    d.partyStatus=Availability::Ready;

    // Fail closed on unsupported catalogue versions, missing identifiers,
    // bad PP, duplicate IDs, nonterminated names or non-ASCII control bytes.
    full.version=2;assert(!browser.initialize(full,d,0));full.version=1;
    full.count=937;assert(!validMoveCatalog(full));full.count=936;
    full.records=nullptr;assert(!validMoveCatalog(full));full.records=all.data();
    all[3].id=all[2].id;assert(!validMoveCatalog(full));all[3].id=4;
    all[0].basePP=0;assert(!validMoveCatalog(full));all[0].basePP=10;
    std::memset(all[0].name,'X',sizeof(all[0].name));
    assert(!validMoveCatalog(full));
    std::snprintf(all[0].name,sizeof(all[0].name),"Research Move 0936");
    all[0].name[0]='\n';assert(!validMoveCatalog(full));
    all[0].name[0]='R';assert(validMoveCatalog(full));
    assert(!browser.initialize(full,d,6)); // invalid party index

    // Long synthetic move names must remain available to bounded review.
    std::vector<std::string> rows;
    forEachBoundedLine(sample.records[12].name,15u,3u,
        [&](const char* line,unsigned n){rows.emplace_back(line,n);});
    assert(rows.size()>=2);
    for(const auto& row:rows)assert(row.size()<=15u);
    std::cout<<"PASS Phase19 936-entry alphabetical move browser, 807/40 flags, Fiery Dance keyboard, bounds and READ-ONLY refusal\n";
}
