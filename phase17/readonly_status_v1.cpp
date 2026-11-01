// SPDX-License-Identifier: GPL-3.0-or-later
// Pure C ABI implementation, isolated from Phase14/18 h168 definitions.
#include "readonly_status_v1.h"
#include <cstring>
static_assert(sizeof(P17ReadOnlyStatusV1)==P17_READONLY_STATUS_V1_SIZE,
              "Phase17 ABI v1 layout changed");
static_assert(offsetof(P17ReadOnlyStatusV1,source_sha256)==40,
              "Phase17 ABI v1 fingerprint offset changed");
extern "C" int p17_readonly_status_init_blocked_v1(P17ReadOnlyStatusV1* out) {
    if(!out)return 0;
    std::memset(out,0,sizeof(*out));
    out->abi_version=P17_READONLY_STATUS_ABI_VERSION;
    out->struct_size=P17_READONLY_STATUS_V1_SIZE;
    out->code=P17_RO_NOT_CHECKED;
    return 1;
}
extern "C" int p17_readonly_status_shape_valid_v1(const P17ReadOnlyStatusV1* s) {
    if(!s || s->abi_version!=P17_READONLY_STATUS_ABI_VERSION ||
       s->struct_size!=P17_READONLY_STATUS_V1_SIZE ||
       s->code>static_cast<uint32_t>(P17_RO_RAM_REHEARSAL_READABLE) ||
       s->capability_bits!=P17_READONLY_STATUS_NO_WRITE_CAPABILITIES ||
       s->reserved0!=0)return 0;
    for(auto c:s->reserved1)if(c)return 0;
    if(s->code!=P17_RO_RAM_REHEARSAL_READABLE) {
        if(s->source_counter || s->container_bytes || s->journal_bytes ||
           s->expected_title)return 0;
        for(auto c:s->source_sha256)if(c)return 0;
    }
    return 1;
}
