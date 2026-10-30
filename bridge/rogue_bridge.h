/* PKSM Rogue Edition candidate-format bridge (READ ONLY).
 * This module has no game-storage access or write API.
 * RogueSM source is MIT licensed; retain original licensing notices.
 */
#ifndef PKSM_ROGUE_BRIDGE_H
#define PKSM_ROGUE_BRIDGE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define ROGUE_RAW_SAVE_SIZE 131072u
#define ROGUE_BRIDGE_API_VERSION 2u

enum rogue_profile {
    ROGUE_UNSUPPORTED = 0,
    ROGUE_CANDIDATE_EX_212 = 1,
    ROGUE_CANDIDATE_EX_221 = 2
};

/* Candidate data is not proof of ROM identity or editing compatibility. */
struct rogue_preview {
    enum rogue_profile profile;
    uint8_t serialized_marker;
    uint8_t has_unverified_pokemon_layout;
    uint32_t money;
    uint32_t save_counter;
    uint16_t party_count;
    uint16_t occupied_bag_slots;
};

struct rogue_party_candidate {
    uint16_t species_id, held_item_id;
    uint16_t moves[4];
    uint8_t ivs[6], evs[6], move_pp[4];
    uint8_t nature_index, ability_slot;
};

struct rogue_item_candidate {
    uint16_t slot_index; /* 0..449 in candidate layout */
    uint16_t item_id;    /* numerical ID: ROM-specific meaning unverified */
    uint16_t quantity;
};

/* Opaque, immutable, independently owned decoded view. No raw bytes retained.
 * May be safely freed after the input buffer is released.
 * No functions exposed by this interface write to save data or the filesystem.
 * Null is returned for malformed, ambiguous or unsupported candidate saves.
 */
struct rogue_snapshot;
struct rogue_snapshot* rogue_snapshot_open(const uint8_t* bytes, size_t n);
void rogue_snapshot_close(struct rogue_snapshot* snapshot);
int rogue_snapshot_summary(const struct rogue_snapshot* snapshot, struct rogue_preview* output);
int rogue_snapshot_party(const struct rogue_snapshot* snapshot, size_t index, struct rogue_party_candidate* output);
int rogue_snapshot_item(const struct rogue_snapshot* snapshot, size_t index, struct rogue_item_candidate* output);

/* Legacy, single-call read-only entry points retained for source compatibility.
 * Multiple calls reparse the entire save; snapshot API is preferred for UI.
 */
int rogue_preview_copy(const uint8_t* bytes, size_t n, struct rogue_preview* output);
int rogue_get_party_candidate(const uint8_t* bytes, size_t n, size_t index, struct rogue_party_candidate* output);
int rogue_get_item_candidate(const uint8_t* bytes, size_t n, size_t index, struct rogue_item_candidate* output);
#ifdef __cplusplus
}
#endif
#endif
