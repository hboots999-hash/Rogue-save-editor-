#include "rogue_edit.h"
#include "edit_draft.hpp"
#include <algorithm>
#include <cstdint>
extern "C" rogue_edit_status rogue_make_offline_draft(const uint8_t* source, size_t n,
    const rogue_draft_request* request, uint8_t* output, size_t out_n,
    size_t* changed_bytes) {
    if (changed_bytes) *changed_bytes = 0;
    if (!source || !request || !output || n != ROGUE_RAW_SAVE_SIZE ||
        out_n != ROGUE_RAW_SAVE_SIZE) return ROGUE_EDIT_BAD_ARGUMENT;
    const auto src = reinterpret_cast<uintptr_t>(source);
    const auto dst = reinterpret_cast<uintptr_t>(output);
    if (dst <= src + n && src <= dst + out_n) return ROGUE_EDIT_BAD_ARGUMENT;
    if (request->change_money > 1 || request->change_quantity > 1)
        return ROGUE_EDIT_BAD_ARGUMENT;
    if (request->change_money && request->money > 999999)
        return ROGUE_EDIT_BAD_MONEY;
    if (request->change_quantity && (request->item_slot >= 450 ||
        !request->expected_item_id || !request->quantity || request->quantity > 999))
        return ROGUE_EDIT_BAD_QUANTITY;
    if (!request->change_money && !request->change_quantity)
        return ROGUE_EDIT_NO_CHANGE;
    try {
        rogue::EditRequest edit{};
        edit.changeMoney = request->change_money != 0;
        edit.money = request->money;
        if (request->change_quantity)
            edit.quantities.push_back({request->item_slot, request->expected_item_id, request->quantity});
        rogue::EditDraft draft = rogue::prepareEditDraft(source, n, edit);
        switch (draft.status) {
            case rogue::DraftStatus::Ready:
                if (draft.bytes.size() != n || draft.changedOffsets.empty())
                    return ROGUE_EDIT_VERIFY_FAILED;
                std::copy(draft.bytes.begin(), draft.bytes.end(), output);
                if (changed_bytes) *changed_bytes = draft.changedOffsets.size();
                return ROGUE_EDIT_OK;
            case rogue::DraftStatus::InvalidMoney: return ROGUE_EDIT_BAD_MONEY;
            case rogue::DraftStatus::InvalidQuantity: return ROGUE_EDIT_BAD_QUANTITY;
            case rogue::DraftStatus::ItemChanged: return ROGUE_EDIT_STALE_ITEM;
            case rogue::DraftStatus::NoChanges: return ROGUE_EDIT_NO_CHANGE;
            case rogue::DraftStatus::InvalidSave: return ROGUE_EDIT_BAD_SOURCE;
            default: return ROGUE_EDIT_VERIFY_FAILED;
        }
    } catch (...) { return ROGUE_EDIT_VERIFY_FAILED; }
}