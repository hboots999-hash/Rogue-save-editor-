# Phase18 -> Phase20 offline editor integration hardening (1 November 2026)

**Branch:** `feature/phase18-offline-editors` (draft PR #8), independent of
Phase20 draft PR #12 (`feature/phase20-readonly-integration`).
**Scope:** synthetic 128 KiB H168 copies, offline-only drafts. No live writer,
ROM data, private .sav, authenticated full-container adapter or restore.
**Audited Phase20 source:** `phase20/offline_edit_v1.hpp` from PR #12 at
`5734735c93f74de3ac2f313488989c28c39a49e1`. Revalidate if PR #12
moves. Phase20's `requestForCopy` uses
`v2::inspect/begin/beginMove/preview/verify` and
`exportGuarded` uses Phase18 `exportCopied`, so no v2 signature change
was necessary.

## Phase20 adoption instructions

1. **Keep using the existing Phase18 v2 signatures.** Single-edits must
   start at the displayed SHA-256-locked snapshot and refuse stale source
   bytes; Phase20's two-confirmation UI is not bypassed.
2. `ExportResult::ok` is the **only success indicator**. A failure may now
   include `backupName` and/or `draftName` identifying a reserved,
   incomplete or otherwise uncertain *attempted* file. This is intentional;
   filenames on failed exports are **not** approved draft filenames.
   The `reason` text also embeds the attempted path, so Phase20's current
   error UI can display the warning without changing the ABI. If Phase20
   adopts the metadata directly, label it `UNCERTAIN FILE — DO NOT USE`,
   never `EXPORT COMPLETE`.
3. Import `00000001.sav` is read-only. The backup must be exclusive,
   verified and immutable before draft creation. Each draft uses exclusive
   new naming (up to 99); incomplete files are preserved for external
   diagnosis, not deleted or overwritten. After a failed export, restart
   and re-import before another deliberate confirmation.
4. Do **not** pass the `candidate` vector to Phase13/14's restore API.
   That preflight supports only money +1 / existing quantity +1 and does
   not approve broader Phase18 edits. All installed writes remain disabled.
5. `v2::availableForOfflinePreview` permits only Money, ExistingQuantity,
   IVs, EVs and **existing move ID with PP**. All other capabilities remain
   disabled, including species, PC, abilities, nature, item insertion and
   new move IDs. This contract does not authenticate the exact private ROM
   or recalculate cached party battle stats.
6. **No automatic multi-edit chaining yet.** The current flow intentionally
   makes *one independent edit from one imported source*. A safe future
   batch would need a finite max number of edits; the same original
   SHA-256 and counter; no duplicated/conflicting field; independent
   revalidation after each sequential intermediate image; final full-image
   proof against the original; all-or-nothing single draft export; and
   physical restart/rollback testing. Introducing chaining today would
   widen the game's unverified save-state risk and Phase20 UI semantics.
   No multi-edit production API is exposed by this branch.

## Five-operation safety matrix

| Operation | Positive synthetic tests | Denied/edge cases | ROM certainty |
| --- | --- | --- | --- |
| Money | 0, 999999, exact checksum/diff, reverse to original | 1000000, unchanged, stale digest/counter | Raw geometry only |
| Existing quantity | 1..999 on occupied entries, first/middle/last including 449/450 | 0/1000, bad slot/index/ID, unchanged | Item IDs not matched to private ROM |
| IVs | 0..31, 6 party positions, 24 block permutations, 6 component indices | 32, missing slot, stale mon checksum, no-op | Cached battle stats not recomputed |
| EVs | 0..252, sum<=510, 6 party positions, 24 permutations, 6 component indices | 253, sum>510, invalid slot, no-op | Cached battle stats not recomputed |
| Existing move PP | 0..64, all 4 nonempty slots, all 24 permutations, 6 party positions | >64, empty slot, invalid slot, change to new move ID, no-op | No move-specific max-PP mapping |

The Phase18 source fixture, Phase20 cross-branch QA and the new
`test_boundaries_v2.cpp` / `test_phase20_matrix.cpp` use fabricated
non-playable data only, never a real game save. A full byte-level check
compares the approved candidate and the entire 131072-byte source, not
only checksummed H168 sectors. Both rotated banks and the tail are pinned;
a changed tail byte is rejected by SHA-256 even if both bank counters and
the edited field are unchanged.

## SD failure and restart review

`test_sd_faults.cpp` injects host-only faults inside the **actual**
`raw_v1_sd.hpp::uniqueWrite` exclusive-create path for both the backup
and draft phase: open/storage-full failure; immediate write failure;
half-file write then ENOSPC; failed fsync; failed close; and readback
failure. It then repeats the export in a new simulated session and
checks that the old incomplete/uncertain name is never overwritten,
the imported file is byte-identical, new backup/draft names advance and
only checked drafts return `ok=true`. Tests also cover missing/short
imports and all 99 occupied backup names.

The failure hook is compiled **only** with
`ROGUE18_TEST_IO_FAULTS` on non-3DS hosts, and absent from native ARM11.
Physical SDMC `FSUSER_CreateFile`, `FSFILE_Write`,
`FSFILE_Close`, `FSUSER_CloseArchive`, media removal and hard
power loss are not emulated faithfully by POSIX faults. The real 3DS
requires its own expendable copied-data/SD stress audit. The native
version deliberately keeps partially created files and returns failure,
not unapproved success.

## Actual memory-risk audit

- `phase18/offline_export.hpp` now frees each completed 128 KiB
  `readExact` buffer **before** calling preview/verify or taking the
  next import snapshot. This reduces peak live heap allocations without
  weakening the before/after freshness checks.
- `v2::preview`, `v2::verify`, `propose`, Phase20's source buffer,
  and `exportCopied` still require multiple independent 128 KiB
  heap allocations. Low-memory/fragmentation behaviour is not proven
  by compiling for ARM11; Phase20 catches `std::bad_alloc` and other
  `std::exception` at its offline-export boundary.
- ARM11 `-fstack-usage` and `-Werror=frame-larger-than=4096`
  compile the **actual** `exportCopied` call tree, not only the
  read-only model. On the audited native build the largest reported
  static stack frame was **704 bytes** for the native SD writer;
  `propose` was **600**, `exportCopied` **528**, source SHA-256
  **480** bytes. These are *per-function estimates*, not a bound on
  the combined call stack, recursive call chains or available heap.
- An actual console heap high-water mark, fragmentation trace, clean
  restart on partially written SD files and cold-boot reinspection
  remain unverified. Do not advertise hardware memory safety as proven.

## Integration disposition

**Ready for further copied-SD testing:** all five v2 edit classes, immutable
and unique backup/draft export, SHA-locked preview and byte-exact verify,
invalid/no-op rejection, and host fault/restart simulations.

**Not release-ready:** any edit of a live GBA VC save, authentication/CMAC
bridge, staged restore, species/PC/abilities/nature/new items/new move ID,
ROM matching, cached-stat recalculation and physical hardware fault testing.

See the Phase18 GitHub Actions workflow for the exact successful commit
SHA and three jobs (strict, ASan/UBSan, native ARM11). Do not extrapolate
the standalone Phase18 CI results to Phase20's own pinned CIA/emulator tests.
