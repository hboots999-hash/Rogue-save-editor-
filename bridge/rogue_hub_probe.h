/* Emerald Rogue EX 2.1.2 candidate HUB INSPECTION ONLY.
 * Never writes to a source save or identifies the installed ROM.
 * Observed saves may store hub coordinates with an ABI-dependent stride.
 */
#ifndef ROGUE_HUB_PROBE_H
#define ROGUE_HUB_PROBE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum rogue_hub_status {
 ROGUE_HUB_OK = 0,
 ROGUE_HUB_BAD_ARGUMENT,
 ROGUE_HUB_BAD_SOURCE,
 ROGUE_HUB_WRONG_VERSION,
 ROGUE_HUB_UNRECOGNIZED_LAYOUT
} rogue_hub_status;
typedef struct rogue_hub_report {
 uint8_t selected_copy;
 uint8_t coordinate_stride; /* observed 4, pinned source declares 2 */
 uint8_t built_count;
 uint8_t upgrades_count;
 uint8_t area_flags[2];
 uint8_t upgrade_flags[6];
 uint8_t area_built[11];
 uint8_t upgrade_owned[48];
} rogue_hub_report;
/* Exactly 131072-byte candidate SAV; result untouched on failure.
 * Requires pinned quest/campaign counts, 11 area coordinates, 2 flag
 * bytes, 6 upgrade bytes, and 192 decoration records. Read-only.
 */
rogue_hub_status rogue_probe_hub(const uint8_t* source,size_t n,rogue_hub_report* result);
#ifdef __cplusplus
}
#endif
#endif
