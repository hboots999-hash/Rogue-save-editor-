# Phase 18 -> Phase 20: multi-edit offline v3 handoff

**Status:** experimental **SD-COPIED-SAVE** batching only. PR #8 stays draft.
**Compatibility:** `phase18/integration_v2.hpp`, `phase18/offline_export.hpp`,
`v2::preview`, `v2::verify`, and Phase 20's existing single-edit controller
remain unchanged. The v3 headers are *additional* opt-in interfaces.

## Exact usage (C++17)

```cpp
#include "phase18/multi_export_v3.hpp"

// source is a separately imported, complete, immutable 131072-byte H168
// SD copy; never a mounted live game save or 263168-byte wrapper.
const auto view = rogue18::v2::inspect(source, 131072);
rogue18::v3::Batch transaction{};
if (!rogue18::v3::start(view, transaction)) return;

bool added =
    rogue18::v3::append(view, transaction, rogue18::Kind::Money, 0, 0, 125000) &&
    rogue18::v3::append(view, transaction, rogue18::Kind::ExistingQuantity, 0, 0, 99) &&
    rogue18::v3::append(view, transaction, rogue18::Kind::IVs, 0, 1, 31) &&
    rogue18::v3::append(view, transaction, rogue18::Kind::EVs, 0, 3, 0) &&
    rogue18::v3::append(view, transaction, rogue18::Kind::Move, 0, 0, 10);
if (!added) return; // invalid, duplicate, unsupported, stale or no-op
const auto proof=rogue18::v3::preview(source, 131072, transaction);
if (!proof.ok ||
    !rogue18::v3::verify(source,131072,proof.candidate.data(),
                        proof.candidate.size(),transaction)) return;

// Only after separate, explicit Phase20 human review and confirmation:
const auto exported=rogue18::v3::exportCopiedBatch(
    source, 131072, "sdmc:/3ds/PKSM-Rogue/imports/00000001.sav",
    "sdmc:/3ds/PKSM-Rogue", transaction);
if (!exported.ok) {
    // Treat backupName and draftName as UNCERTAIN attempted paths, if present.
    // Never use a failed output. Never overwrite or delete it automatically.
    return;
}
// exported.backupName and exported.draftName name new separate SD-only files.
// NEVER restore an edited draft to the installed game.
```

**Parameters:** `append(snapshot,batch,kind,partyOrBagSlot,field,newValue)`.
For money and item quantity the field index MUST be zero. For party IV/EV,
field is 0..5. For existing move PP only, field is move slot 0..3; the
original nonempty move **ID is locked** and never replaced. Capture **all**
edits against the same original v2 snapshot, not successive edited drafts.

## Transaction invariants

- `ContractVersion=3`; full SHA-256 of every original byte; active raw-bank
  counter; exact original field values, item IDs, Pokémon PID/OT/species,
  checksum and move IDs. Caller cannot supply ROM-specific evidence overrides.
- Every edited scalar appears **once**. Duplicate/conflicting logical fields,
  stale inputs, absent/egg party, empty moves, forbidden kinds, out-of-range
  inputs, no-op writes, EV sum over 510 and wrong sizes are rejected.
- Per-edit preview/verify works on the **current** candidate, with its updated
  Pokémon checksum and digest. EV reductions are sorted before increases.
  An independent direct-field encoder reconstructs all expected after-bytes
  from the original source; final dual-bank parser and exact full-buffer
  equality/changed-byte footprint checks are mandatory.
- Bounded to **128 scalar edits** until physical ARM11 heap/latency tests.
  Every request retains its original optimistic identity; batches are atomic
  at the *RAM candidate* level, not at the filesystem/hardware level.
- A successful `exportCopiedBatch` creates one new immutable
  `backups/phase18_original_NNN.sav` and one verified
  `drafts/phase18_draft_NNN.sav`. It never changes the imported source.
  Exports are exclusive/no-overwrite, re-read the original at several stages,
  and independently validate the SD draft readback.
- **Failed exports can leave an attempted or partial SD file**. `ok=false`
  always blocks approval; any `backupName`/`draftName` on failure is an
  attempted location, not proof that the path exists or contains valid data.
  This is particularly important for full SD, flush/close and restart failures.

## Evidence and limitations

Synthetic fixtures and host-injected SD faults exercise cross-field edits,
all 24 encrypted Pokémon permutations, all six party positions, bounded
large batches, immutable backup naming, corruption and restart. ARM11 CI
checks header compilation and compiler-estimated frames, **not** physical
3DS available heap, export latency, flush durability, game loading or
re-saving. Phase 20 must re-pin Phase 18's **exact final passing SHA** and
rerun its own strict/sanitizer/native emulator integration before adopting v3.

A multi-edit candidate is *not* a valid Phase 13/14 restore transaction:
the authenticated 263168-byte VC container, CMAC, write-ahead durable
recovery and actual installed-ROM semantics remain unapproved. **No live
GBA VC writes, restore UI, auto-injection or PC/species/item-insertion/
ability/nature/replacement-move editing may be enabled.**
