// Portable Phase19 -> Phase20 UI presentation and input-safety tests.
// Synthetic, non-playable data only. Does not write or access SD/game saves.
#include "offline_edit_presentation.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
using namespace phase19;
static std::vector<std::string> view(const Demo& d,const ReviewTarget& t,bool final) {
    std::vector<std::string> out;
    const bool ok=emitOfflineReview(d,t,final,[&](const char* s){
        out.emplace_back(s);
    });
    assert(ok);
    return out;
}
static bool has(const std::vector<std::string>& v,const std::string& text) {
    for(const auto& s:v)if(s.find(text)!=std::string::npos)return true;
    return false;
}
static ReviewTarget target(const Demo& d,ReviewField kind,unsigned slot,
                           unsigned field,unsigned old,unsigned next,
                           unsigned item=0,unsigned move=0) {
    ReviewTarget t{};
    t.kind=kind;t.slot=slot;t.field=field;
    t.before=old;t.after=next;t.itemId=item;t.moveId=move;
    t.bankCounter=d.activeCounter;t.sourceSha256=d.sourceSha256;
    return t;
}
int main() {
    static Demo d{};
    populateMock(d);
    d.synthetic=false;
    d.sourceModelApi=1;
    d.sourceSha256[0]=0xDA;
    d.sourceSha256[31]=0x9E;
    d.activeCounter=98;
    const auto money=target(d,ReviewField::Money,0,0,d.money,d.money+1);
    const auto item=target(d,ReviewField::ExistingQuantity,0,0,
          d.bag[0].quantity,d.bag[0].quantity+1,d.bag[0].id);
    const auto iv=target(d,ReviewField::IV,0,0,d.party[0].iv[0],
                          d.party[0].iv[0]==31?30:31);
    const auto ev=target(d,ReviewField::EV,0,0,d.party[0].ev[0],
                          d.party[0].ev[0]+1);
    const auto pp=target(d,ReviewField::PP,0,0,d.party[0].pp[0],
                          d.party[0].pp[0]==64?63:64,
                          0,d.party[0].moves[0]);
    for(const auto& t:{money,item,iv,ev,pp}) {
        assert(reviewDisplayMatches(d,t));
        const auto first=view(d,t,false), last=view(d,t,true);
        assert(has(first,"Original: ") && has(first,"New value: "));
        assert(has(last,"Source: imports/00000001.sav"));
        assert(has(last,"Installed game: UNCHANGED"));
        assert(has(last,"One edit per separate SD draft"));
        assert(has(first,"A continues") && has(last,"press X"));
        assert(has(last,"A cannot export"));
        if(t.kind==ReviewField::EV)
            assert(has(first,"EV total: ") && has(last," / 510"));
        if(t.kind==ReviewField::PP)
            assert(has(last,"Existing move ID: "));
        if(t.kind==ReviewField::IV||t.kind==ReviewField::EV||
           t.kind==ReviewField::PP)
            assert(has(first,"Species ID: "));
    }
    const auto ivFields=[&](){
        std::vector<std::string> lines;
        assert(emitOfflineFields(d,ReviewField::IV,0,3,[&](const char* s){
            lines.emplace_back(s);
        }));
        return lines;
    }();
    assert(ivFields.size()==9u && has(ivFields,"> SpA "));
    for(const auto kind:{ReviewField::IV,ReviewField::EV,ReviewField::PP}){
        std::vector<std::string> fields;
        const unsigned n=kind==ReviewField::PP?4u:6u;
        for(unsigned i=0;i<n;i++){
            fields.clear();
            assert(emitOfflineFields(d,kind,0,i,[&](const char* s){
                fields.emplace_back(s);
            }));
            unsigned selected=0;
            for(const auto& s:fields)if(!s.empty() && s[0]=='>')++selected;
            assert(selected==1u);
            if(kind==ReviewField::PP)assert(has(fields,"ID"));
        }
        assert(!emitOfflineFields(d,kind,0,n,[](const char*){}));
        assert(!emitOfflineFields(d,kind,6u,0u,[](const char*){}));
    }
    assert(!emitOfflineFields(d,ReviewField::Money,0u,0u,[](const char*){}));
    const auto base=d;
    d.partyCount=0;
    assert(!emitOfflineFields(d,ReviewField::IV,0,0,[](const char*){}));
    assert(!emitOfflineReview(d,iv,true,[](const char*){}));
    d=base;d.bagCount=0;
    assert(!emitOfflineReview(d,item,false,[](const char*){}));
    d=base;d.party[0].egg=true;
    assert(!emitOfflineFields(d,ReviewField::PP,0,0,[](const char*){}));
    assert(!emitOfflineReview(d,pp,true,[](const char*){}));
    d=base;d.party[0].moves[0]=0;
    assert(!emitOfflineReview(d,pp,true,[](const char*){}));
    d=base;d.sourceSha256[0]^=0x01;
    assert(!emitOfflineReview(d,money,true,[](const char*){}));
    d=base;d.activeCounter++;
    assert(!emitOfflineReview(d,money,true,[](const char*){}));
    d=base;d.synthetic=true;
    assert(!emitOfflineReview(d,money,true,[](const char*){}));
    d=base;

    OfflineResultNames names{};
    names.backup="phase18_original_001.sav";
    names.draft="phase18_draft_001.sav";
    names.exporterReturnedSuccess=true;
    assert(validatedResultNames(names));
    std::vector<std::string> result;
    assert(emitOfflineResult(names,[&](const char* s){result.emplace_back(s);}));
    assert(has(result,"Backup: backups/phase18_original_001.sav"));
    assert(has(result,"Draft: drafts/phase18_draft_001.sav"));
    assert(has(result,"DO NOT RESTORE"));
    assert(has(result,"A/B: return"));
    names.draft="../../unsafe.sav";
    result.clear();
    assert(!emitOfflineResult(names,[&](const char* s){result.emplace_back(s);}));
    assert(has(result,"UNVERIFIED / BLOCKED") && !has(result,"DRAFT CREATED"));
    names.draft="phase18_original_001.sav";
    assert(!validatedResultNames(names));
    names.draft=nullptr;
    assert(!validatedResultNames(names));
    names.draft="phase18_draft_001.sav";
    names.exporterReturnedSuccess=false;
    assert(!validatedResultNames(names));
    names.exporterReturnedSuccess=true;
    names.source="../wrong.sav";
    assert(!validatedResultNames(names));
    names.source="00000001.sav";

    // Approval is a pure UI gate: A advances review; only a subsequent
    // physically distinct X press after release permits final confirmation.
    OfflineApprovalGate g{};
    assert(g.poll(true,false,false,false)==ApprovalEvent::Next);
    for(unsigned i=0;i<1000;i++)
        assert(g.poll(true,false,false,false)==ApprovalEvent::None);
    assert(g.stage==ApprovalStage::FinalReview);
    assert(g.poll(true,true,false,false)==ApprovalEvent::None);
    assert(g.poll(false,true,false,false)==ApprovalEvent::None);
    assert(g.poll(false,false,false,false)==ApprovalEvent::None);
    assert(g.poll(false,true,false,true)==ApprovalEvent::None);
    assert(g.poll(false,false,false,false)==ApprovalEvent::None);
    assert(g.poll(false,true,false,false)==ApprovalEvent::Approved);
    for(unsigned i=0;i<100;i++)
        assert(g.poll(false,true,false,false)==ApprovalEvent::None);
    assert(g.stage==ApprovalStage::Approved);
    g.reset();
    assert(g.poll(true,true,false,false)==ApprovalEvent::None);
    assert(g.poll(false,false,false,false)==ApprovalEvent::None);
    assert(g.poll(true,false,false,false)==ApprovalEvent::Next);
    assert(g.poll(false,false,true,false)==ApprovalEvent::Cancelled);
    assert(g.stage==ApprovalStage::Cancelled);
    g.reset();
    assert(g.poll(false,false,true,false)==ApprovalEvent::Cancelled);
    g.reset();
    assert(g.poll(true,false,false,true)==ApprovalEvent::None); // SELECT reserved
    assert(g.poll(false,false,false,false)==ApprovalEvent::None);
    assert(g.poll(true,false,false,false)==ApprovalEvent::Next);
    assert(g.poll(false,false,false,false)==ApprovalEvent::None);
    assert(g.poll(false,true,false,false)==ApprovalEvent::Approved);
    g.reset();
    assert(g.poll(true,false,false,false)==ApprovalEvent::Next);
    // A new source/import MUST reset the approval state, cancelling intent.
    g.reset();
    assert(g.stage==ApprovalStage::FirstReview);
    assert(g.poll(false,true,false,false)==ApprovalEvent::None);
    assert(g.poll(false,false,false,false)==ApprovalEvent::None);
    assert(g.poll(true,false,false,false)==ApprovalEvent::Next);
    assert(g.poll(false,false,true,false)==ApprovalEvent::Cancelled);

    std::cout<<"PASS shared Phase19 field highlighting, all five offline reviews, two-key edge gate, filenames and stale/invalid state\n";
}
