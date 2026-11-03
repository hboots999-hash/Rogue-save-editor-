// SPDX-License-Identifier: GPL-3.0-or-later
// PRIVATE Phase17-only implementation of the Issue #15 installed-save probe.
// Keep this in its OWN translation unit; never include with Phase18 h168
// parsers or stock PKSM generation-three writer headers.
#pragma once
#include "installed_save_probe_v1.h"
#include "authenticated_restore.hpp"
#include <algorithm>
#include <cstring>
#include <new>
#include <utility>

namespace phase17 {
// Device must use a fresh, exact, READ-ONLY installed-title handle on each
// read(Bytes&), and calculate(Digest,Mac) must be bound to the MOST RECENT
// read-only file session. Device::writesAllowed() must always return false.
// Synthetic test doubles NEVER attest real 3DS CMAC validity.
//
// This producer intentionally does NOT inspect journal/receipt files: PKSM
// needs a read-only save view independently of the separate RAM-recovery
// evidence gate. Raw bytes are returned to a *caller-owned memory buffer*
// only after fresh full-container CMAC/inner-checksum validation. The caller
// may then run the existing Phase18 guarded copied-save verifier; the result
// is NOT permission to write the installed title or convert raw drafts.
template<class ReadOnlyDevice>
bool inspectInstalledSaveCopyV1(ReadOnlyDevice& dev,
                                P17SaveProbeReportV1& out,
                                phase14::Bytes& rawCopy) {
    static_assert(!phase14::LiveWritesEnabled,
                  "Installed-game write gate must remain permanently closed");
    (void)p17_save_probe_init_v1(&out);
    rawCopy.clear();
    const auto denied=[&](P17SaveProbeCodeV1 reason)->bool {
        rawCopy.clear();
        (void)p17_save_probe_init_v1(&out);
        out.code=reason;
        return false;
    };
    if(dev.writesAllowed())return denied(P17_SAVE_AUTHENTICATION_FAILED);
    try {
        phase14::Bytes first, second, third;
        if(!dev.read(first))return denied(P17_SAVE_READ_FAILED);
        if(first.size()!=P17_SAVE_PROBE_CONTAINER_BYTES)
            return denied(P17_SAVE_WRONG_CONTAINER_SIZE);
        if(!dev.read(second))return denied(P17_SAVE_READ_FAILED);
        if(second.size()!=P17_SAVE_PROBE_CONTAINER_BYTES)
            return denied(P17_SAVE_WRONG_CONTAINER_SIZE);
        if(first!=second)return denied(P17_SAVE_UNSTABLE_SNAPSHOT);
        // Keep only the second source while opening the THIRD independently
        // read-only session. Authenticate against the third session's own MAC
        // service, not a stale earlier file handle.
        phase14::Bytes().swap(first);
        if(!dev.read(third))return denied(P17_SAVE_READ_FAILED);
        if(third.size()!=P17_SAVE_PROBE_CONTAINER_BYTES)
            return denied(P17_SAVE_WRONG_CONTAINER_SIZE);
        if(third!=second)return denied(P17_SAVE_UNSTABLE_SNAPSHOT);
        phase14::Bytes().swap(second);
        phase14::Layout layout{};
        if(!phase14::authenticate(third,dev,layout))
            return denied(P17_SAVE_AUTHENTICATION_FAILED);
        const auto slot0=rogue_live::parseHeader(third.data(),
                              rogue_live::HeaderSize,0);
        const auto slot1=rogue_live::parseHeader(
                              third.data()+rogue_live::SlotSize,
                              rogue_live::HeaderSize,rogue_live::SlotSize);
        if(!slot0.valid || !slot1.valid)
            return denied(P17_SAVE_AUTHENTICATION_FAILED);
        const size_t rawOffset=layout.active+rogue_live::HeaderSize;
        if(rawOffset>third.size() ||
           third.size()-rawOffset<P17_SAVE_PROBE_RAW_BYTES)
            return denied(P17_SAVE_AUTHENTICATION_FAILED);
        // The only copied content is one vetted 128KiB active raw slot.
        // All large buffers are heap-backed, never 3DS main-thread stack.
        phase14::Bytes stagedRaw(third.begin()+rawOffset,
                                 third.begin()+rawOffset+P17_SAVE_PROBE_RAW_BYTES);
        P17SaveProbeReportV1 staged{};
        (void)p17_save_probe_init_v1(&staged);
        staged.code=P17_SAVE_READ_ONLY_VERIFIED;
        staged.title_id=P17_SAVE_PROBE_EXPECTED_TITLE;
        staged.container_bytes=uint32_t(third.size());
        staged.raw_bytes=uint32_t(stagedRaw.size());
        staged.active_slot=layout.active?1u:0u;
        staged.slot0_counter=slot0.timesSaved;
        staged.slot1_counter=slot1.timesSaved;
        const auto whole=phase14::hash(third.data(),third.size());
        const auto active=phase14::hash(stagedRaw.data(),stagedRaw.size());
        std::copy(whole.begin(),whole.end(),staged.container_sha256);
        std::copy(active.begin(),active.end(),staged.active_raw_sha256);
        if(!p17_save_probe_report_shape_v1(&staged))
            return denied(P17_SAVE_AUTHENTICATION_FAILED);
        rawCopy.swap(stagedRaw);
        out=staged;
        return true;
    } catch (...) {
        // Includes failed heap allocation: no partial raw copy or positive
        // report survives. Native builds must enable C++ exception handling.
        return denied(P17_SAVE_READ_FAILED);
    }
}
} // namespace phase17
