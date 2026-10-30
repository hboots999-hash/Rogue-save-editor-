#include "observed_profile.hpp"
#include "candidate_profile.hpp"
#include <algorithm>
namespace rogue {
namespace {
uint16_t r16(const uint8_t*p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
uint32_t r32(const uint8_t*p){return uint32_t(r16(p))|uint32_t(r16(p+2))<<16;}
bool blank(const uint8_t*p,std::size_t n){bool zero=true,erased=true;for(std::size_t i=0;i<n;i++){zero&=p[i]==0;erased&=p[i]==255;}return zero||erased;}
// Canonical type -> physical position, inverse of the reference permutation.
constexpr uint8_t offsets[24][4]={
 {0,1,2,3},{0,1,3,2},{0,2,1,3},{0,3,1,2},{0,2,3,1},{0,3,2,1},
 {1,0,2,3},{1,0,3,2},{2,0,1,3},{3,0,1,2},{2,0,3,1},{3,0,2,1},
 {1,2,0,3},{1,3,0,2},{2,1,0,3},{3,1,0,2},{2,3,0,1},{3,2,0,1},
 {1,2,3,0},{1,3,2,0},{2,1,3,0},{3,1,2,0},{2,3,1,0},{3,2,1,0}};
bool mon(const uint8_t*p,ObservedMon& out){
 // Candidate BoxPokemon flags: reject bad eggs and unpopulated records.
 if((p[0x13]&1u)||!(p[0x13]&2u))return false;
 const uint32_t pid=r32(p),key=pid^r32(p+4);
 std::array<uint8_t,48> plain{};
 for(unsigned i=0;i<48;i+=4){uint32_t x=r32(p+32+i)^key;for(unsigned j=0;j<4;j++)plain[i+j]=uint8_t(x>>(8*j));}
 uint32_t sum=0;for(unsigned i=0;i<48;i+=2)sum+=r16(plain.data()+i);
 out.checksum=uint16_t(sum); if(out.checksum!=r16(p+28))return false;
 const auto* growth=plain.data()+12*offsets[pid%24][0];
 const auto* attacks=plain.data()+12*offsets[pid%24][1];
 const auto* ev=plain.data()+12*offsets[pid%24][2];
 const auto* misc=plain.data()+12*offsets[pid%24][3];
 // These bit layouts differ materially from the user-supplied generic parser.
 const uint32_t g=r32(growth),ivs=r32(misc+4),miscFlags=r32(misc+8);
 out.species=uint16_t(g&0x7ffu);
 out.heldItem=uint16_t(((g>>11)&0x3ffu)|(((g>>26)&1u)<<10));
 out.nature=uint8_t((pid%25)^((p[0x12]>>3)&31));
 out.abilitySlot=uint8_t((miscFlags>>29)&3u);
 out.egg=((ivs>>30)&1u)!=0;
 if(out.egg!=bool(p[0x13]&4u))return false;
 if(!out.species||out.nature>=25||out.abilitySlot>=3)return false;
 for(unsigned i=0;i<6;i++){out.iv[i]=uint8_t((ivs>>(5*i))&31);out.ev[i]=ev[i];}
 for(unsigned i=0;i<4;i++){out.moves[i]=r16(attacks+2*i);out.pp[i]=attacks[8+i];}
 return true;
}
}
ObservedView inspectObserved(const uint8_t* p,std::size_t n){
 ObservedView out;if(!p||n!=ObservedSaveBytes)return out;
 constexpr std::size_t slotBytes=14*SectorBytes;
 for(unsigned i=0;i<28;i++){
  const uint8_t* s=p+i*SectorBytes;auto& d=out.sectors[i];
  d.id=r16(s+0xff4);d.stored=r16(s+0xff6);d.computed=checksum(s,0xff4);
  if(d.id<14){d.candidateLengthKnown=true;d.candidateComputed=checksum(s,kCandidateSectorSizes[d.id]);}
  d.signature=r32(s+0xff8);d.counter=r32(s+0xffc);
 }
 SectorSizes sizes{};sizes.fill(0xff4);
 SlotStatus slots[2]={validateSlot(p,slotBytes,sizes),validateSlot(p+slotBytes,slotBytes,sizes)};
 for(int i=0;i<2;i++)out.validCopy[i]=slots[i].valid;
 if(!slots[0].valid&&!slots[1].valid){out.status=ObservedStatus::NoValidCopy;return out;}
 for(int i=0;i<2;i++)if(!slots[i].valid&&!blank(p+i*slotBytes,slotBytes)){out.status=ObservedStatus::CorruptOtherCopy;return out;}
 int selected=chooseSlot(slots[0],slots[1]);if(selected<0){out.status=ObservedStatus::AmbiguousCopies;return out;}
 for(int i=0;i<2;i++)if(slots[i].valid&&(slots[i].counter&1u)!=unsigned(i)){out.status=ObservedStatus::WrongSlotParity;return out;}
 // An all-0xFF4 checksum can pass when a shorter source-defined checksum
 // fails. Require both interpretations rather than displaying such data.
 for(int i=0;i<2;i++)if(slots[i].valid&&!validateSlot(p+i*slotBytes,slotBytes,kCandidateSectorSizes).valid){out.status=ObservedStatus::ChecksumProfileConflict;return out;}
 out.selected=selected;out.counter=slots[selected].counter;
 const uint8_t* base=p+selected*slotBytes;
 const uint8_t* marker=base+slots[selected].physical[10]*SectorBytes+0xe00;
 constexpr uint8_t markerTail[5]={0,2,0xdf,0x6b,1};
 if((marker[0]!=6&&marker[0]!=8)||!std::equal(marker+1,marker+6,markerTail)){out.status=ObservedStatus::UnsupportedMarker;return out;}
 out.saveVersion=marker[0];
 const uint8_t* s0=base+slots[selected].physical[0]*SectorBytes;
 const uint8_t* s1=base+slots[selected].physical[1]*SectorBytes;
 uint32_t count=r32(s1+0x234);if(count>6){out.status=ObservedStatus::InvalidPartyCount;return out;}
 for(uint32_t i=0;i<count;i++){ObservedMon m;if(!mon(s1+0x238+i*0x68,m)){out.party.clear();out.status=ObservedStatus::InvalidPokemon;return out;}out.party.push_back(m);}
 uint32_t key=r32(s0+0x4c);out.money=r32(s1+0x4a8)^key;
 for(unsigned i=0;i<450;i++){const uint8_t* q=s1+0x578+4*i;uint16_t id=r16(q);if(id)out.items.push_back({uint16_t(i),id,uint16_t(r16(q+2)^uint16_t(key))});}
 out.status=ObservedStatus::Candidate;return out;
}
