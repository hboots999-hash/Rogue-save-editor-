#pragma once
#include "observed_profile.hpp"
#include <string>
#include <vector>
namespace rogue {
struct QuantityEdit { uint16_t slot=0, expectedId=0, quantity=0; };
struct PartyCopy { uint8_t destination=0, source=0; };
struct EditRequest { std::vector<PartyCopy> partyCopies; bool changeMoney=false; uint32_t money=0; std::vector<QuantityEdit> quantities; };
enum class DraftStatus { Ready, InvalidSave, InvalidMoney, InvalidQuantity, DuplicateSlot, ItemChanged, NoChanges, VerificationFailed, InvalidPartyEdit };
struct EditDraft {
 DraftStatus status=DraftStatus::InvalidSave;
 std::vector<uint8_t> bytes;
 std::vector<std::size_t> changedOffsets;
 int selected=-1;
};
// Produces independent candidate-profile bytes; never accesses a live save.
// Selected-copy money, quantities, explicit full party-record copies and checksum may change.
EditDraft prepareEditDraft(const uint8_t* source,std::size_t length,const EditRequest& request);
const char* draftStatusText(DraftStatus status);
bool readRawSave(const char* path,std::vector<uint8_t>& bytes,std::string& error);
bool exportRawSave(const char* path,const std::vector<uint8_t>& bytes,std::string& error);
}