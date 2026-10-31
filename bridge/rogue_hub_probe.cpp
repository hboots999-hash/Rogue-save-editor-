#include "rogue_hub_probe.h"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
namespace {
uint16_t rd16(const uint8_t* p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
constexpr size_t kData=0xff4;
constexpr size_t kStart=5*kData;
constexpr size_t kHubAt=(10-5)*kData+0xe00;
bool take16(const std::vector<uint8_t>& buf,size_t& pos,uint16_t& result) {
 if(pos>buf.size()||buf.size()-pos<2)return false;
 result=rd16(buf.data()+pos);pos+=2;return true;
}
bool skip(const std::vector<uint8_t>& buf,size_t& pos,size_t bytes) {
 if(pos>buf.size()||bytes>buf.size()-pos)return false;
 pos+=bytes;return true;
}
}
extern "C" rogue_hub_status rogue_probe_hub(
 const uint8_t* src,size_t n,rogue_hub_report* result)
{
 if(!src||!result||n!=rogue::ObservedSaveBytes)return ROGUE_HUB_BAD_ARGUMENT;
 try {
  const auto v=rogue::inspectObserved(src,n);
  if(v.status!=rogue::ObservedStatus::Candidate)return ROGUE_HUB_BAD_SOURCE;
  if(v.saveVersion!=6)return ROGUE_HUB_WRONG_VERSION;
  const size_t base=size_t(v.selected)*14*rogue::SectorBytes;
  const auto sector=rogue::validateSlot(src+base,14*rogue::SectorBytes,rogue::kCandidateSectorSizes);
  if(!sector.valid)return ROGUE_HUB_BAD_SOURCE;
  std::vector<uint8_t> buf;
  buf.reserve(9*kData);
  for(unsigned i=5;i<14;i++){
    const size_t at=base+sector.physical[i]*rogue::SectorBytes;
    buf.insert(buf.end(),src+at,src+at+kData);
  }
  size_t pos=kHubAt;
  if(!skip(buf,pos,8))return ROGUE_HUB_UNRECOGNIZED_LAYOUT;
  const uint8_t* hdr=buf.data()+kHubAt;
  if(rd16(hdr)!=6 || hdr[2]!=2 || rd16(hdr+3)!=27615 ||
     hdr[5]!=1 || rd16(hdr+6)==0)return ROGUE_HUB_UNRECOGNIZED_LAYOUT;
  uint16_t count=0;
  // An exact, observed EX 2.1.2 candidate profile. These lengths are not
  // assumed safe for other game versions/editions.
  if(!take16(buf,pos,count)||count!=222||!skip(buf,pos,size_t(count)*8))
    return ROGUE_HUB_UNRECOGNIZED_LAYOUT;
  if(!take16(buf,pos,count)||count!=7||!skip(buf,pos,size_t(count)*4))
    return ROGUE_HUB_UNRECOGNIZED_LAYOUT;
  if(!take16(buf,pos,count)||count!=2 || !skip(buf,pos,2))
    return ROGUE_HUB_UNRECOGNIZED_LAYOUT;
  const size_t areaAt=pos-2;
  if(!take16(buf,pos,count)||count!=11)
    return ROGUE_HUB_UNRECOGNIZED_LAYOUT;
  const size_t coordAt=pos;
  // The pinned game struct is Coords8 (2 bytes); the actual candidate
  // sample has 2 zero padding bytes following each coordinate.
  // We support neither interpretation unless upgrade and decoration
  // headers both line up exactly, and 4-byte padding must be zero.
  for(unsigned stride : {2u,4u}) {
    if(stride==4){
      bool padZero=true;
      for(unsigned i=0;i<11;i++){
        const size_t at=coordAt+i*4+2;
        if(at+1>=buf.size()||buf[at]||buf[at+1])padZero=false;
      }
      if(!padZero)continue;
    }
    size_t probe=coordAt;
    uint16_t upgrades=0,decor=0;
    if(!skip(buf,probe,11*stride) || !take16(buf,probe,upgrades) ||
       upgrades!=6)continue;
    const size_t upgradeAt=probe;
    if(!skip(buf,probe,6)||!take16(buf,probe,decor)||decor!=192)continue;
    const uint16_t flags=rd16(buf.data()+areaAt);
    if(flags&0xF800u)continue; // 11 area bits; all reserved bits zero.
    rogue_hub_report found{};
    found.selected_copy=static_cast<uint8_t>(v.selected);
    found.coordinate_stride=static_cast<uint8_t>(stride);
    found.area_flags[0]=buf[areaAt];
    found.area_flags[1]=buf[areaAt+1];
    std::memcpy(found.upgrade_flags,buf.data()+upgradeAt,6);
    for(unsigned i=0;i<11;i++){
      found.area_built[i]=static_cast<uint8_t>((flags>>i)&1);
      found.built_count+=found.area_built[i];
    }
    for(unsigned i=0;i<48;i++){
      found.upgrade_owned[i]=static_cast<uint8_t>((found.upgrade_flags[i/8]>>(i%8))&1);
      found.upgrades_count+=found.upgrade_owned[i];
    }
    *result=found;
    return ROGUE_HUB_OK;
  }
  return ROGUE_HUB_UNRECOGNIZED_LAYOUT;
 } catch(...) {
  return ROGUE_HUB_UNRECOGNIZED_LAYOUT;
 }
}
