// SPDX-License-Identifier: GPL-3.0-or-later
// Native-only PKSM H168 read-only probe. Compiled separately from Phase18,
// stock PKSM EditorScreen, and UI; contains NO write/export/restore path.
#include "installed_save_probe_native_v1.h"
#include "installed_save_probe_private.hpp"
#include "native_pxi.hpp"
#include <cstring>
static_assert(!phase14::LiveWritesEnabled,"NO installed-game writes");
extern "C" int p17_native_inspect_copy_v1(P17SaveProbeReportV1* out,
                                           uint8_t* raw_out,
                                           uint32_t raw_capacity) {
    if(!out)return 0;
    (void)p17_save_probe_init_v1(out);
    if((raw_out==nullptr && raw_capacity!=0u) ||
       (raw_out!=nullptr && raw_capacity!=P17_SAVE_PROBE_RAW_BYTES)) {
        out->code=P17_SAVE_READ_FAILED;
        return 0;
    }
    // A stale caller buffer must never be mistaken for an authenticated save
    // after an error. Allocate this large output in heap/storage, not stack.
    if(raw_out)std::memset(raw_out,0,P17_SAVE_PROBE_RAW_BYTES);
    phase14::NativePxi device; // Opens only FS_OPEN_READ.
    phase14::Bytes copied{};
    const bool accepted=phase17::inspectInstalledSaveCopyV1(device,*out,copied);
    const bool closed=device.close();
    if(!accepted || !closed ||
       copied.size()!=P17_SAVE_PROBE_RAW_BYTES ||
       !p17_save_probe_report_shape_v1(out)) {
        (void)p17_save_probe_init_v1(out);
        out->code=P17_SAVE_READ_FAILED;
        return 0;
    }
    if(raw_out)std::memcpy(raw_out,copied.data(),P17_SAVE_PROBE_RAW_BYTES);
    return 1;
}
