// Phase19 shared preview text contract for Phase20; synthetic data only.
// No file system, editor, export or privileged title access.
#include "offline_review_ui.hpp"
#include "screen_text.hpp"
#include <cassert>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
using namespace phase19;
int main(){
    static Demo display{};populateMock(display);
    display.synthetic=false;
    display.sourceModelApi=1;
    display.sourceSha256[0]=0x98;
    display.sourceSha256[31]=0x40;
    display.activeCounter=41;
    const auto fingerprint=display.sourceSha256;
    const auto money=display.money;
    auto make=[&](ReviewField kind,unsigned slot,unsigned field,
                   unsigned original,unsigned proposed,unsigned item=0,unsigned move=0){
        ReviewTarget t{};
        t.kind=kind;t.slot=slot;t.field=field;
        t.before=original;t.after=proposed;
        t.itemId=item;t.moveId=move;
        t.bankCounter=41;t.sourceSha256=fingerprint;
        return t;
    };
    char label[60]{};
    auto m=make(ReviewField::Money,0,0,money,money+1);
    assert(reviewDisplayMatches(display,m));
    assert(formatReviewTarget(display,m,label,sizeof(label)));
    assert(std::strcmp(label,"Money")==0);
    assert(!formatReviewTarget(display,m,label,3u));
    assert(!label[0]); // never show a cut-off or misleading target
    const auto item=display.bag[0];
    auto q=make(ReviewField::ExistingQuantity,0,0,item.quantity,item.quantity+1,item.id);
    assert(reviewDisplayMatches(display,q));
    assert(formatReviewTarget(display,q,label,sizeof(label)));
    assert(std::string(label).find("ID")!=std::string::npos);
    const auto& mon=display.party[0];
    auto iv=make(ReviewField::IV,0,2,mon.iv[2],mon.iv[2]+1);
    auto ev=make(ReviewField::EV,0,0,mon.ev[0],mon.ev[0]+1);
    auto pp=make(ReviewField::PP,0,0,mon.pp[0],mon.pp[0]+1,0,mon.moves[0]);
    for(auto t:{iv,ev,pp}){
        assert(reviewDisplayMatches(display,t));
        assert(formatReviewTarget(display,t,label,sizeof(label)));
        assert(std::string(label).find("Party #1")!=std::string::npos);
    }
    // Empty party, empty bag, absent PP move, and a malformed selected index
    // must never be shown as a valid export target.
    for(auto bad:{make(ReviewField::Money,1,0,money,money+1),
                  make(ReviewField::Money,0,0,money,money),
                  make(ReviewField::Money,0,0,money,1000000),
                  make(ReviewField::ExistingQuantity,450,0,item.quantity,2,item.id),
                  make(ReviewField::ExistingQuantity,0,0,item.quantity,2,item.id+1),
                  make(ReviewField::ExistingQuantity,0,0,item.quantity,0,item.id),
                  make(ReviewField::IV,6,0,0,1),
                  make(ReviewField::IV,0,6,0,1),
                  make(ReviewField::EV,0,6,0,1),
                  make(ReviewField::EV,0,0,mon.ev[0],253),
                  make(ReviewField::PP,0,4,0,1),
                  make(ReviewField::PP,0,2,0,1,0,0),
                  make(ReviewField::PP,0,0,mon.pp[0],1,0,9999)}){
        assert(!reviewDisplayMatches(display,bad));
        assert(!formatReviewTarget(display,bad,label,sizeof(label)));
    }
    const auto copy=display;
    display.partyCount=0;
    assert(!reviewDisplayMatches(display,iv));
    display=copy;
    display.bagCount=0;
    assert(!reviewDisplayMatches(display,q));
    display=copy;
    display.party[0].egg=true;
    assert(!reviewDisplayMatches(display,iv));
    display=copy;
    display.synthetic=true;
    assert(!reviewDisplayMatches(display,m));
    display=copy;
    display.sourceSha256[1]^=0x01;
    assert(!reviewDisplayMatches(display,m));
    display=copy;
    display.activeCounter++;
    assert(!reviewDisplayMatches(display,m));
    display=copy;
    display.sourceModelApi=99;
    assert(!reviewDisplayMatches(display,m));
    display=copy;
    display.sourceSha256.fill(0u);
    m.sourceSha256=display.sourceSha256;
    assert(!reviewDisplayMatches(display,m));
    display=copy;
    display.moneyStatus=Availability::Unverified;
    assert(!reviewDisplayMatches(display,m));
    display=copy;
    display.bagStatus=Availability::Empty;
    assert(!reviewDisplayMatches(display,q));
    display=copy;
    display.partyStatus=Availability::Unsupported;
    assert(!reviewDisplayMatches(display,iv));
    display=copy;
    display.party[0].moves[0]=0;
    assert(!reviewDisplayMatches(display,pp));
    display=copy;
    // EV limit takes sum of *other* fields, not a per-stat shortcut.
    display.party[0].ev={252,252,6,0,0,0};
    auto evBoundary=make(ReviewField::EV,0,3,0,1);
    assert(!reviewDisplayMatches(display,evBoundary));
    display=copy;
    assert(std::memcmp(&display,&copy,sizeof(Demo))==0);

    for(const char* candidate:{
        "phase18_draft_000.sav","phase18_original_999.sav",
        "a.sav","phase18_draft_1234567890123.sav"}){
        assert(safeDraftBasename(candidate));
    }
    for(const char* candidate:{
        nullptr,"",".sav","../bad.sav","one/two.sav","..bad.sav",
        "bad\\file.sav","bad.txt","bad.sav/","white space.sav",
        "phase18_draft_999.sav\nunsafe"}){
        assert(!safeDraftBasename(candidate));
    }
    std::string veryLong(90,'a');veryLong+=".sav";
    assert(!safeDraftBasename(veryLong.c_str()));
    // Overflow of a 3DS console notice must preserve truncation warning.
    std::vector<std::string> lines;
    std::string enormous(900,'E');
    forEachWrappedLineBounded(enormous.c_str(),31u,8u,
        [&](const char* text,unsigned n){lines.emplace_back(text,n);});
    assert(lines.size()==8);
    assert(lines.back()=="[message truncated]");
    for(const auto& row:lines)assert(row.size()<=31);
    lines.clear();
    forEachWrappedLineBounded("short error",31u,8u,
        [&](const char* text,unsigned n){lines.emplace_back(text,n);});
    assert(lines.size()==1 && lines[0]=="short error");
    lines.clear();
    forEachWrappedLineBounded("A\nB\nC",31u,2u,
        [&](const char* text,unsigned n){lines.emplace_back(text,n);});
    assert(lines.size()==2 && lines.back()=="[message truncated]");

    std::cout<<"PASS Phase19 offline review source binding, all five targets, bad indices, filenames, bounded notices\n";
}
