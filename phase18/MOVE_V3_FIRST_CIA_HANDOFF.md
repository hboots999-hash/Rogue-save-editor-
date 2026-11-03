# Phase 18 final move replacement implementation — Phase 20 first CIA handoff

**Purpose:** edit **one occupied move slot** in an imported H168 131072-byte copied save. First CIA **experimental offline draft/backup export**, NOT GBA VC save injection. Phase20's existing PP-only `Kind::Move` editor is unchanged. Code is on **draft PR #14 only**, not merged.

## Required files

- `phase18/move_replacement_v3.hpp`: **production-compiled** encrypted replacement and SHA-256/identity/sector checksum/byte-footprint proof; independently reconstructs candidate in `verify`.
- `phase18/offline_move_export_v3.hpp`: exclusive-create backup and draft on SD, fsync, reopen, compare **all 131072 bytes**, return SHA256 receipt; fixed destination `sdmc:/3ds/PKSMRogue/drafts` (folder must already exist).
- `phase18/move_queue_v4.hpp`: existing staging unchanged; new **`previewSingleMoveDraft`** and **`exportSingleMoveDraft`** adapters, which only work with exactly ONE replacement and ZERO queued scalar edits. Existing `v4::preview` remains blocked when any replacement is present.
- `phase18/move_replacement_v2.hpp`: verified packed exact-ROM catalogue loader/search interface. Full 936-record fingerprint remains:
  - ROM SHA256: `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`
  - 14040-byte [13 name bytes, PP A, PP B] × 936 metadata SHA256: `3d2d63e5fd77371a76540d07a6c5a4d20a9ddae7acea9f03035d86936ee34fa4`

**The 14,040-byte catalogue binary is generated LOCALLY from the uploaded CIA with**
`python3 phase18/tools/extract_2_1_2_move_catalog.py "<CIA>" --out <private.csv> --packed <private.bin>`
and placed in the app's SD-only resource directory. Do **NOT** commit or redistribute the CIA/ROM/catalogue bytes or embed the 32MiB ROM in the CIA; only the small SHA values are hardcoded. Phase20 must provide the *actual* authenticated catalogue bytes to the APIs for every selection and export.

## Production integration (NO L button required)

```cpp
#include "phase18/move_queue_v4.hpp"

const auto view = rogue18::v2::inspect(importedRaw,131072);
rogue18::move_queue_v4::Queue staged{};
if (!rogue18::move_queue_v4::begin(view,staged)) return;

// authenticatedCatalog is an immutable user-local 14040-byte buffer.
// Party index 0..5; occupied move index 0..3. New ID from the filtered
// move browser: move_v3::selectedOrdinary(id)==true.
if (!rogue18::move_v3::selectedOrdinary(newMoveId)) return;
if (!rogue18::move_queue_v4::appendReplacement(
      view,importedRaw,131072,authenticatedCatalog,staged,
      partyIndex,occupiedMoveIndex,newMoveId)) return;

const auto candidate = rogue18::move_queue_v4::previewSingleMoveDraft(
      importedRaw,131072,authenticatedCatalog,staged);
if (!candidate.ok) return; // show failure, never silently write

// User explicitly confirms EXPERIMENTAL OFFLINE DRAFT, NOT installed game.
// An alphanumeric token (e.g. "trial_001") must be unique, 1..24 chars.
// Create sdmc:/3ds/PKSMRogue/drafts folder before showing Export.
rogue18::move_export_v3::Receipt proof{};
const bool made = rogue18::move_queue_v4::exportSingleMoveDraft(
      importedRaw,131072,authenticatedCatalog,staged,"trial_001",proof);
if (!made || !proof.ok) return;
// Present proof.backupFile, proof.draftFile, proof.originalSha,
// proof.draftSha, proof.previousMove, proof.newMove, proof.changedBytes.
```

No `FSUSER_`, `FSPXI_`, `svcBackdoor`, VC save-restore, CMAC, Checkpoint in-place mutation, or any installed title access in this path. It only uses POSIX FAT-SD file APIs for separate backups/drafts. It does **not** select target by installed title ID. Menu input can be D-Pad/AB/Start, no shoulder L required.

## Initial move restrictions and status

There are **43 first-CIA permitted ordinary move IDs**, each confirmed against the *actual ROM's* two move tables with identical nonzero base PP: 7,8,9,14,33,53,56,57,73,85,89,94,126,127,157,182,183,188,202,229,242,245,247,337,347,349,369,370,396,403,405,406,412,417,418,434,442,444,483,521,542,583,585. Examples: Flamethrower (53/15), Earthquake (89/10), Dragon Dance (349/20), Bug Buzz (405/10), Quiver Dance (483/20), Moonblast (585/15). Selected IDs already occupying another move slot are rejected. **All 40 PP-disagreeing moves, Struggle, special moves and unreviewed IDs are blocked.** PP agreement is necessary but does **not** establish species move learnability or emulator gameplay behaviour. Don't describe these as legal-by-species moves or claim this feature is gameplay-certified.

The single-move code intentionally **clears only the selected move's 2-bit PP-Up lane and gives the new move the exact ROM's base PP**. This is a consistent, structurally verified *experimental* reset policy. Exact 2.1.2 ROM max-PP routine/PP-Up mask geometry has been traced, but not every learn/forget path. The game may retain moves with custom learnsets; user should test disposable drafts in an emulator before any possible transfer back.

## Acceptance tests and safety

CI verifies 24 encryption permutations × 6 party slots × 4 move slots = **576** synthetic replacements, 28 corrupted sectors, duplicate IDs, invalid catalog and source/counter/PP-Up guards, full 128KiB checksum validation, max 8 changed bytes, original untouched and equivalence with the earlier independent v2 research engine. Host tests create/reopen distinct `backup.sav` and `draft.sav` using `O_CREAT|O_EXCL`, refuse overwrites, validate both byte-for-byte and reject path traversal. Independent Phase9 Python reads the new move ID/PP and both save banks in the synthetic draft.

**Remaining before declaring safe to restore/play:** a real copied save from this user, emulator loading and re-saving an exported draft, final verification of installed game identity and the full move-learn/forget path. This PR **does not** install an actual CIA, change Phase20 PR #12, or authorize installed-game writes. The first CIA can expose `Preview` and `Export experimental offline draft`, but no `Apply to Game` action.
