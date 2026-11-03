# PKSM Rogue Phase 20 Integration QA Findings & Move-Replacement Research Audit

**Date:** 2 November 2026
**Auditor:** Independent C++ Integration QA Engineer
**Target Branch:** Clean QA Pull Request targeting `feature/phase20-readonly-integration`
**Base Commit:** `7ddd7a20e4ec6b9a6294f7b060754956da722be9`
**Referenced Workflows & PRs:**
- Phase 20 Integration PR: [#12](https://github.com/hboots999-hash/Rogue-save-editor-/pull/12)
- Move Replacement Research PR: [#14](https://github.com/hboots999-hash/Rogue-save-editor-/pull/14)
- Phase 18 Integration PR: [#8](https://github.com/hboots999-hash/Rogue-save-editor-/pull/8)
- Phase 17 Recovery PR: [#9](https://github.com/hboots999-hash/Rogue-save-editor-/pull/9)
- Independent QA Baseline PR: [#10](https://github.com/hboots999-hash/Rogue-save-editor-/pull/10) (Supersedes unmerged #13)

---

## 1. Executive Summary & Audit Scope

This report documents an independent quality assurance audit and integration assessment covering:
1. **Assignment 1 (Clean QA Pull Request):** Cleaning PR #13 to ensure only independent QA additions (`phase20/QA_FINDINGS_REPORT.md`, `phase20/test_read_only_v1.cpp`, `phase20/test_offline_edit_v1.cpp`) are submitted against `feature/phase20-readonly-integration` without historical file pollution.
2. **Assignment 2 (Audit of Move Replacement Research):** Technical audit of `phase18/move_replacement_v1.hpp`, `phase18/move_replacement_v2.hpp`, `phase18/move_queue_v4.hpp`, `phase18/MOVE_REPLACEMENT_RESEARCH.md`, and exact-ROM metadata evidence from PR #14.
3. **Move Table & Conflict Analysis:** Review of the 936-record move-name table (offset `0x004C283C`, stride 13) and two 28-byte battle move tables (`0x00BD27D0`, `0x00BD8E30`), including the 40 conflicting base-PP entries.
4. **Encoding, Checksums & PP-Up Rules:** Verification of encrypted A/G substructure encoding, 24 Gen III permutations, 6 party slots, 4 move slots, PP-Up bitwise masking, and byte-exact footprint limits.
5. **Release Blockers & Multi-Edit Integration Hazards (Assignment 3):** Hazards of integrating move replacement into Phase 20's v3/v4 multi-edit queues, memory safety, and strict write-prohibition gates.

---

## 2. Independent Audit of Move Replacement Engine (`phase18/move_replacement_v1.hpp` / `v2.hpp`)

### 2.1 Production Capability Gate Enforcement
- In `rogue18::move_v1`, production preview is unconditionally blocked:
  - `capability()` returns `mayPreviewReplacement = false`, `mayExportReplacement = false`, and `mayWriteInstalledGame = false`.
  - `ExactRomMoveTableApproved` is `false`.
  - `preview()` immediately returns `blocked("Move replacement locked: installed Rogue 2.1.2 ROM move table not authenticated")`.
  - `verify()` unconditionally returns `false`.
- In `rogue18::move_queue_v4`, if `queue.replacements` is non-empty, `preview()` immediately fails closed: `blocked("Move replacement queue staged only: exact-ROM Apply gate disabled")`.

### 2.2 Exact-ROM 2.1.2 Metadata & 40 Disagreeing Move PP Entries
- Inspection of the GodMode9 CIA extract confirmed the 32 MiB GBA ROM SHA-256 is:
  `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`
- Move name table (936 entries, stride 13) is at offset `0x004C283C` (SHA-256: `e91817b5...`).
- Two compiled battle move data variants exist in ROM:
  - Variant A at `0x00BD27D0` (SHA-256: `b68bb9f4...`)
  - Variant B at `0x00BD8E30` (SHA-256: `323c8d3d...`)
- **40 Ordinary Move IDs Have Conflicting Base PP:** Across the two tables, 40 move IDs have different base PP values at offset +5 (e.g., ID 58 *Ice Beam* has 10 PP in Variant A vs. 15 PP in Variant B). Because the runtime selection between these tables is unverified, guessing or choosing one variant arbitrarily is prohibited.
- **807 Read-Only Catalogue Candidates:** Excluding Struggle (ID 165) and entries with variant disagreement, 807 ordinary entries have identical positive base PP across both tables. These are verified as read-only candidate entries, not an authorization to write.

### 2.3 Pokémon Encoding, Checksum Regeneration & PP-Up Rules
- **Encrypted Substructures:**
  - Moves and current PP are located in substructure **A** (four 16-bit move IDs followed by four 8-bit PP values).
  - PP-Up bonus counters are located in substructure **G** at byte +8 (`ppBonuses`), storing four 2-bit counters (0–3 each, 2 bits per move slot).
- **PP-Up Reset Policy:** The research engine resets only the target slot's 2-bit bonus lane (`ppBonuses &= ~(3 << (2 * slot))`) and initializes new PP to the base PP from the test table. Other move slots retain their existing PP-Up bonuses.
- **Exact Byte-Change Footprint:** The candidate verification strictly enforces that no more than 8 specific bytes change:
  - 2 bytes: new encrypted move ID in substructure A;
  - 1 byte: new encrypted PP in substructure A;
  - 1 byte: modified encrypted PP-Up bonus in substructure G;
  - 2 bytes: recalculated Pokémon 48-byte payload checksum;
  - 2 bytes: recalculated active save sector 4096-byte checksum.
  Any alteration outside these 8 bytes causes immediate verification failure.
- **Full Coverage of Permutations:** 576 synthetic tests exhaustively verify all 24 personality order permutations (`GAEM`, `GAME`, `MGEA`, etc.) across all 6 party positions and all 4 move slots under AddressSanitizer and UndefinedBehaviorSanitizer.

---

## 3. Verified Read-Only & Offline Editor Findings (Phase 20)

### 3.1 ARM11 Stack & Heap Safety
- Compiler flags `-Werror=frame-larger-than=4096 -fstack-usage` are verified on ARM11 builds.
- Largest frame in Phase 20 is `phase20::inspectCopy` at 1,064 bytes, and `main` at 232 bytes.
- Frame usage for move replacement research (`previewResearch`: 784 bytes; `v3::preview`: 792 bytes; `v3::exportCopiedBatch`: 552 bytes) is well below the 4,096-byte hardware stack cap.
- Standard-library and dynamic allocation exceptions are caught at the UI boundary, preventing unhandled exceptions from terminating the 3DS event loop.

### 3.2 Read-Only Save Parsing Stress Tests
Tests in `phase20/test_read_only_v1.cpp` independently verify:
- Immediate rejection of null pointers, zero-length buffers, and malformed buffer sizes (1, 512, 4096, 65536, 131071, 131073, 263168, 526400 bytes).
- Full rejection of sector checksum corruption across all 28 bank sectors (both content bodies and `0xFF6` footer checksums).
- Rejection of invalid Pokémon structures (`partyCount > 6` or corrupted block checksums).
- Stream I/O failures (null streams, 0-byte streams, and truncated files fail closed, leaving visible snapshots unchanged).

### 3.3 Offline Editor Bounds & Multi-Edit v3 Safety
Tests in `phase20/test_offline_edit_v1.cpp` verify:
- Boundary conditions across all five edit operations: Money (`0..999,999`), Existing Items (`1..999`), IVs (`0..31`), EVs (`0..252`, `total <= 510`), and PP (`0..64`).
- Rejection of out-of-bounds party slots, invalid bag slots, and empty move slots (`moveId == 0`).
- Multi-edit v3 transaction safety: five simultaneous edits (Money, Item, IV, EV, PP) correctly coexisting in a single batch locked to the same SHA-256 source digest.
- UI cancellation flow: cancellation via Key::B at `ConfirmOne`, `ConfirmTwo`, and `Value` stages, and reset to original via Key::X.

---

## 4. Analysis of Permitted SDMC Operations vs. Prohibited Installed Writes

| Operation Category | Specific Mechanism | Current Status | Boundary & Safety Policy |
| :--- | :--- | :--- | :--- |
| **Permitted SDMC Drafts** | `FSUSER_OpenArchive(ARCHIVE_SDMC)` + `FSUSER_CreateFile` with `O_CREAT \| O_EXCL` | **ACTIVE & VERIFIED** | SD-only file creation in `/backups/` and `/drafts/`. The source import remains read-only. |
| **Prohibited Installed Game Saves** | `FSUSER_OpenFileDirectly(ARCHIVE_SAVEDATA)` / `ARCHIVE_USER_SAVEDATA` | **ABSENT & PROHIBITED** | No live GBA VC save title archive handle is opened or requested. |
| **Prohibited Live Restoration** | CMAC signing, title injection | **ABSENT & PROHIBITED** | Flags `canModifyInstalledGame` and `canRestore` are hardcoded to `false`. |
| **Prohibited System Calls** | `FSFILE_Write` to non-SDMC handles, `FSPXI_`, `AGBSAVE` | **ABSENT & PROHIBITED** | Verified absent from executable symbol table. |
| **Prohibited Move Overwrites** | Unauthenticated move replacement | **BLOCKED & GATED** | Production move preview/export is unconditionally blocked until exact-ROM gates pass. |

---

## 5. Review of Emulator Evidence & Hardware Limitations

### 5.1 Azahar CI Verification (Run #41338410580 & #41349356154)
- Verified that the real `PKSMRoguePhase20.3dsx` ran in Azahar 2126.2 under Xvfb.
- Followed on-screen inputs to validate a synthetic H168 save, adjust money (+1), and test native batch queue flows, outputting verified drafts.
- `verify_emulator_draft.cpp` confirmed that drafts matched authorized edits with valid save checksums.

### 5.2 Emulation vs. Physical Hardware Limitations
- **No Physical Hardware Declaration:** Emulators do not accurately simulate physical SD card controller latencies, NAND bus contention, power interruption during flash writes, or AGB_FIRM hardware save handoffs.
- **Hardware Abort Risk:** Phase 19 historically encountered a data-abort crash on physical hardware caused by stack pressure. While Phase 20 maintains small stack frames (`-Werror=frame-larger-than=4096`), physical 3DS execution on real hardware has **not** been performed.

---

## 6. Hazards When Integrating Move Replacement into Phase 20 v3/v4 Queue

1. **Substructure Checksum Invalidation in Queues:** In v3, editing PP is a scalar change in substructure A. However, replacing a move alters both substructure A (move ID, PP) and substructure G (PP-Up bonuses), which invalidates the Pokémon payload checksum. Staging multiple edits on the same Pokémon requires updating the in-memory intermediate decrypted representation rather than assuming initial original checksums.
2. **Move Table Ambiguity (40 PP Discrepancies):** The 40 moves with differing base PP across ROM tables must not be taught or offered for replacement until the runtime engine mode (or exact variant) is resolved.
3. **Queue Cancellation & Staged Revert:** If a user cancels a move replacement in the queue, both the substructure A and G modifications must be rolled back simultaneously.
4. **Cached Battle Structs in Save:** Emerald Rogue maintains in-battle or run-specific state caches. Changing a move ID without updating corresponding cached structs could cause game desynchronization upon save load.
5. **No Installed Game Writing:** Move replacement must remain an SDMC-only draft export. Under no circumstances should live GBA VC save restoration be authorized for move-replaced saves.

---

## 7. Deliverables & Integration Instructions

- **Superceded PR:** PR #13 (unmerged, identified as superseded due to base branch file pollution).
- **Clean PR Branch:** `feature/phase20-qa-improvements-9065036775014488963` rebased cleanly on `feature/phase20-readonly-integration` HEAD (`7ddd7a20e4ec6b9a6294f7b060754956da722be9`).
- **Clean PR Scope:** Strictly contains 3 files:
  - `phase20/QA_FINDINGS_REPORT.md`
  - `phase20/test_read_only_v1.cpp`
  - `phase20/test_offline_edit_v1.cpp`
