// SPDX-License-Identifier: GPL-3.0-or-later
#include "installed_save_probe_v1.h"
#include <cstring>
#include <cstddef>
static_assert(sizeof(P17SaveProbeReportV1)==P17_SAVE_PROBE_REPORT_SIZE,
              "Phase17 installed-save ABI v1 size changed");
static_assert(offsetof(P17SaveProbeReportV1,container_sha256)==48,
              "Phase17 installed-save digest offset changed");
static_assert(offsetof(P17SaveProbeReportV1,active_raw_sha256)==80,
              "Phase17 active raw digest offset changed");
extern "C" int p17_save_probe_init_v1(P17SaveProbeReportV1* out) {
    if(!out)return 0;
    std::memset(out,0,sizeof(*out));
    out->version=P17_SAVE_PROBE_VERSION;
    out->struct_size=P17_SAVE_PROBE_REPORT_SIZE;
    return 1;
}
extern "C" int p17_save_probe_report_shape_v1(const P17SaveProbeReportV1* r) {
    if(!r || r->version!=P17_SAVE_PROBE_VERSION ||
       r->struct_size!=P17_SAVE_PROBE_REPORT_SIZE ||
       r->code>static_cast<uint32_t>(P17_SAVE_READ_ONLY_VERIFIED) ||
       r->capability_bits!=P17_SAVE_PROBE_NO_WRITE_CAPABILITIES ||
       r->reserved0!=0)return 0;
    for(uint8_t v:r->reserved1)if(v)return 0;
    if(r->code==P17_SAVE_READ_ONLY_VERIFIED) {
        if(r->title_id!=P17_SAVE_PROBE_EXPECTED_TITLE ||
           r->container_bytes!=P17_SAVE_PROBE_CONTAINER_BYTES ||
           r->raw_bytes!=P17_SAVE_PROBE_RAW_BYTES || r->active_slot>1u)
            return 0;
        const uint32_t a=r->slot0_counter,b=r->slot1_counter;
        const uint32_t low=a<b?a:b,high=a>b?a:b;
        if(high>=UINT32_MAX-1u || high-low!=1u ||
           r->active_slot!=(a>b?0u:1u))return 0;
    } else {
        if(r->title_id || r->container_bytes || r->raw_bytes ||
           r->active_slot || r->slot0_counter || r->slot1_counter)return 0;
        for(uint8_t v:r->container_sha256)if(v)return 0;
        for(uint8_t v:r->active_raw_sha256)if(v)return 0;
    }
    return 1;
}
