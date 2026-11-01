# Phase 20 offline editor hardening handoff — 1 November 2026

**PR:** Phase18 #8 (draft), integration #12 (draft).
**Audited integration input:** `feature/phase20-readonly-integration`,
`offline_edit_v1.hpp` at `5734735c93f74de3ac2f313488989c28c39a49e1`.
**Scope:** detached 131072-byte SD copied save ONLY; no installed game write,
CMAC signing, raw-to-wrapper conversion, restore or title archive.
**No private save or copyrighted ROM material entered these tests.**

## Audit conclusion

Phase20 correctly builds its five choices through
`requestForCopy` -> `v2::inspect` -> `v2::begin` or
`v2::beginMove` -> `v2::preview` -> `v2::verify` ->
`exportGuarded` -> `exportCopied`. The UI checks imported display/counter,
full source SHA-256, all old Pokémon fields, occupied item IDs and two
explicit A confirmations. Cancelling is an in-memory state change only.
The exporter re-reads the fixed import, retains immutable backup and distinct
draft files, then checks the entire draft byte-for-byte on SD readback.
`v2::preview` refuses new move IDs and all ROM-unverified edits.

**Exact APIs Phase20 should keep calling:**

```cpp
rogue18::v2::inspect(source, sourceLength);
rogue18::v2::begin(snapshot, kind, partyOrBagSlot, request);
rogue18::v2::beginMove(snapshot, partySlot, moveSlot, request);
rogue18::v2::preview(source, sourceLength, request);
rogue18::v2::verify(source, sourceLength, draft, draftLength, request);
rogue18::exportCopied(source, sourceLength,
                     fixedSourcePath, fixedAppRoot, request);
```

No v2 function signature or capability flag changed in this hardening.
**Phase20 MUST re-pin its Phase18 dependency** to the final commit after CI,
rather than continuing to build against `59888cb7194e2a819602a58ddfa76829d060aa00`.

## Error handling / output contract change (backward compatible)

Previously a failed write could leave a short, complete-but-unflushed or
close-uncertain output on SD without returning its filename. Now
`raw_v1::uniqueWrite` clears the outgoing filename before a new attempt,
and populates it immediately after a successful POSIX `O_EXCL` creation
or immediately before the exclusive native SDMC create. Failures retain
the **attempted** filename, which may or may not correspond to a created
file on native FS.

`ExportResult::backupName` and `draftName` can therefore be nonempty
**when `ok==false`**. This is a *possible failed/uncertain file location*,
not approval or proof of validity. The error `reason` includes
`[attempted backups/phase18_original_NNN.sav]` or
`[attempted drafts/phase18_draft_NNN.sav]` when available.
Phase20's existing `EXPORT BLOCKED: %.215s` result screen will surface
a useful attempted path with no source change. New native guidance should
state: **never import, restore or use a draft on a failed result; inspect
partial SD files manually and keep immutable originals**.

`raw_v1::readExact` now rejects close failures in addition to short reads,
extra data and read errors, avoiding false fresh-source acceptance.

### Deterministic storage failures and restart

Host-only `ROGUE18_TEST_IO_FAULTS` injection covers exclusive-create
no-space, write no-space, partial write/no-space, flush failure,
close failure and readback failure, independently during backup and
draft creation (12 combinations). On failure, the imported copy remains
identical; successful backups remain immutable; uncertain files stay
at unique names; a new invocation allocates another name rather than
truncating or overwriting the partial output. Also checks missing/short
import and exhaustion of all 99 slot names. These are **simulated** host
faults, not measured 3DS SD card power-loss behaviour.

No automatic cleanup, retry-overwrite, source replacement or restore
authority has been added. A native partially failed SDMC create can still
leave an uncertain file; future hardware testing must check each FS
failure code and directory contents.

## Synthetic editor/byte-footprint coverage

- 24 encrypted Pokémon block permutations, six occupied party positions,
  all six IV fields, all six EV fields and four existing move slots;
  `test_phase20_matrix.cpp` tests the five operations using the same v2
  interface Phase20 consumes.
- Both ends of money (0, 999999), existing item quantity (1, 999),
  IVs (0,31), EV budget <=510 and same-move PP (0,64), plus overflows,
  invalid positions, empty move slots and no-op requests.
- Source SHA-256 on the entire 131072 bytes, including unchecksummed
  tail; corrupt current/inactive banks and stale counters; exact-source
  restore of a single edited IV proposal and no mutation of the source.
- `test_phase20_boundaries.cpp` complements this with generated
  full-party/expanded-bag synthetic boundary coverage.
- `v2::verify` independently recreates each approved candidate and
  compares all bytes, not merely the allowed fields. The unchanged inactive
  bank and unchecksummed tail must remain identical.

## Memory and ARM11 evidence — do not overclaim

`exportCopied` releases the 128 KiB fresh-import readback buffer before
each expensive v2 preview or verification. The draft and reconstructed
verification candidate remain separate heap-backed 128 KiB buffers;
the caller's original imported source remains live. Thus several
128 KiB allocations can temporarily coexist and low-memory exceptions
remain possible on physical hardware. The Phase20 controller already
catches `std::bad_alloc` / `std::exception` and blocks approval.
The optional PC research scratch has been moved to checked heap allocation.
The Phase18 native job now forces code generation for SD exporter,
adds `-Werror=frame-larger-than=4096 -fstack-usage` and reports
compiler-estimated ARM11 frames. A passing compile DOES NOT establish
a physical 3DS heap high-water mark, actual stack usage at runtime,
SD flash durability or interrupted-write recovery.

**Needed later for device approval:** safe, non-game copied SD file only,
live free-heap/stack watermark samples before/after two-confirmation export,
forced low-memory tests, power-loss and full-SD scenarios with externally
verified immutable backups and isolated disposable data. No private save
or real game required for early SD transport checks.

## Multiple edits — deliberately NOT enabled

A multi-edit API is postponed. An approved design must independently pin
the original full-file digest, regenerate an optimistic request from each
intermediate **verified** copied snapshot, reject overlapping/duplicate
field edits, independently validate final semantics/checksums and prove
the union of allowed exact byte footprints from the immutable source.
It must keep a single immutable backup and one separately named draft,
reject all unsupported types and not expose any wrapper/restore bridge.
Additional 128 KiB copies exacerbate unmeasured 3DS heap pressure.
The current per-confirmation single-edit API remains safer.

## Exact ROM evidence and release gates

Public EX-v2.1 structural geometry is still not a build-identity proof for
the installed Emerald Rogue 2.1.2 ROM. Ability/nature/species, new moves,
new item insertion and PC edit operations remain disabled. Pokémon IV/EV
edits may leave cached battle-stat fields unchanged until actual game
initialization/re-save; target-game acceptance is unproven. Phase13/14's
+1 restoration preflight never authorizes wider Phase18 edits; even a
matching +1 offline candidate has **no permission** to restore.

**Phase20 actions after Phase18 passes CI:** re-pin Phase18 commit; rerun
full strict/sanitizer native package/optional emulator workflows; preserve
two A confirmations; display attempted filenames on failed exports;
keep no live GBA VC writes or restore controls.
