#include "candidate_profile.hpp"
namespace rogue {
static bool blankSlot(const uint8_t* p, std::size_t n) {
 bool zero=true, erased=true;
 for(std::size_t i=0;i<n;i++) { zero = zero && p[i]==0; erased = erased && p[i]==0xff; }
 return zero || erased;
}
static CandidateResult inspectWithVersion(const uint8_t* flash, std::size_t length, uint8_t version) {
 CandidateResult out;
 if (!flash || length!=kFlashSize) return out;
 constexpr std::size_t slotSize=SectorBytes*SectorsPerSlot;
 SlotStatus slots[2]={validateSlot(flash,slotSize,kCandidateSectorSizes),
                      validateSlot(flash+slotSize,slotSize,kCandidateSectorSizes)};
 out.slotValid[0]=slots[0].valid; out.slotValid[1]=slots[1].valid;
 if (!slots[0].valid && !slots[1].valid) { out.status=CandidateStatus::NoValidSlot; return out; }
 for(int i=0;i<2;i++) { if(!slots[i].valid && !blankSlot(flash+i*slotSize,slotSize)) { out.status=CandidateStatus::CorruptOtherSlot; return out; } }
 int selected=chooseSlot(slots[0],slots[1]);
 if (selected<0) { out.status=CandidateStatus::AmbiguousSlots; return out; }
 // The candidate game writes/loads slot (saveCounter % 2). Refuse a valid
 // checksum set in the wrong physical slot instead of guessing.
 for(int i=0;i<2;i++) { if(slots[i].valid && (slots[i].counter & 1u)!=unsigned(i)) { out.status=CandidateStatus::SlotParityMismatch; return out; } }
 out.selected=selected; out.counter=slots[selected].counter;
 // Candidate compiler layout puts the serialised Rogue header at storage
 // offset 0x5dc4 = data offset 0xe00 in logical save sector 10.
 const uint8_t* sector=flash+selected*slotSize+slots[selected].physical[10]*SectorBytes;
 const uint8_t* p=sector+0xe00;
 // Serialized bytes: LE u16 save version, u8 write format, LE u16 marker,
 // u8 edition. The game version ID is saveVersion + 1.
 if (p[0]!=version || p[1]!=0 || p[2]!=2 || p[3]!=0xdf || p[4]!=0x6b || p[5]!=1) {
  out.status=CandidateStatus::UnsupportedMarker; return out;
 }
 out.status=CandidateStatus::CandidateOnly;
 return out;
}
CandidateResult inspectCandidate(const uint8_t* flash, std::size_t length) { return inspectWithVersion(flash,length,8); }
CandidateResult inspectCandidate212(const uint8_t* flash, std::size_t length) { return inspectWithVersion(flash,length,6); }
}
