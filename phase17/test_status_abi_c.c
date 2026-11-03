/* Phase20 C consumer only. Never imports internal Phase17/18 headers. */
#include "readonly_status_v1.h"
#include <assert.h>
#include <string.h>
int main(void) {
    P17ReadOnlyStatusV1 s;
    assert(sizeof(s)==P17_READONLY_STATUS_V1_SIZE);
    assert(p17_readonly_status_init_blocked_v1(&s));
    assert(p17_readonly_status_shape_valid_v1(&s));
    assert(s.code==P17_RO_NOT_CHECKED && s.capability_bits==0);
    s.capability_bits=1; assert(!p17_readonly_status_shape_valid_v1(&s));
    s.capability_bits=0; s.abi_version=2;
    assert(!p17_readonly_status_shape_valid_v1(&s));
    s.abi_version=1; s.struct_size=sizeof(s)-1;
    assert(!p17_readonly_status_shape_valid_v1(&s));
    s.struct_size=sizeof(s);s.code=UINT32_MAX;
    assert(!p17_readonly_status_shape_valid_v1(&s));
    assert(!p17_readonly_status_shape_valid_v1(0));
    s.code=P17_RO_NOT_CHECKED;s.source_sha256[0]=1;
    assert(!p17_readonly_status_shape_valid_v1(&s));
    return 0;
}
