#include "rogue_pokemon_edit.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
namespace {
uint16_t r16(const uint8_t*p){return uint16_t(p[0]) | uint16_t(p[1])<<8;}
uint32_t r32(const uint8_t*p){return uint32_t(r16(p)) | uint32_t(r16(p+2))<<16;}
void w16(uint8_t*p,uint16_t x){p[0]=uint8_t(x);p[1]=uint8_t(x>>8);}
void w32(uint8_t*p,uint32_t x){w16(p,uint16_t(x));w16(p+2,uint16_t(x>>16));}
// Canonical block -> physical 12-byte substructure; source-derived 24 permutations.
constexpr uint8_t offsets[24][4]={
 {0,1,2,3},{0,1,3,2},{0,2,1,3},{0,3,1,2},{0,2,3,1},{0,3,2,1},
 {1,0,2,3},{1,0,3,2},{2,0,1,3},{3,0,1,2},{2,0,3,1},{3,0,2,1},
 {1,2,0,3},{1,3,0,2},{2,1,0,3},{3,1,0,2},{2,3,0,1},{3,2,0,1},
 {1,2,3,0},{1,3,2,0},{2,1,3,0},{3,1,2,0},{2,3,1,0},{3,2,1,0}
};
bool sameMonExceptStats(const rogue::ObservedMon& a,const rogue::ObservedMon& b){
 return a.species==b.species && a.heldItem==b.heldItem &&
   a.nature==b.nature && a.abilitySlot==b.abilitySlot && a.egg==b.egg &&
   a.moves==b.moves && a.pp==b.pp;
}
}
extern "C" rogue_pokemon_edit_status rogue_make_pokemon_stats_draft(
 const uint8_t* src,size_t n,const rogue_pokemon_stats_request* q,
 uint8_t* out,size_t out_n,size_t* changed){
 if(changed)*changed=0;
 if(!src||!q||!out||n!=rogue::ObservedSaveBytes||out_n!=n)return ROGUE_MON_EDIT_BAD_ARGS;
 const uintptr_t a=reinterpret_cast<uintptr_t>(src),b=reinterpret_cast<uintptr_t>(out);
 if(a<=b+n && b<=a+n)return ROGUE_MON_EDIT_BAD_ARGS;
 if(q->change_ivs>1||q->change_evs>1||(!q->change_ivs&&!q->change_evs))return ROGUE_MON_EDIT_BAD_ARGS;
 if(q->change_ivs)for(auto v:q->ivs)if(v>31)return ROGUE_MON_EDIT_BAD_STATS;
 if(q->change_evs){unsigned sum=0;for(auto v:q->evs){if(v>252)return ROGUE_MON_EDIT_BAD_STATS;sum+=v;}if(sum>510)return ROGUE_MON_EDIT_BAD_STATS;}
 try {
  const auto before=rogue::inspectObserved(src,n);
  if(before.status!=rogue::ObservedStatus::Candidate)return ROGUE_MON_EDIT_BAD_SOURCE;
  const auto profile=before.saveVersion==6?rogue::inspectCandidate212(src,n):rogue::inspectCandidate(src,n);
  if(profile.status!=rogue::CandidateStatus::CandidateOnly)return ROGUE_MON_EDIT_BAD_SOURCE;
  if(q->party_index>=before.party.size())return ROGUE_MON_EDIT_STALE_PARTY;
  const std::size_t base=std::size_t(before.selected)*14*rogue::SectorBytes;
  const auto sector=rogue::validateSlot(src+base,14*rogue::SectorBytes,rogue::kCandidateSectorSizes);
  if(!sector.valid)return ROGUE_MON_EDIT_BAD_SOURCE;
  const std::size_t sectorStart=base+sector.physical[1]*rogue::SectorBytes;
  const std::size_t monStart=sectorStart+0x238+std::size_t(q->party_index)*0x68;
  const uint8_t* mon=src+monStart;
  const uint32_t pid=r32(mon),ot=r32(mon+4),key=pid^ot;
  if(pid!=q->expected_personality||before.party[q->party_index].species!=q->expected_species || !q->expected_species)
   return ROGUE_MON_EDIT_STALE_PARTY;
  std::array<uint8_t,48> plain{};
  for(std::size_t i=0;i<48;i+=4)w32(plain.data()+i,r32(mon+32+i)^key);
  uint32_t csum=0;for(std::size_t i=0;i<48;i+=2)csum+=r16(plain.data()+i);
  if(uint16_t(csum)!=r16(mon+28))return ROGUE_MON_EDIT_BAD_SOURCE;
  uint8_t* ev=plain.data()+12*offsets[pid%24][2];
  uint8_t* misc=plain.data()+12*offsets[pid%24][3];
  if(q->change_evs)std::copy(q->evs,q->evs+6,ev);
  if(q->change_ivs){
   // Bits 30 and 31 encode egg and gigantamax flags. Preserve them.
   uint32_t ivword=r32(misc+4)&0xC0000000u;
   for(unsigned i=0;i<6;i++)ivword|=uint32_t(q->ivs[i])<<(5*i);
   w32(misc+4,ivword);
  }
  std::vector<uint8_t> bytes(src,src+n);
  uint8_t* draft=bytes.data()+monStart;
  uint32_t newsum=0;for(std::size_t i=0;i<48;i+=2)newsum+=r16(plain.data()+i);
  w16(draft+28,uint16_t(newsum));
  for(std::size_t i=0;i<48;i+=4)w32(draft+32+i,r32(plain.data()+i)^key);
  w16(bytes.data()+sectorStart+0xff6,rogue::checksum(bytes.data()+sectorStart,0xff4));
  const auto after=rogue::inspectObserved(bytes.data(),bytes.size());
  if(after.status!=rogue::ObservedStatus::Candidate || after.selected!=before.selected ||
    after.counter!=before.counter || after.saveVersion!=before.saveVersion ||
    after.money!=before.money || after.items.size()!=before.items.size() ||
    after.party.size()!=before.party.size())return ROGUE_MON_EDIT_VERIFY_FAILED;
  for(std::size_t i=0;i<before.party.size();i++){
    if(!sameMonExceptStats(before.party[i],after.party[i]))return ROGUE_MON_EDIT_VERIFY_FAILED;
    if(i!=q->party_index && (before.party[i].iv!=after.party[i].iv || before.party[i].ev!=after.party[i].ev))
      return ROGUE_MON_EDIT_VERIFY_FAILED;
  }
  for(std::size_t i=0;i<before.items.size();i++)if(before.items[i].slot!=after.items[i].slot ||
    before.items[i].id!=after.items[i].id||before.items[i].quantity!=after.items[i].quantity)
      return ROGUE_MON_EDIT_VERIFY_FAILED;
  const auto& target=after.party[q->party_index];
  if(q->change_ivs && !std::equal(target.iv.begin(),target.iv.end(),q->ivs))return ROGUE_MON_EDIT_VERIFY_FAILED;
  if(q->change_evs && !std::equal(target.ev.begin(),target.ev.end(),q->evs))return ROGUE_MON_EDIT_VERIFY_FAILED;
  if(!q->change_ivs && target.iv!=before.party[q->party_index].iv)return ROGUE_MON_EDIT_VERIFY_FAILED;
  if(!q->change_evs && target.ev!=before.party[q->party_index].ev)return ROGUE_MON_EDIT_VERIFY_FAILED;
  std::size_t count=0;
  for(std::size_t i=0;i<n;i++)if(bytes[i]!=src[i]){
    if(!((i>=monStart+28 && i<monStart+80) || i==sectorStart+0xff6 || i==sectorStart+0xff7))
      return ROGUE_MON_EDIT_VERIFY_FAILED;
    ++count;
  }
  if(!count)return ROGUE_MON_EDIT_NO_CHANGE;
  std::memcpy(out,bytes.data(),n);
  if(changed)*changed=count;
  return ROGUE_MON_EDIT_OK;
 } catch(...){return ROGUE_MON_EDIT_VERIFY_FAILED;}
}
