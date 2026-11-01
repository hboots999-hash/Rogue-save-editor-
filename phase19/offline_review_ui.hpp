// Phase19 -> Phase20 portable user-facing review/preflight helper.
// Text/selection validation only; NEVER authorises writes or creates files.
#pragma once
#include "adapter.hpp"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdint>

namespace phase19 {
enum class ReviewField : uint8_t { Money, ExistingQuantity, IV, EV, PP };
struct ReviewTarget {
    ReviewField kind=ReviewField::Money;
    unsigned slot=0,field=0,before=0,after=0;
    unsigned itemId=0,moveId=0;
    unsigned bankCounter=0;
    std::array<uint8_t,32> sourceSha256{};
};
// A valid display review is NOT proof that a draft can be exported.
// Phase20 must independently verify the source SHA, party identity,
// current item/move slot and complete exact-byte candidate before SD I/O.
inline bool reviewDisplayMatches(const Demo& d,const ReviewTarget& t){
    if(d.synthetic || d.sourceModelApi!=1 || d.activeCounter!=t.bankCounter ||
       d.sourceSha256!=t.sourceSha256)return false;
    bool digestPresent=false;
    for(auto byte:d.sourceSha256)if(byte){digestPresent=true;break;}
    if(!digestPresent || t.before==t.after)return false;
    if(d.partyCount>d.party.size() || d.bagCount>d.bag.size())return false;
    switch(t.kind){
        case ReviewField::Money:
            return t.slot==0 && t.field==0 && t.itemId==0 && t.moveId==0 &&
                d.moneyStatus==Availability::Ready &&
                t.before==d.money && t.after<=999999u;
        case ReviewField::ExistingQuantity:
            return t.field==0 && t.slot<d.bagCount &&
                d.bagStatus==Availability::Ready && t.moveId==0 &&
                t.itemId!=0 && d.bag[t.slot].id==t.itemId &&
                t.before==d.bag[t.slot].quantity &&
                t.after>=1 && t.after<=999u;
        case ReviewField::IV:
        case ReviewField::EV:
        case ReviewField::PP:
            break;
    }
    if(t.slot>=d.partyCount || d.partyStatus!=Availability::Ready ||
       d.party[t.slot].egg || t.itemId!=0)return false;
    const auto& mon=d.party[t.slot];
    if(t.kind==ReviewField::PP)
        return t.field<4 && mon.moves[t.field]!=0 &&
               t.moveId==mon.moves[t.field] &&
               t.before==mon.pp[t.field] && t.after<=64u;
    if(t.field>=6 || t.moveId!=0)return false;
    if(t.kind==ReviewField::IV)
        return t.before==mon.iv[t.field] && t.after<=31u;
    if(t.kind==ReviewField::EV){
        unsigned other=0;
        for(unsigned i=0;i<6;i++){
            if(mon.ev[i]>252u)return false;
            if(i!=t.field)other+=mon.ev[i];
        }
        return t.before==mon.ev[t.field] && other<=510u &&
               t.after<=252u && t.after<=510u-other;
    }
    return false;
}
inline bool formatReviewTarget(const Demo& d,const ReviewTarget& t,
                               char* out,size_t cap){
    if(!out || !cap)return false;
    out[0]='\0';
    if(!reviewDisplayMatches(d,t))return false;
    const char* stats[]={"HP","Atk","Def","SpA","SpD","Spe"};
    int n=0;
    switch(t.kind){
        case ReviewField::Money:
            n=std::snprintf(out,cap,"Money");
            break;
        case ReviewField::ExistingQuantity:
            n=std::snprintf(out,cap,"Bag #%u ID%u",t.slot+1u,t.itemId);
            break;
        case ReviewField::IV:
            n=std::snprintf(out,cap,"Party #%u IV %s",t.slot+1u,stats[t.field]);
            break;
        case ReviewField::EV:
            n=std::snprintf(out,cap,"Party #%u EV %s",t.slot+1u,stats[t.field]);
            break;
        case ReviewField::PP:
            n=std::snprintf(out,cap,"Party #%u move #%u PP",t.slot+1u,t.field+1u);
            break;
    }
    if(n<0 || static_cast<size_t>(n)>=cap){
        out[0]='\0';
        return false; // no truncated/misleading target
    }
    return true;
}
// Display-only validation before showing exported filenames. No path or
// arbitrary filename may be presented as a successfully verified draft.
inline bool safeDraftBasename(const char* name){
    if(!name)return false;
    size_t len=0;
    while(name[len] && len<=63u)++len;
    if(len<5u || len>63u)return false;
    if(std::strcmp(name+len-4u,".sav")!=0)return false;
    if(name[0]=='.')return false;
    for(size_t i=0;i<len;i++){
        const char c=name[i];
        const bool safe=(c>='A'&&c<='Z') || (c>='a'&&c<='z') ||
            (c>='0'&&c<='9') || c=='-' || c=='_' || c=='.';
        if(!safe || (c=='.' && i>0 && name[i-1]=='.'))return false;
    }
    return true;
}
} // namespace phase19
