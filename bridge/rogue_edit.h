/* Offline candidate-profile draft API. No live game or filesystem access.
 * Origin: RogueSM edit_draft module (MIT), wrapped for PKSM Rogue.
 * Save format is still candidate-only and unverified against installed ROM.
 */
#ifndef PKSM_ROGUE_EDIT_H
#define PKSM_ROGUE_EDIT_H
#include "rogue_bridge.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum rogue_edit_status {
    ROGUE_EDIT_OK = 0,
    ROGUE_EDIT_BAD_ARGUMENT,
    ROGUE_EDIT_BAD_SOURCE,
    ROGUE_EDIT_BAD_MONEY,
    ROGUE_EDIT_BAD_QUANTITY,
    ROGUE_EDIT_STALE_ITEM,
    ROGUE_EDIT_NO_CHANGE,
    ROGUE_EDIT_VERIFY_FAILED
} rogue_edit_status;
typedef struct rogue_draft_request {
    uint8_t change_money;
    uint32_t money; /* 0..999999 */
    uint8_t change_quantity;
    uint16_t item_slot; /* 0..449; existing slot only */
    uint16_t expected_item_id; /* stale ID guard */
    uint16_t quantity; /* 1..999 */
} rogue_draft_request;
/* Independent candidate draft: bytes are never written to game storage.
 * Output remains untouched on failure. Returns number changed if requested.
 * Only money, an existing quantity, and selected-sector checksum may change.
 */
rogue_edit_status rogue_make_offline_draft(const uint8_t* source, size_t n,
    const rogue_draft_request* request, uint8_t* output, size_t out_n,
    size_t* changed_bytes);
#ifdef __cplusplus
}
#endif
#endif