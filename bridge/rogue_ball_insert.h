/* Offline Master Ball insertion for candidate EX 2.1.2. NO LIVE SAVE WRITES.
 * Does not establish game acceptance or identify the patched ROM.
 */
#ifndef ROGUE_BALL_INSERT_H
#define ROGUE_BALL_INSERT_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum rogue_ball_insert_status {
    ROGUE_BALL_INSERT_OK = 0,
    ROGUE_BALL_INSERT_BAD_ARGUMENT,
    ROGUE_BALL_INSERT_BAD_SOURCE,
    ROGUE_BALL_INSERT_UNSUPPORTED_VERSION,
    ROGUE_BALL_INSERT_ALREADY_OWNED,
    ROGUE_BALL_INSERT_BAG_UNSAFE,
    ROGUE_BALL_INSERT_NO_CAPACITY,
    ROGUE_BALL_INSERT_NO_CHANGES,
    ROGUE_BALL_INSERT_VERIFY_FAILED
} rogue_ball_insert_status;
/* Produces a separate 128KiB output; output is UNCHANGED on failure.
 * Only a 999-Master-Ball stack (item 4), into the confirmed Pokeball group.
 * Requires marker 6, default type sorting, contiguous items and spare
 * capacity after a conservative 50-slot reserved-items allowance.
 */
rogue_ball_insert_status rogue_make_master_ball_draft(
    const uint8_t* source, size_t source_size, uint8_t* output,
    size_t output_size, size_t* changed_bytes);
#ifdef __cplusplus
}
#endif
#endif
