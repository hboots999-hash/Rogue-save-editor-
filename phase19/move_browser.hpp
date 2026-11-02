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
inline bool validMoveCatalog(const MoveCatalog& c) {
    if(c.version!=MoveCatalogApi || !c.records || !c.count ||
       c.count>MaxMoveRecords || static_cast<unsigned>(c.origin)>2u)return false;
    // CPU-bounded, no heap: at most 936^2 ID checks once on import.
    for(unsigned i=0;i<c.count;i++){
        const auto& m=c.records[i];
        if(!m.id || !validMoveName(m.name,sizeof(m.name)) ||
           static_cast<unsigned>(m.evidence)>4u)return false;
        if(m.evidence==MoveEvidence::Candidate) {
            if(m.basePP==0 || m.basePP>64)return false;
        }else if(m.basePP>64)return false;
        for(unsigned j=0;j<i;j++)if(c.records[j].id==m.id)return false;
    }
    return true;
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
    std::array<uint16_t,MaxMoveRecords> alphabetical{}; // ~1872 bytes
    bool ready=false;
    static constexpr char Keyboard[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ -<!";
    bool initialize(const MoveCatalog& source,const Demo& d,unsigned party) {
        *this=MoveBrowser{}; // 2KB state: caller must put in static/heap
        if(!validMoveCatalog(source) || party>=d.partyCount ||
           party>=d.party.size() || d.partyStatus!=Availability::Ready ||
           !d.party[party].species)return false;
        catalog=&source;partySlot=party;
        for(unsigned i=0;i<source.count;i++)
            alphabetical[i]=static_cast<uint16_t>(i);
        std::sort(alphabetical.begin(),alphabetical.begin()+source.count,
                  [&source](uint16_t a,uint16_t b){
            const int cmp=caseCmp(source.records[a].name,source.records[b].name);
            return cmp<0 || (cmp==0 && source.records[a].id<source.records[b].id);
        });
        ready=true;
        recount();
        return true;
    }
    bool activeParty(const Demo& d) const {
        return ready && catalog && partySlot<d.partyCount &&
               partySlot<d.party.size() && d.partyStatus==Availability::Ready &&
               d.party[partySlot].species!=0;
    }
    bool matchAt(unsigned ordinal,unsigned& rowIndex) const {
        if(!ready || !catalog || ordinal>=matchCount)return false;
        unsigned seen=0;
        for(unsigned i=0;i<catalog->count;i++){
            const unsigned row=alphabetical[i];
            if(hasSearch(catalog->records[row].name,query)){
                if(seen++==ordinal){rowIndex=row;return true;}
            }
        }
        return false;
    }
    const MoveRecord* selectedMove() const {
        unsigned row=0;
        return matchAt(selected,row)?&catalog->records[row]:nullptr;
    }
    void recount() {
        matchCount=0;
        if(!ready || !catalog)return;
        for(unsigned i=0;i<catalog->count;i++)
            if(hasSearch(catalog->records[alphabetical[i]].name,query))
                ++matchCount;
        if(!matchCount)selected=0;
        else if(selected>=matchCount)selected=matchCount-1;
    }
    bool browse(const Demo& d,MoveKey k) {
        if(!activeParty(d)){
            page=MovePage::Notice;
            return false;
        }
        switch(page) {
            case MovePage::Slots:
                if(k==MoveKey::Up)slot=(slot+3u)%4u;
                else if(k==MoveKey::Down)slot=(slot+1u)%4u;
                else if(k==MoveKey::A){page=MovePage::Catalog;selected=0;recount();}
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
                if(k==MoveKey::A)page=MovePage::Notice; // BLOCKED
                else if(k==MoveKey::B)page=MovePage::Catalog;
                break;
            case MovePage::Notice:
                if(k==MoveKey::B || k==MoveKey::A)page=MovePage::Catalog;
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
