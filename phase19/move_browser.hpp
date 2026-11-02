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
enum class MovePage : uint8_t { Slots, Catalog, Keyboard, Review, Notice };
enum class MoveNotice : uint8_t { ReplacementBlocked, EmptySlot, PartyChanged };
enum class MoveKey : uint8_t { Up,Down,Left,Right,A,B,X,Y,R };
struct MoveBrowser {
    const MoveCatalog* catalog=nullptr;
    MovePage page=MovePage::Slots;
    unsigned partySlot=0,slot=0,selected=0;
    unsigned shown=0,matchCount=0;
    unsigned keyboardIndex=0;
    unsigned queryLength=0;
    char query[SearchMax+1]{};
    char pending[SearchMax+1]{};
    std::array<uint16_t,MaxMoveRecords> alphabetical{}; // 1872 B
    std::array<uint16_t,MaxMoveRecords> matches{};      // 1872 B, O(1) rows
    CatalogProblem problem=CatalogProblem::Missing;
    MoveNotice notice=MoveNotice::ReplacementBlocked;
    bool ready=false;
    void reset() {
        catalog=nullptr;page=MovePage::Slots;partySlot=slot=selected=0;
        shown=matchCount=keyboardIndex=queryLength=0;
        query[0]=pending[0]='\0';ready=false;
        problem=CatalogProblem::Missing;
        notice=MoveNotice::ReplacementBlocked;
        // Large index arrays are overwritten on initialize/recount, not
        // value-initialized in a temporary 4-KiB ARM11 stack object.
    }
    static constexpr char Keyboard[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ -<!";
    bool initialize(const MoveCatalog& source,const Demo& d,unsigned party) {
        reset();
        problem=validateMoveCatalog(source);
        if(problem!=CatalogProblem::None)return false;
        if(party>=d.partyCount || party>=d.party.size() ||
           d.partyStatus!=Availability::Ready || !d.party[party].species) {
            problem=CatalogProblem::BadMove;return false;
        }
        catalog=&source;partySlot=party;
        for(unsigned i=0;i<source.count;i++)
            alphabetical[i]=static_cast<uint16_t>(i);
        std::sort(alphabetical.begin(),alphabetical.begin()+source.count,
                  [&source](uint16_t a,uint16_t b){
            const int cmp=caseCmp(source.records[a].name,source.records[b].name);
            return cmp<0 || (cmp==0 && source.records[a].id<source.records[b].id);
        });
        ready=true;
        problem=CatalogProblem::None;
        recount();
        return true;
    }
    bool activeParty(const Demo& d) const {
        return ready && catalog && catalog->count<=MaxMoveRecords &&
               partySlot<d.partyCount &&
               partySlot<d.party.size() && d.partyStatus==Availability::Ready &&
               d.party[partySlot].species!=0;
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
    bool browse(const Demo& d,MoveKey k) {
        if(!activeParty(d)){
            page=MovePage::Notice;
            notice=MoveNotice::PartyChanged;
            return false;
        }
        switch(page) {
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
                else if(k==MoveKey::B)return false; // caller exits browser
                break;
            case MovePage::Catalog:
                if(k==MoveKey::Up && selected>0)--selected;
                else if(k==MoveKey::Down && selected+1<matchCount)++selected;
                else if(k==MoveKey::R || k==MoveKey::Right)
                    selected=matchCount?std::min(matchCount-1u,selected+BrowserPageRows):0;
                else if(k==MoveKey::X || k==MoveKey::Left)
                    selected=selected>=BrowserPageRows?selected-BrowserPageRows:0;
                else if(k==MoveKey::A && matchCount)page=MovePage::Review;
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
                if(k==MoveKey::A){notice=MoveNotice::ReplacementBlocked;page=MovePage::Notice;} // BLOCKED
                else if(k==MoveKey::B)page=MovePage::Catalog;
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
