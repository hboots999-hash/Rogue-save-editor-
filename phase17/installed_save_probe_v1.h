// SPDX-License-Identifier: GPL-3.0-or-later
// Issue #15: versioned installed GBA VC H168 save *read-only* presentation.
// This C ABI imports NO h168, PKX, Phase18, native service or writer types.
// A report is observational and FORGEABLE; never a save write capability.
#ifndef P17_INSTALLED_SAVE_PROBE_V1_H
#define P17_INSTALLED_SAVE_PROBE_V1_H
#include <stddef.h>
#include <stdint.h>
#define P17_SAVE_PROBE_VERSION 1u
#define P17_SAVE_PROBE_REPORT_SIZE 128u
#define P17_SAVE_PROBE_RAW_BYTES 131072u
#define P17_SAVE_PROBE_CONTAINER_BYTES 263168u
#define P17_SAVE_PROBE_EXPECTED_TITLE UINT64_C(0x0004000007D7BA00)
#define P17_SAVE_PROBE_NO_WRITE_CAPABILITIES 0u
#ifdef __cplusplus
extern "C" {
#endif
typedef enum P17SaveProbeCodeV1 {
    P17_SAVE_NOT_CHECKED=0,
    P17_SAVE_READ_FAILED=1,
    P17_SAVE_UNSTABLE_SNAPSHOT=2,
    P17_SAVE_WRONG_CONTAINER_SIZE=3,
    P17_SAVE_AUTHENTICATION_FAILED=4,
    P17_SAVE_READ_ONLY_VERIFIED=5
} P17SaveProbeCodeV1;
// Exactly 128 bytes on host and ARM11. No pointer, file/path/handle,
// writable grant or unsigned trust token; buffer owned by caller.
typedef struct P17SaveProbeReportV1 {
    uint32_t version, struct_size, code, capability_bits;
    uint64_t title_id;
    uint32_t container_bytes, raw_bytes, active_slot, slot0_counter,
             slot1_counter, reserved0;
    uint8_t container_sha256[32];
    uint8_t active_raw_sha256[32];
    uint8_t reserved1[16];
} P17SaveProbeReportV1;
// Strict SHAPE validation only; cannot prove authentic/real console data.
int p17_save_probe_report_shape_v1(const P17SaveProbeReportV1* report);
int p17_save_probe_init_v1(P17SaveProbeReportV1* out);
#ifdef __cplusplus
}
#endif
#endif
