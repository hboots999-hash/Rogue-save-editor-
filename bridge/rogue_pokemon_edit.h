/* Candidate-format party Pokemon IV/EV offline draft API.
 * No live game save access or filesystem operations.
 * Species/moves/other Pokemon fields remain unchanged.
 */
#ifndef ROGUE_POKEMON_EDIT_H
#define ROGUE_POKEMON_EDIT_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct rogue_pokemon_stats_request {
    uint8_t party_index;
    uint16_t expected_species;
    uint32_t expected_personality;
    uint8_t change_ivs;
    uint8_t change_evs;
    uint8_t ivs[6]; /* HP, Atk, Def, Speed, SpAtk, SpDef; 0..31 */
    uint8_t evs[6]; /* same order; <=252 each, sum<=510 */
} rogue_pokemon_stats_request;
typedef enum rogue_pokemon_edit_status {
    ROGUE_MON_EDIT_OK = 0,
    ROGUE_MON_EDIT_BAD_ARGS,
    ROGUE_MON_EDIT_BAD_SOURCE,
    ROGUE_MON_EDIT_STALE_PARTY,
    ROGUE_MON_EDIT_BAD_STATS,
    ROGUE_MON_EDIT_NO_CHANGE,
    ROGUE_MON_EDIT_VERIFY_FAILED
} rogue_pokemon_edit_status;
/* Creates a complete independent 131072-byte draft, never in-place.
 * Does NOT write the game or any file; output untouched on error.
 */
rogue_pokemon_edit_status rogue_make_pokemon_stats_draft(
    const uint8_t* source,size_t source_size,
    const rogue_pokemon_stats_request* request,
    uint8_t* output,size_t output_size,size_t* changed_bytes);
#ifdef __cplusplus
}
#endif
#endif
