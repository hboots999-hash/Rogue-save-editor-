#include "edit_draft.hpp"
#include "candidate_profile.hpp"
#include <algorithm>
#include <array>
namespace rogue {
namespace {
uint16_t r16(const uint8_t*p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t r32(const uint8_t*p){return uint32_t(r16(p))|(uint32_t(r16(p+2))<<16);}
void w16(uint8_t*p,uint16_t n){p[0]=uint8_t(n);p[1]=uint8_t(n>>8);}
void w32(uint8_t*p,uint32_t n){w16(p,uint16_t(n));w16(p+2,uint16_t(n>>16));}
}
EditDraft prepareEditDraft(const uint8_t* source,std::size_t length,const EditRequest& request){
 EditDraft result;
 const auto before=inspectObserved(source,length);
 if(before.status!=ObservedStatus::Candidate)return result;
 if(request.changeMoney&&request.money>999999){result.status=DraftStatus::InvalidMoney;return result;}
 std::array<bool,450> seen{};
 const std::size_t base=std::size_t(before.selected)*14*SectorBytes;
 const auto slot=validateSlot(source+base,14*SectorBytes,kCandidateSectorSizes);
 if(!slot.valid)return result;
 const auto s0=base+slot.physical[0]*SectorBytes;
 const auto s1=base+slot.physical[1]*SectorBytes;
 const uint32_t key=r32(source+s0+0x4c);
 for(const auto& q:request.quantities){
  if(q.slot>=450||q.quantity<1||q.quantity>999||!q.expectedId){result.status=DraftStatus::InvalidQuantity;return result;}
  if(seen[q.slot]){result.status=DraftStatus::DuplicateSlot;return result;}
  seen[q.slot]=true;
  if(r16(source+s1+0x578+q.slot*4)!=q.expectedId){result.status=DraftStatus::ItemChanged;return result;}
 }
 std::array<bool,6> partySeen{};
 for(const auto& edit:request.partyCopies){
  if(edit.source>=before.party.size()||edit.destination>=before.party.size()||partySeen[edit.destination]){result.status=DraftStatus::InvalidPartyEdit;return result;}
  partySeen[edit.destination]=true;
 }
 std::vector<uint8_t> bytes(source,source+length);
 std::array<bool,SectorBytes> allowed{};
 for(const auto& edit:request.partyCopies){
  const std::size_t target=0x238+edit.destination*0x68;
  std::copy_n(source+s1+0x238+edit.source*0x68,0x68,bytes.data()+s1+target);
  std::fill(allowed.begin()+target,allowed.begin()+target+0x68,true);
 }
 if(request.changeMoney){w32(bytes.data()+s1+0x4a8,request.money^key);std::fill(allowed.begin()+0x4a8,allowed.begin()+0x4ac,true);}
 for(const auto& q:request.quantities){
  const std::size_t at=0x57a+q.slot*4;
  w16(bytes.data()+s1+at,uint16_t(q.quantity^uint16_t(key)));allowed[at]=allowed[at+1]=true;
 }
 w16(bytes.data()+s1+0xff6,checksum(bytes.data()+s1,0xff4));allowed[0xff6]=allowed[0xff7]=true;
 const auto after=inspectObserved(bytes.data(),bytes.size());
 if(after.status!=ObservedStatus::Candidate||after.selected!=before.selected||after.counter!=before.counter||after.saveVersion!=before.saveVersion||
    (request.changeMoney&&after.money!=request.money)){result.status=DraftStatus::VerificationFailed;return result;}
 for(const auto& q:request.quantities){
  auto i=std::find_if(after.items.begin(),after.items.end(),[&](const ObservedItem& v){return v.slot==q.slot;});
  if(i==after.items.end()||i->id!=q.expectedId||i->quantity!=q.quantity){result.status=DraftStatus::VerificationFailed;return result;}
 }
 for(const auto& edit:request.partyCopies){
  if(!std::equal(bytes.data()+s1+0x238+edit.destination*0x68,bytes.data()+s1+0x238+(edit.destination+1)*0x68,source+s1+0x238+edit.source*0x68)){result.status=DraftStatus::VerificationFailed;return result;}
 }
 for(std::size_t i=0;i<length;i++)if(source[i]!=bytes[i]){
  if(i<s1||i>=s1+SectorBytes||!allowed[i-s1]){result.status=DraftStatus::VerificationFailed;return result;}
  result.changedOffsets.push_back(i);
 }
 if(result.changedOffsets.empty()){result.status=DraftStatus::NoChanges;return result;}
 result.selected=before.selected;result.bytes=std::move(bytes);result.status=DraftStatus::Ready;return result;
}
const char* draftStatusText(DraftStatus s){
 switch(s){
 case DraftStatus::Ready:return "Draft verified; ROM compatibility unverified";
 case DraftStatus::InvalidSave:return "Unsupported or invalid save";
 case DraftStatus::InvalidMoney:return "Money must be 0..999999";
 case DraftStatus::InvalidQuantity:return "Existing stack quantity must be 1..999";
 case DraftStatus::DuplicateSlot:return "Duplicate item-slot edits refused";
 case DraftStatus::ItemChanged:return "Item ID does not match source slot";
 case DraftStatus::InvalidPartyEdit:return "Invalid or duplicate party destination";
 case DraftStatus::NoChanges:return "No changes to export";
 default:return "Draft verification failed";
 }
}
}