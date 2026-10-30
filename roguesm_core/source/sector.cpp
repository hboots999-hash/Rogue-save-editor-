#include "sector.hpp"
namespace rogue {
static uint16_t r16(const uint8_t* p) { return uint16_t(p[0]) | uint16_t(p[1])<<8; }
static uint32_t r32(const uint8_t* p) { return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24; }
uint16_t checksum(const uint8_t* data,std::size_t length) { uint32_t sum=0; for(std::size_t i=0;i+4<=length;i+=4) sum+=r32(data+i); return uint16_t((sum>>16)+sum); }
SlotStatus validateSlot(const uint8_t* data,std::size_t length,const SectorSizes& sizes) {
 SlotStatus out; if(!data || length<SectorBytes*SectorsPerSlot) return out;
 unsigned seen=0; uint32_t counter=0;
 for(std::size_t i=0;i<SectorsPerSlot;i++) {
  const auto* s=data+i*SectorBytes; auto id=r16(s+4084);
  if(id>=SectorsPerSlot || (seen&(1u<<id)) || sizes[id]>SectorData || r32(s+4088)!=0x08012025) return out;
  if(checksum(s,sizes[id])!=r16(s+4086)) return out;
  auto c=r32(s+4092); if(i && c!=counter) return out; counter=c;
  seen|=1u<<id; out.physical[id]=i;
 }
 // Upstream writes logical ID n at (rotation + n) % 14. Distinct and
 // checksummed IDs in an arbitrary permutation are not a normal full save.
 if(seen!=(1u<<SectorsPerSlot)-1) return out;
 for(std::size_t id=0;id<SectorsPerSlot;id++)
  if(out.physical[id]!=(out.physical[0]+id)%SectorsPerSlot) return SlotStatus{};
 out.valid=true; out.counter=counter; return out;
}
int chooseSlot(const SlotStatus& a,const SlotStatus& b) {
 if(!a.valid) return b.valid?1:-1;
 if(!b.valid) return 0;
 // Conservative: only consecutive generations, including rollover, are ordered.
 if(uint32_t(a.counter+1u)==b.counter) return 1;
 if(uint32_t(b.counter+1u)==a.counter) return 0;
 return -1;
}
