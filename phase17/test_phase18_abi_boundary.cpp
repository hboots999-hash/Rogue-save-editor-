// Compile-only+host adapter consumer: Phase18 parser and Phase17 PUBLIC ABI.
// The Phase17 internal producer MUST be compiled in another translation unit;
// including Phase17 restore headers here would reintroduce duplicate h168 types.
#include "readonly_status_v1.h"
#include "phase18/offline_engine.hpp"
#include <cassert>
#include <type_traits>
static_assert(std::is_standard_layout<P17ReadOnlyStatusV1>::value,
              "Phase20 ABI must remain standard layout");
static_assert(sizeof(P17ReadOnlyStatusV1)==80,"Unexpected API size");
int main() {
    P17ReadOnlyStatusV1 s{};
    assert(p17_readonly_status_init_blocked_v1(&s));
    assert(p17_readonly_status_shape_valid_v1(&s));
    assert(s.capability_bits==0 && s.code==P17_RO_NOT_CHECKED);
    static_assert(!std::is_pointer<decltype(s.source_sha256)>::value,
                  "No native save pointer may cross the boundary");
    return 0;
}
