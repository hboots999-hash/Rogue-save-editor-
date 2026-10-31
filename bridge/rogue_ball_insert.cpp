#include "rogue_ball_insert.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
namespace {
uint16_t r16(const uint8_t* p) {return uint16_t(p[0])|uint16_t(p[1])<<8;}
uint32_t r32(const uint8_t* p) {return uint32_t(r16(p))|uint32_t(r16(p+2))<<16;}
void w16(uint8_t* p,uint16_t v) {p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
bool isBall(uint16_t id) {return id>=1 && id<=28;}
}
extern "C" rogue_ball_insert_status rogue_make_master_ball_draft(
 const uint8_t* src,size_t n,uint8_t* out,size_t out_n,size_t* changed)
{
 if(changed)*changed=0;
 if(!src||!out||n!=rogue::ObservedSaveBytes||out_n!=n)
  return ROGUE_BALL_INSERT_BAD_ARGUMENT;
 const uintptr_t a=reinterpret_cast<uintptr_t>(src),b=reinterpret_cast<uintptr_t>(out);
 if(a<=b+out_n && b<=a+n)return ROGUE_BALL_INSERT_BAD_ARGUMENT;
 try {
  const auto before=rogue::inspectObserved(src,n);
  if(before.status!=rogue::ObservedStatus::Candidate)
   return ROGUE_BALL_INSERT_BAD_SOURCE;
  if(before.saveVersion!=6)return ROGUE_BALL_INSERT_UNSUPPORTED_VERSION;
  const auto base=std::size_t(before.selected)*14*rogue::SectorBytes;
  const auto slot=rogue::validateSlot(src+base,14*rogue::SectorBytes,
      rogue::kCandidateSectorSizes);
  if(!slot.valid)return ROGUE_BALL_INSERT_BAD_SOURCE;
  const auto s0=base+slot.physical[0]*rogue::SectorBytes;
  const auto s1=base+slot.physical[1]*rogue::SectorBytes;
  // EX 2.1.2 SaveBlock1 candidate has 0x18 byte shift from pinned source.
  const unsigned sortMode=src[s1+0x4ac],upgrades=src[s1+0x4ad];
  if(sortMode!=0||upgrades>37)return ROGUE_BALL_INSERT_BAG_UNSAFE;
  const std::size_t bag=s1+0x578,count=before.items.size();
  // Pinned game formula: capacity=80+10*upgrade (max 450).
  // Conservatively charge ALL occupied items against the non-key-item
  // capacity, reserving 50 slots even if no key items are present.
  const std::size_t capacity=80+10*upgrades;
  if(count>=450||count+1>capacity-50)return ROGUE_BALL_INSERT_NO_CAPACITY;
  for(std::size_t i=0;i<450;i++){
   const uint16_t id=r16(src+bag+i*4);
   if(i<count){
    if(!id||before.items[i].slot!=i||before.items[i].id!=id)
     return ROGUE_BALL_INSERT_BAG_UNSAFE;
   }else if(id)return ROGUE_BALL_INSERT_BAG_UNSAFE;
  }
  std::size_t first=450,last=0;
  bool seenBall=false,closed=false;
  for(std::size_t i=0;i<count;i++){
   uint16_t id=before.items[i].id;
   if(id==4)return ROGUE_BALL_INSERT_ALREADY_OWNED;
   if(isBall(id)){
    if(closed)return ROGUE_BALL_INSERT_BAG_UNSAFE;
    seenBall=true;
    if(first==450)first=i;
    last=i+1;
   }else if(seenBall)closed=true;
  }
  if(!seenBall||first==last)return ROGUE_BALL_INSERT_BAG_UNSAFE;
  // Source default type sort: basic balls ID1/2/3 have score 0/1/2,
  // Master Ball ID4 score 3; all other balls have score >=10.
  std::size_t insert=first;
  uint16_t lastBasic=0;
  bool seenSpecial=false;
  for(std::size_t i=first;i<last;i++){
   uint16_t id=before.items[i].id;
   if(id<=3){
    if(seenSpecial||id<lastBasic)return ROGUE_BALL_INSERT_BAG_UNSAFE;
    lastBasic=id;insert=i+1;
   }else seenSpecial=true;
  }
  std::vector<uint8_t> draft(src,src+n);
  uint8_t* bptr=draft.data()+bag;
  std::memmove(bptr+(insert+1)*4,bptr+insert*4,(count-insert)*4);
  w16(bptr+insert*4,4);
  w16(bptr+insert*4+2,uint16_t(999u^uint16_t(r32(src+s0+0x4c))));
  w16(draft.data()+s1+0xff6,rogue::checksum(draft.data()+s1,0xff4));
  const auto after=rogue::inspectObserved(draft.data(),draft.size());
  if(after.status!=rogue::ObservedStatus::Candidate ||
     after.selected!=before.selected || after.counter!=before.counter ||
     after.saveVersion!=before.saveVersion || after.money!=before.money ||
     after.party.size()!=before.party.size() ||
     after.items.size()!=before.items.size()+1)return ROGUE_BALL_INSERT_VERIFY_FAILED;
  for(std::size_t i=0;i<before.party.size();i++){
   const auto& x=before.party[i];const auto& y=after.party[i];
   if(x.species!=y.species||x.heldItem!=y.heldItem||x.moves!=y.moves||
      x.iv!=y.iv||x.ev!=y.ev||x.nature!=y.nature||x.abilitySlot!=y.abilitySlot)
    return ROGUE_BALL_INSERT_VERIFY_FAILED;
  }
  for(std::size_t i=0;i<after.items.size();i++){
   const auto& item=after.items[i];
   if(item.slot!=i)return ROGUE_BALL_INSERT_VERIFY_FAILED;
   if(i==insert){
    if(item.id!=4||item.quantity!=999)return ROGUE_BALL_INSERT_VERIFY_FAILED;
   }else{
    const auto& expected=before.items[i<insert?i:i-1];
    if(item.id!=expected.id||item.quantity!=expected.quantity)
     return ROGUE_BALL_INSERT_VERIFY_FAILED;
   }
  }
  std::size_t countChanges=0;
  for(std::size_t i=0;i<n;i++)if(src[i]!=draft[i]){
   if(!((i>=bag+insert*4&&i<bag+(count+1)*4)||
      i==s1+0xff6||i==s1+0xff7))return ROGUE_BALL_INSERT_VERIFY_FAILED;
   ++countChanges;
  }
  if(!countChanges)return ROGUE_BALL_INSERT_NO_CHANGES;
  std::memcpy(out,draft.data(),n);
  if(changed)*changed=countChanges;
  return ROGUE_BALL_INSERT_OK;
 }catch(...){return ROGUE_BALL_INSERT_VERIFY_FAILED;}
}
