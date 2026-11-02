// Phase19 standalone READ-ONLY move browser, interface v1.
// No ROM bytes, CSV, save-edit request, filesystem, preview or export.
// A future Phase20 provider may pass independently authenticated metadata;
// NO browsing action authorises writing, even with a Verified header.
#pragma once
#include "adapter.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdio>

namespace phase19 {
constexpr unsigned MaxMoveRecords=936;
constexpr unsigned BrowserPageRows=6;
constexpr unsigned SearchMax=18;
constexpr uint32_t MoveCatalogApi=1;
enum class MoveEvidence : uint8_t { Candidate, ConflictingPP, Special, Dummy, Unknown };
enum class CatalogOrigin : uint8_t { Synthetic, Unauthenticated, ExactRomReviewed };
struct MoveRecord {
    uint16_t id=0;
    uint8_t basePP=0;
    MoveEvidence evidence=MoveEvidence::Unknown;
    char name[40]{}; // fixed immutable nul-terminated UTF-8/ASCII display
};
struct MoveCatalog {
    uint32_t version=MoveCatalogApi;
    CatalogOrigin origin=CatalogOrigin::Synthetic;
    unsigned count=0;
    const MoveRecord* records=nullptr; // borrowed immutable model-backed storage
    std::array<uint8_t,32> romSha256{}; // provenance, NOT permission for edits
};
inline const char* moveCatalogSourceLabel(CatalogOrigin origin) {
    switch(origin) {
        case CatalogOrigin::Synthetic:return "DEMO MOVE IDS - NOT ROM DATA";
        case CatalogOrigin::Unauthenticated:return "MOVE METADATA UNVERIFIED";
        case CatalogOrigin::ExactRomReviewed:return "META SHA VERIFIED; ROM NOT";
    }
    return "MOVE METADATA NOT VERIFIED";
}
inline const char* evidenceLabel(MoveEvidence x) {
    switch(x) {
        case MoveEvidence::Candidate:return "PP MATCH - REVIEW ONLY";
        case MoveEvidence::ConflictingPP:return "PP CONFLICT - UNVERIFIED";
        case MoveEvidence::Special:return "SPECIAL - UNAVAILABLE";
        case MoveEvidence::Dummy:return "DUMMY - UNAVAILABLE";
        case MoveEvidence::Unknown:return "UNKNOWN - UNAVAILABLE";
    }
    return "UNKNOWN - UNAVAILABLE";
}
inline bool validMoveName(const char* s,unsigned capacity) {
    if(!s || !capacity)return false;
    unsigned length=0;
    for(;length<capacity && s[length];++length) {
        const unsigned char ch=static_cast<unsigned char>(s[length]);
        // Avoid escaped console control chars and partial UTF-8 glyphs in
        // initial v1. Future names need explicit UTF-8 glyph decoding.
        if(ch<32 || ch>126)return false;
    }
    return length>0 && length<capacity;
}
enum class CatalogProblem : uint8_t {
    None, Missing, ReadFailure, WrongSize, CorruptDigest, WrongRom,
    UnsupportedVersion, BadCount, BadName, BadMove, DuplicateId
};
inline const char* catalogProblemText(CatalogProblem problem) {
    switch(problem){
        case CatalogProblem::None:return "Catalogue metadata ready (browse only).";
        case CatalogProblem::Missing:return "Move metadata missing. Check /3ds/PKSM-Rogue/catalogs/. No moves applied.";
        case CatalogProblem::ReadFailure:return "Could not read move metadata from SD. Check SD card; no moves applied.";
        case CatalogProblem::WrongSize:return "Move metadata has wrong size; expected the reviewed fixed-length format.";
        case CatalogProblem::CorruptDigest:return "Move metadata checksum mismatch. Do not use this catalogue.";
        case CatalogProblem::WrongRom:return "Metadata ROM identity unverified. Install matching reviewed metadata only.";
        case CatalogProblem::UnsupportedVersion:return "Unsupported move catalogue version. Update metadata and app together.";
        case CatalogProblem::BadCount:return "Move catalogue count invalid or exceeds safe 936-entry limit.";
        case CatalogProblem::BadName:return "Move metadata includes empty, invalid or unterminated names.";
        case CatalogProblem::BadMove:return "Move ID, PP or evidence status invalid; catalogue unavailable.";
        case CatalogProblem::DuplicateId:return "Move catalogue has duplicate IDs; browser disabled.";
    }
    return "Move catalogue unverified; no replacements allowed.";
}
inline CatalogProblem validateMoveCatalog(const MoveCatalog& c) {
    if(c.version!=MoveCatalogApi)return CatalogProblem::UnsupportedVersion;
    if(!c.records || !c.count || c.count>MaxMoveRecords ||
       static_cast<unsigned>(c.origin)>2u)return CatalogProblem::BadCount;
    // No heap/large temporary arrays: 936x935/2 comparisons maximum,
    // performed only once on browser initialization.
    for(unsigned i=0;i<c.count;i++){
        const auto& m=c.records[i];
        if(!validMoveName(m.name,sizeof(m.name)))return CatalogProblem::BadName;
        if(!m.id || static_cast<unsigned>(m.evidence)>4u ||
           m.basePP>64 || (m.evidence==MoveEvidence::Candidate && !m.basePP))
            return CatalogProblem::BadMove;
        for(unsigned j=0;j<i;j++)if(c.records[j].id==m.id)
            return CatalogProblem::DuplicateId;
    }
    return CatalogProblem::None;
}
inline bool validMoveCatalog(const MoveCatalog& c) {
    return validateMoveCatalog(c)==CatalogProblem::None;
}
// Exactly 15 column slots: abbreviated alphabetical rows are explicitly
// marked with "..."; the full 39-byte name remains on the review page.
inline void formatMoveShortName(const char* name,char (&out)[16]) {
    std::memset(out,0,sizeof(out));
    if(!name)return;
    unsigned n=0;
    while(name[n] && n<15u)++n;
    if(n<15u){std::memcpy(out,name,n);return;}
    if(!name[15]){std::memcpy(out,name,15);return;}
    std::memcpy(out,name,12);std::memcpy(out+12,"...",3);
}
inline char lowerAscii(char c){
    return c>='A' && c<='Z'?char(c+('a'-'A')):c;
}
inline int caseCmp(const char* a,const char* b){
    while(*a && *b){
        const int delta=int(static_cast<unsigned char>(lowerAscii(*a)))-
                        int(static_cast<unsigned char>(lowerAscii(*b)));
        if(delta)return delta;
        ++a;++b;
    }
    return int(static_cast<unsigned char>(*a))-
           int(static_cast<unsigned char>(*b));
}
inline bool hasSearch(const char* title,const char* query){
    if(!title || !query)return false;
    if(!*query)return true;
    for(unsigned i=0;title[i];++i){
        unsigned j=0;
        while(query[j] && title[i+j] &&
              lowerAscii(query[j])==lowerAscii(title[i+j]))++j;
        if(!query[j])return true;
    }
    return false;
}
enum class MovePage : uint8_t { Slots, Catalog, Keyboard, Review, Notice, Party, ConfirmOne, ConfirmTwo };
enum class MoveNotice : uint8_t { ReplacementBlocked, EmptySlot, PartyChanged, CandidateUnavailable, DuplicateMove, SameMove };
enum class MoveKey : uint8_t { Up,Down,Left,Right,A,B,X,Y,R };
struct MoveBrowser {
    const MoveCatalog* catalog=nullptr;
    MovePage page=MovePage::Slots;
    unsigned partySlot=0,slot=0,selected=0;
    unsigned shown=0,matchCount=0;
    unsigned keyboardIndex=0;
    unsigned queryLength=0;
    unsigned sourceCounter=0,pinnedSpecies=0;
    bool pinnedSynthetic=false;
    std::array<uint8_t,32> sourceFingerprint{};
    std::array<unsigned,4> originalMoves{},originalPP{};
    char query[SearchMax+1]{};
    char pending[SearchMax+1]{};
    std::array<uint16_t,MaxMoveRecords> alphabetical{}; // 1872 B
    std::array<uint16_t,MaxMoveRecords> matches{};      // 1872 B, O(1) rows
    CatalogProblem problem=CatalogProblem::Missing;
    MoveNotice notice=MoveNotice::ReplacementBlocked;
    bool fromPartyPicker=false;
    bool confirmationReleased=false; // a held A/X cannot approve either stage
    bool ready=false;
    void reset() {
        catalog=nullptr;page=MovePage::Slots;partySlot=slot=selected=0;
        shown=matchCount=keyboardIndex=queryLength=0;
        sourceCounter=pinnedSpecies=0;pinnedSynthetic=false;
        sourceFingerprint.fill(0);originalMoves.fill(0);originalPP.fill(0);
        query[0]=pending[0]='\0';ready=false;
        problem=CatalogProblem::Missing;
        notice=MoveNotice::ReplacementBlocked;
        fromPartyPicker=false;confirmationReleased=false;
        // Large index arrays are overwritten on initialize/recount, not
        // value-initialized in a temporary 4-KiB ARM11 stack object.
    }
    static constexpr char Keyboard[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ -<!";
    bool prepareCatalog(const MoveCatalog& source) {
        reset();
        problem=validateMoveCatalog(source);
        if(problem!=CatalogProblem::None)return false;
        catalog=&source;
        for(unsigned i=0;i<source.count;i++)
            alphabetical[i]=static_cast<uint16_t>(i);
        std::sort(alphabetical.begin(),alphabetical.begin()+source.count,
                  [&source](uint16_t a,uint16_t b){
            const int cmp=caseCmp(source.records[a].name,source.records[b].name);
            return cmp<0 || (cmp==0 && source.records[a].id<source.records[b].id);
        });
        ready=true;problem=CatalogProblem::None;recount();
        return true;
    }
    bool selectParty(const Demo& d,unsigned party) {
        if(!ready || party>=6u || party>=d.partyCount ||
           party>=d.party.size() || d.partyStatus!=Availability::Ready ||
           !d.party[party].species || d.party[party].egg)
            return false;
        partySlot=party;slot=0;selected=0;page=MovePage::Slots;
        sourceCounter=d.activeCounter;sourceFingerprint=d.sourceSha256;
        pinnedSynthetic=d.synthetic;pinnedSpecies=d.party[party].species;
        originalMoves=d.party[party].moves;originalPP=d.party[party].pp;
        confirmationReleased=false;
        return true;
    }
    bool beginPartySelection(const MoveCatalog& source,const Demo& d) {
        if(!prepareCatalog(source))return false;
        if(d.partyStatus!=Availability::Ready &&
           d.partyStatus!=Availability::Empty) {
            problem=CatalogProblem::BadMove;reset();return false;
        }
        page=MovePage::Party;fromPartyPicker=true;partySlot=0;
        return true; // empty party is shown with an explicit warning
    }
    bool initialize(const MoveCatalog& source,const Demo& d,unsigned party) {
        if(!prepareCatalog(source))return false;
        if(!selectParty(d,party)) {
            problem=CatalogProblem::BadMove;ready=false;return false;
        }
        return true;
    }
    bool activeParty(const Demo& d) const {
        return ready && catalog && catalog->count<=MaxMoveRecords &&
               partySlot<d.partyCount &&
               partySlot<d.party.size() && d.partyStatus==Availability::Ready &&
               d.party[partySlot].species!=0 && !d.party[partySlot].egg &&
               d.party[partySlot].species==pinnedSpecies &&
               d.party[partySlot].moves==originalMoves &&
               d.party[partySlot].pp==originalPP &&
               d.activeCounter==sourceCounter &&
               d.sourceSha256==sourceFingerprint &&
               d.synthetic==pinnedSynthetic;
    }
    const MoveRecord* existingMove(const Demo& d,unsigned currentSlot) const {
        if(!catalog || currentSlot>=4u || !activeParty(d))return nullptr;
        const unsigned id=d.party[partySlot].moves[currentSlot];
        if(!id)return nullptr;
        for(unsigned i=0;i<catalog->count;i++)
            if(catalog->records[i].id==id)return &catalog->records[i];
        return nullptr; // unknown original ID: show numeric ID, never guess
    }
    bool matchAt(unsigned ordinal,unsigned& rowIndex) const {
        if(!ready || !catalog || ordinal>=matchCount ||
           ordinal>=MaxMoveRecords)return false;
        rowIndex=matches[ordinal];
        return rowIndex<catalog->count;
    }
    const MoveRecord* selectedMove() const {
        unsigned row=0;
        return matchAt(selected,row)?&catalog->records[row]:nullptr;
    }
    void recount() {
        matchCount=0;
        if(!ready || !catalog || catalog->count>MaxMoveRecords)return;
        for(unsigned i=0;i<catalog->count && i<MaxMoveRecords;i++){
            const uint16_t idx=alphabetical[i];
            if(idx>=catalog->count)continue;
            if(hasSearch(catalog->records[idx].name,query)) {
                if(matchCount>=MaxMoveRecords)break;
                matches[matchCount++]=idx;
            }
        }
        if(!matchCount)selected=0;
        else if(selected>=matchCount)selected=matchCount-1u;
    }
    // Non-keyboard frame release callback from native input loop.
    // Never triggers a write or advances a confirmation stage.
    void releaseApproval() {
        if(page==MovePage::Review || page==MovePage::ConfirmOne ||
           page==MovePage::ConfirmTwo)confirmationReleased=true;
    }
    MoveNotice reviewBlocker(const Demo& d) const {
        if(!activeParty(d) || slot>=4u)return MoveNotice::PartyChanged;
        const MoveRecord* candidate=selectedMove();
        if(!candidate || candidate->evidence!=MoveEvidence::Candidate ||
           !candidate->basePP)return MoveNotice::CandidateUnavailable;
        const Pokemon& mon=d.party[partySlot];
        if(candidate->id==mon.moves[slot])return MoveNotice::SameMove;
        for(unsigned i=0;i<4u;i++)
            if(i!=slot && candidate->id==mon.moves[i])
                return MoveNotice::DuplicateMove;
        return MoveNotice::ReplacementBlocked; // candidate only; Apply still off
    }
    bool browse(const Demo& d,MoveKey k) {
        // Clamp accidental/stale browser state before any keyboard access.
        if(slot>=4u){slot=0;page=MovePage::Slots;}
        if(keyboardIndex>=sizeof(Keyboard)-1u)keyboardIndex=0;
        if(queryLength>SearchMax) {
            queryLength=0;pending[0]='\0';page=MovePage::Catalog;
        }
        if(page==MovePage::Party) {
            const unsigned n=d.partyStatus==Availability::Ready?
                 std::min(6u,d.partyCount):0u;
            if(k==MoveKey::Up)partySlot=partySlot?partySlot-1u:(n?n-1u:0u);
            else if(k==MoveKey::Down)partySlot=n?(partySlot+1u)%n:0u;
            else if(k==MoveKey::A)(void)selectParty(d,partySlot);
            else if(k==MoveKey::B)return false;
            return true;
        }
        if(!activeParty(d)){
            page=MovePage::Notice;
            notice=MoveNotice::PartyChanged;
            return false;
        }
        switch(page) {
            case MovePage::Party: return true; // handled before party viability check
            case MovePage::Slots:
                if(k==MoveKey::Up)slot=(slot+3u)%4u;
                else if(k==MoveKey::Down)slot=(slot+1u)%4u;
                else if(k==MoveKey::A) {
                    if(!d.party[partySlot].moves[slot]) {
                        notice=MoveNotice::EmptySlot;page=MovePage::Notice;
                    }else{
                        page=MovePage::Catalog;selected=0;recount();
                    }
                }
                else if(k==MoveKey::B) {
                    if(fromPartyPicker){page=MovePage::Party;return true;}
                    return false; // caller exits browser
                }
                break;
            case MovePage::Catalog:
                if(k==MoveKey::Up && selected>0)--selected;
                else if(k==MoveKey::Down && selected+1<matchCount)++selected;
                else if(k==MoveKey::R || k==MoveKey::Right)
                    selected=matchCount?std::min(matchCount-1u,selected+BrowserPageRows):0;
                else if(k==MoveKey::X || k==MoveKey::Left)
                    selected=selected>=BrowserPageRows?selected-BrowserPageRows:0;
                else if(k==MoveKey::A && matchCount) {
                    page=MovePage::Review;confirmationReleased=false;
                }
                else if(k==MoveKey::Y){
                    std::memcpy(pending,query,sizeof(query));
                    queryLength=static_cast<unsigned>(std::strlen(pending));
                    keyboardIndex=0;page=MovePage::Keyboard;
                }else if(k==MoveKey::B)page=MovePage::Slots;
                break;
            case MovePage::Keyboard: {
                // Six-column, five-row on-screen keyboard; no touch or L key.
                // A append; '<' backspace; '!' apply; X backspace; Y apply.
                const unsigned n=sizeof(Keyboard)-1u;
                if(k==MoveKey::Left && keyboardIndex%6u) --keyboardIndex;
                else if(k==MoveKey::Right && keyboardIndex%6u<5u &&
                        keyboardIndex+1u<n)++keyboardIndex;
                else if(k==MoveKey::Up && keyboardIndex>=6u)keyboardIndex-=6u;
                else if(k==MoveKey::Down && keyboardIndex+6u<n)keyboardIndex+=6u;
                else if(k==MoveKey::X && queryLength>0)pending[--queryLength]='\0';
                else if(k==MoveKey::B){
                    // Discard pending search completely
                    queryLength=static_cast<unsigned>(std::strlen(query));
                    std::memcpy(pending,query,sizeof(query));
                    page=MovePage::Catalog;
                }else if(k==MoveKey::Y ||
                         (k==MoveKey::A && Keyboard[keyboardIndex]=='!')){
                    std::memcpy(query,pending,sizeof(query));
                    selected=0;recount();page=MovePage::Catalog;
                }else if(k==MoveKey::A) {
                    const char c=Keyboard[keyboardIndex];
                    if(c=='<'){if(queryLength)pending[--queryLength]='\0';}
                    else if(queryLength<SearchMax){
                        pending[queryLength++]=c;
                        pending[queryLength]='\0';
                    }
                }
                break;
            }
            case MovePage::Review:
                if(k==MoveKey::A && confirmationReleased) {
                    notice=reviewBlocker(d);
                    if(notice==MoveNotice::ReplacementBlocked) {
                        page=MovePage::ConfirmOne;confirmationReleased=false;
                    }else page=MovePage::Notice;
                } else if(k==MoveKey::B)page=MovePage::Catalog;
                break;
            case MovePage::ConfirmOne:
                if(k==MoveKey::A && confirmationReleased) {
                    page=MovePage::ConfirmTwo;confirmationReleased=false;
                } else if(k==MoveKey::B)page=MovePage::Catalog;
                break;
            case MovePage::ConfirmTwo:
                if(k==MoveKey::X && confirmationReleased) {
                    // Explicitly BLOCKED even after the final UI confirmation.
                    notice=MoveNotice::ReplacementBlocked;page=MovePage::Notice;
                } else if(k==MoveKey::B)page=MovePage::Catalog;
                break;
            case MovePage::Notice:
                if(k==MoveKey::B || k==MoveKey::A)
                    page=notice==MoveNotice::EmptySlot?MovePage::Slots:MovePage::Catalog;
                break;
        }
        return true;
    }
    bool startKeyboard() {
        if(!ready || page!=MovePage::Catalog)return false;
        std::memcpy(pending,query,sizeof(query));
        queryLength=static_cast<unsigned>(std::strlen(pending));
        keyboardIndex=0;page=MovePage::Keyboard;return true;
    }
};

constexpr uint32_t MoveSelectionIntentVersion=1;
struct MoveSelectionIntentV1 {
    uint32_t version=MoveSelectionIntentVersion;
    unsigned partySlot=0, moveSlot=0, species=0;
    unsigned originalMoveId=0, originalPP=0;
    unsigned replacementMoveId=0, replacementBasePP=0;
    uint32_t sourceCounter=0;
    std::array<uint8_t,32> sourceSha256{};
    std::array<uint8_t,32> catalogRomSha256{};
    bool candidatePPMatchesTables=false;
};
// Produces a bounded, read-only description for a separately reviewed
// Phase18 API adapter. Caller cannot turn this data into write authority.
// No save bytes, Pokemon PID/OT/checksum/PP-Ups, or full Phase18 proof exists.
inline bool describeMoveSelection(const MoveBrowser& browser,const Demo& d,
                                  MoveSelectionIntentV1& out) {
    out=MoveSelectionIntentV1{};
    if(!browser.activeParty(d) || browser.slot>=4u || !browser.catalog ||
       (browser.page!=MovePage::Review &&
        browser.page!=MovePage::ConfirmOne &&
        browser.page!=MovePage::ConfirmTwo &&
        browser.page!=MovePage::Notice))return false;
    const MoveRecord* candidate=browser.selectedMove();
    const Pokemon& mon=d.party[browser.partySlot];
    if(!candidate || !mon.moves[browser.slot] ||
       candidate->evidence!=MoveEvidence::Candidate ||
       !candidate->basePP || candidate->id==mon.moves[browser.slot])return false;
    for(unsigned i=0;i<4u;i++)
        if(i!=browser.slot && mon.moves[i]==candidate->id)return false;
    out.partySlot=browser.partySlot;out.moveSlot=browser.slot;
    out.species=mon.species;
    out.originalMoveId=mon.moves[browser.slot];out.originalPP=mon.pp[browser.slot];
    out.replacementMoveId=candidate->id;
    out.replacementBasePP=candidate->basePP;
    out.sourceCounter=d.activeCounter;out.sourceSha256=d.sourceSha256;
    out.catalogRomSha256=browser.catalog->romSha256;
    out.candidatePPMatchesTables=true;
    return true; // descriptor exists, NOT approval to preview or export
}
struct MoveReplacementGateV1 {
    static constexpr uint32_t version=1;
    static constexpr bool productionPreview=false;
    static constexpr bool offlineExport=false;
    static constexpr bool installedWrite=false;
    static bool mayApply(const MoveSelectionIntentV1&) {return false;}
};
static_assert(!MoveReplacementGateV1::productionPreview &&
              !MoveReplacementGateV1::offlineExport &&
              !MoveReplacementGateV1::installedWrite,
              "Phase19 must never enable move edits by itself");
// No-repeat on A/B/X/Y: this gate repeats only D-pad/R navigation.
// Call once per HID frame; pass the highest-priority HELD direction.
struct MoveNavRepeat {
    MoveKey previous=MoveKey::A;
    unsigned frames=0;
    bool active=false;
    void reset(){active=false;frames=0;}
    bool poll(MoveKey k,bool held) {
        if(!held){reset();return false;}
        const bool nav=k==MoveKey::Up || k==MoveKey::Down ||
                       k==MoveKey::Left || k==MoveKey::Right || k==MoveKey::R;
        if(!nav){reset();return false;}
        if(!active || k!=previous) {
            previous=k;active=true;frames=0;return true;
        }
        if(frames<1000u)++frames;
        return frames>=16u && ((frames-16u)%5u)==0;
    }
};
// Fictional catalogue ONLY. IDs are deliberately fictional (9001+) and
// MUST NEVER be used as a mapping to Emerald Rogue 2.1.2's actual ROM.
inline const MoveCatalog& syntheticMoveCatalog() {
    static const MoveRecord records[]={
        {9001,10,MoveEvidence::Candidate,"Fiery Dance"},
        {9002,10,MoveEvidence::Candidate,"Flamethrower"},
        {9003,5, MoveEvidence::Candidate,"Fire Blast"},
        {9004,10,MoveEvidence::Candidate,"Bug Buzz"},
        {9005,20,MoveEvidence::Candidate,"Giga Drain"},
        {9006,15,MoveEvidence::Candidate,"Heat Wave"},
        {9007,20,MoveEvidence::Candidate,"Quiver Dance"},
        {9008,15,MoveEvidence::Candidate,"Psychic"},
        {9009,10,MoveEvidence::ConflictingPP,"Conflicted Test Move"},
        {9010,0, MoveEvidence::Special,"Special Research Move"},
        {9011,0, MoveEvidence::Dummy,"Dummy Placeholder"},
        {9012,0, MoveEvidence::Unknown,"Unknown Move"},
        {9013,10,MoveEvidence::Candidate,"Very Long Synthetic Move Name XYZ"},
        {9014,25,MoveEvidence::Candidate,"Acrobatics"},
        {9015,10,MoveEvidence::Candidate,"Air Slash"},
        {9016,15,MoveEvidence::Candidate,"Ancient Power"},
    };
    static const MoveCatalog c{MoveCatalogApi,CatalogOrigin::Synthetic,
       static_cast<unsigned>(sizeof(records)/sizeof(records[0])),records,{}};
    return c;
}
} // namespace phase19
