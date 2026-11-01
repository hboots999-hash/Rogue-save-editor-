# Phase 18 v3 multi-edit offline transaction — Phase 20 integration handoff

**Status:** experimental SD-copy-only API in **draft PR #8**; no native GBA VC game writes, no restore, no CMAC, no ROM or private save data. The independent Phase 20 draft PR #12 is not modified. Existing v2 single-edit `begin/preview/verify/exportCopied` signatures are unchanged.

## Versioned API

- `phase18/multi_edit_v3.hpp` defines `rogue18::v3::ContractVersion == 3`, `Batch`, `Edit`, `MaxEdits == 128`, `start`, `append`, `preview`, `verify`, and `mayWriteInstalledGame` (always **false**).
- `phase18/multi_export_v3.hpp` adds `v3::exportCopiedBatch`, whose root/source restrictions and success/failure semantics match the existing SD-only `rogue18::exportCopied` path. It never overwrites the imported save.
- **No v2 API or Phase20 menu changes are required** until Phase20 separately chooses to consume this additive v3 interface. DO NOT conflate v3 offline approval with game-playable drafts, title-archive permission, or Phase13/14 narrow +1 restoration preflight.

## Integration example (only after explicit UI review + confirmation)

```cpp
#include "phase18/multi_export_v3.hpp"
const auto original=rogue18::v2::inspect(importedCopy, 131072);
rogue18::v3::Batch batch{};
if (!rogue18::v3::start(original,batch)) {
    // No valid copied-save snapshot; stay read-only.
}
bool complete =
    rogue18::v3::append(original,batch,rogue18::Kind::Money,0,0,50000) &&
    rogue18::v3::append(original,batch,rogue18::Kind::ExistingQuantity,0,0,50) &&
    rogue18::v3::append(original,batch,rogue18::Kind::IVs,0,0,31) &&
    rogue18::v3::append(original,batch,rogue18::Kind::EVs,0,1,251) &&
    rogue18::v3::append(original,batch,rogue18::Kind::Move,0,0,10);
// Each append must target a value different from this copied save's
// original value. This particular example is illustrative: check the
// actual snapshot values first and omit unchanged requests.
if (complete) {
    const auto candidate=rogue18::v3::preview(importedCopy,131072,batch);
    if(candidate.ok &&
       rogue18::v3::verify(importedCopy,131072,
                           candidate.candidate.data(),candidate.candidate.size(),
                           batch)) {
        // AFTER an explicit two-step user confirmation, and only for a
        // user-supplied SD copy in the fixed app-owned import directory:
        const auto saved=rogue18::v3::exportCopiedBatch(
            importedCopy,131072,
            "sdmc:/3ds/PKSM-Rogue/imports/00000001.sav",
            "sdmc:/3ds/PKSM-Rogue",batch);
        // saved.ok is true ONLY when immutable backup, complete draft,
        // source freshness, readback and batch proof all succeeded.
        // saved.backupName / saved.draftName may identify uncertain files
        // on failure; never approve or use those files as a draft.
    }
}
```

**The batch references ONE immutable original, not prior exported drafts.** Batch requests must be assembled with `v3::append` from that original `v2::ReadSnapshot`; never create arbitrary `Edit` structs in a UI. Batches are mutable data objects, and `v3::preview` defensively revalidates every lock even if a caller edits the structs directly.

## Allowed scalar operations only

| `Kind` | `slot` | `field` | `value` |
| --- | --- | --- | --- |
| `Money` | 0 | 0 | 0…999999 |
| `ExistingQuantity` | Existing occupied bag index | 0 | 1…999 |
| `IVs` | Existing non-egg party index 0…5 | IV index 0…5 | 0…31 |
| `EVs` | Existing non-egg party index 0…5 | EV index 0…5 | 0…252; combined sum <=510 |
| `Move` | Existing non-egg party index 0…5 | Existing occupied move slot 0…3 | PP only, 0…64 |

A valid batch has **1–128 edits**, with **no duplicate (`Kind`, `slot`, `field`) key** and no no-op scalar. Item IDs, move IDs, Pokémon species, nature, ability, personality and trainer identity are never edit targets. New items, species, abilities, natures, new moves and PC edits remain disabled.

## Safety protocol

1. Validate exact 131072-byte raw source, dual-bank counters/checksums and SHA-256. A 263168-byte authenticated wrapper is **not accepted**.
2. Every `Edit` includes the complete original SHA-256 and counter; old scalar, expected item ID or Pokémon PID/OT/species/decrypted checksum and old move ID (for PP). A transplanted or stale edit is rejected.
3. Check the **combined final** EV totals for all affected Pokémon; stable-sort EV decreases before increases to ensure intermediate v2 previews never violate 510, including redistribution from one stat into another.
4. For each ordered scalar, re-inspect **the current in-RAM candidate**, rebuild a guarded **new v2 request from it**, then run v2 preview with exact expected scalar. No original Pokémon checksum is reused after another field of that Pokémon changed.
5. Independently rebuild the final expected output from original bytes with direct byte-scoped encoders, encrypted substructure checksum regeneration and exactly one active H168 sector checksum update. Byte-compare this independent reconstruction to the sequential v2 result across all 131072 bytes. Reject any byte outside the allowed union of scalar, Pokémon checksum and sector checksum fields. Final v2 read-only roundtrip validates structure and each requested result.
6. `v3::verify` reruns the original-locked aggregate and whole-buffer equality. Import must be byte-identical before backup, after immutable backup and after draft creation. Stored draft is independently re-read and verified. All outputs have exclusive unique filenames; failed/uncertain partials are preserved for inspection.
7. Repeated export produces an entirely new backup and draft; never modifies the import or preexisting files. Errors return `ok=false` even when a partial/uncertain file exists. No automatic retries of occupied output filenames.
8. Do not use this as a restore transaction. `v3::mayWriteInstalledGame(batch)` is unconditionally false.

## Synthetic verification and resource caveats

`test_multi_v3.cpp` tests 24 Gen III encrypted substructure permutations and all six party slots; every occupied move field, non-overlapping IV changes and multi-field aggregate; a **100-edit batch** incorporating IVs, EVs, PP, money and item quantities, including EV increases inserted before decreases; duplicate/stale/forged identity, no-op, oversized/empty and illegal EV total rejection; source/inactive-bank/tail preservation; two immutable SD exports and source mutation rejection. Additional independent hostile v3 tests exercise generated high bag-item indices and batch stress.

`test_multi_v3_faults.cpp` uses host-only simulated storage faults: six failure modes each at the backup and draft stage, followed by a restart/re-export checking that original/previous outputs remain intact. This does **not** validate native filesystem crash consistency on a real 3DS.

Native ARM11 compile forces code generation of `v3::preview` and `v3::exportCopiedBatch`, enforces `-Werror=frame-larger-than=4096` and parses `-fstack-usage` outputs. Representative compiled frames: **792 bytes** for v3 preview and **552 bytes** for v3 exporter, from run 41344444236; results apply to toolchain and commit of that run. Heap pressure can exceed several 128 KiB buffers plus a 4 KiB active-section byte-footprint mask during long batches, and execution latency scales with the number of repeated v2 validations. Cap is 128 until physical-device free-heap/latency and SD reliability are measured.

**Outstanding:** exact private Emerald Rogue 2.1.2 ROM metadata; recalculation of cached party battle stats after IV/EV edits; native 3DS heap high-water marks, repeated exports, actual power-loss fault behavior, gameplay and save/reload; release gate authorization. Keep offline-only and PR #8 draft.

## Phase 20 adoption

1. Pin the **final green Phase18 commit** and run its own full host strict, ASan/UBSan, ARM11, package and emulator smoke tests. Do not silently substitute older `59888cb7` evidence.
2. Preserve its existing single-edit journey and two-confirmation gate; v3 is an additive optional path, **not** an automatic chain of previous drafts.
3. Display batch edit count, exact summary, source digest prefix, original backup and distinct draft; on failure display all attempted/uncertain file names and refuse to open them as approved data.
4. Never enable a live title writer or restoration based on v3 checks alone.

**Further ARM11 memory hardening:** the independent expected-diff footprint now uses only a 4096-byte active-sector bitmap, because every supported scalar resides in H168 logical sector 1. Any change outside that sector is rejected. This reduces temporary heap use by approximately 124 KiB versus the initial implementation; the full 131072-byte final expected-data comparison is retained.
