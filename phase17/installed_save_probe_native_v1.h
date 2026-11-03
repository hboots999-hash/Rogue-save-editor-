// SPDX-License-Identifier: GPL-3.0-or-later
// OPTIONAL native 3DS implementation boundary for PKSM Issue #15.
// Compiled in the Phase17 TU; no third-party PKSM/Phase18 h168 types.
// This entrypoint is NOT connected to an editor save or restoration action.
#ifndef P17_INSTALLED_SAVE_PROBE_NATIVE_V1_H
#define P17_INSTALLED_SAVE_PROBE_NATIVE_V1_H
#include "installed_save_probe_v1.h"
#ifdef __cplusplus
extern "C" {
#endif
// Returns 1 ONLY on fresh triple-read, file-bound CMAC + inner-H168 PASS
// and successful read-only close. report must be nonnull.
// raw_out==NULL, raw_capacity==0: status-only.
// raw_out!=NULL requires exactly 131072 bytes CALLER-ALLOCATED (HEAP).
// Copies the verified active slot into raw_out; zeroes it on any failure.
// No SD export, no title write, no CMAC rewrite, no replay, no handle exposed.
// NOT an installed-save write permission even when it returns 1.
// Native/hardware behavior is NOT validated merely by emulator compilation.
int p17_native_inspect_copy_v1(P17SaveProbeReportV1* report,
                               uint8_t* raw_out, uint32_t raw_capacity);
#ifdef __cplusplus
}
#endif
#endif
