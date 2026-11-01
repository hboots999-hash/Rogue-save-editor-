// SPDX-License-Identifier: GPL-3.0-or-later
// Phase20-compatible public C ABI. NO raw-save parser, restore or write API.
// Safe to include beside Phase18 headers: no h168 declarations or C++ types.
#ifndef PKSM_ROGUE_PHASE17_READONLY_STATUS_V1_H
#define PKSM_ROGUE_PHASE17_READONLY_STATUS_V1_H
#include <stdint.h>
#include <stddef.h>

#define P17_READONLY_STATUS_ABI_VERSION 1u
#define P17_READONLY_STATUS_V1_SIZE 80u
#define P17_READONLY_STATUS_NO_WRITE_CAPABILITIES 0u

#ifdef __cplusplus
extern "C" {
#endif

// Values are append-only within ABI v1. Unknown numbers must be rejected.
typedef enum P17ReadOnlyCodeV1 {
    P17_RO_NOT_CHECKED = 0,
    P17_RO_STORAGE_ERROR = 1,
    P17_RO_BACKUP_MISSING = 2,
    P17_RO_JOURNAL_MISSING = 3,
    P17_RO_TERMINAL_RECEIPT_PRESENT = 4,
    P17_RO_JOURNAL_INVALID = 5,
    P17_RO_BACKUP_MISMATCH = 6,
    P17_RO_LIVE_CHANGED = 7,
    P17_RO_AUTHENTICATION_FAILED = 8,
    P17_RO_RAM_REHEARSAL_READABLE = 9
} P17ReadOnlyCodeV1;

// Fixed 80-byte value-only envelope; no pointers, file handles, callbacks,
// native service access or permission flags. All numeric fields are C types.
// A positive status confirms a RAM-rehearsal *input* only, NOT game validity,
// editability, live restoration, recovery durability or ROM identity.
typedef struct P17ReadOnlyStatusV1 {
    uint32_t abi_version;       // P17_READONLY_STATUS_ABI_VERSION
    uint32_t struct_size;       // P17_READONLY_STATUS_V1_SIZE
    uint32_t code;              // P17ReadOnlyCodeV1
    uint32_t capability_bits;   // ALWAYS ZERO; nonzero is an invalid envelope
    uint64_t expected_title;    // Only the expected title identifier
    uint32_t source_counter;    // Verified GBA VC wrapper counter or 0
    uint32_t container_bytes;   // Verified exact backup length or 0
    uint32_t journal_bytes;     // Verified exact journal length or 0
    uint32_t reserved0;         // Zero, no claims about ROM or backup recency
    uint8_t source_sha256[32];  // SHA-256 of verified full container or zero
    uint8_t reserved1[8];       // All zero, reserved for ABI future
} P17ReadOnlyStatusV1;

// Pure data shape check only; never reads files and NEVER authorizes writes.
// Returns 1 for a structurally recognized envelope, 0 otherwise.
int p17_readonly_status_shape_valid_v1(const P17ReadOnlyStatusV1* status);
// Creates a default explicitly blocked report; 1 on success.
int p17_readonly_status_init_blocked_v1(P17ReadOnlyStatusV1* out);

#ifdef __cplusplus
}
#endif
#endif
