# PKSM Rogue Phase 20 Integration QA Findings & Independent Audit Report

**Date:** 1 November 2026
**Auditor:** Independent C++ Integration QA Engineer
**Target Branch:** Clean QA Pull Request targeting `feature/phase20-readonly-integration`
**Base Commit:** `7ddd7a20e4ec6b9a6294f7b060754956da722be9`
**Referenced Workflows & Issues:** CI Run [#41338410580](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41338410580), Draft PR #12, Superseded Draft PR #13, Phase 18 PR #8 (v3 multi-edit @ `3625e160`), Phase 19 PR #7, Phase 17 PR #9

---

## 1. Executive Summary & Audit Scope

This report documents an independent quality assurance audit and integration assessment of PKSM Rogue Phase 20 across its read-only architecture, single-edit offline draft editor, and the newly available Phase 18 multi-edit v3 transaction engine:
1. **Assignment 1 (Clean QA Pull Request):** Retargeted and isolated PR #13 to ensure only independent QA additions (`phase20/QA_FINDINGS_REPORT.md`, `phase20/test_read_only_v1.cpp`, `phase20/test_offline_edit_v1.cpp`) are submitted against `feature/phase20-readonly-integration` HEAD without any extraneous historical files.
2. **Assignment 2 (Independent Audit of Multi-Edit v3):** Evaluated `phase18/multi_edit_v3.hpp`, `phase18/multi_export_v3.hpp`, and Phase 20 host integration tests for combined Pokémon IV/EV, bag item, PP, and money transactions.
3. **Assignment 3 (Release Blockers & 3DS Menu Integration Risks):** Analyzed native UI considerations for batch editing, including cancellation semantics, double confirmation, edit-queue display, memory limits, and prohibited live write paths.

---

## 2. Verified Findings

### 2.1 Multi-Edit v3 Transaction Safety & Coexistence
- **Coexistence of Edits:** Verified that multiple Pokémon IV and EV modifications, item quantity changes, move PP changes, and money updates safely coexist within a single `rogue18::v3::Batch` transaction.
- **SHA-256 Lock & Stale Edit Rejection:** Every edit appended to a batch is cryptographically bound to the same `sourceSha256` and `activeCounter`. Transplanted edits from different saves or stale snapshots are rejected.
- **Duplicate & Conflicting Edit Prevention:** `rogue18::v3::append` checks `sameField(previous, e)` and rejects duplicate edits targeting the same `(kind, slot, field)` tuple.
- **Strict EV Aggregate Bounds:** EV changes defensively enforce both individual stat caps (`0..252`) and total party member EV caps (`sum(EVs) <= 510`). Even when multiple EV increases and decreases are combined, `v3::preview` stable-sorts decreases before increases to prevent intermediate candidate overflow.
- **Gen III Encryption & Party Position Agility:** The underlying v2/v3 engine accurately handles all 24 personality-based substructure order permutations across all 6 party slots.
- **Fail-Closed SD Storage & Immutability:** The original imported save `/3ds/PKSM-Rogue/imports/00000001.sav` is never modified or overwritten. If memory allocation fails or an SD write error occurs, the batch fails closed without creating an approved draft.
- **ARM11 Stack Frame Suitability:** Frame sizes for `v3::preview` (792 bytes) and `v3::exportCopiedBatch` (552 bytes) remain well under the 4,096-byte ARM11 hardware limit (`-Werror=frame-larger-than=4096`).

### 2.2 Read-Only Copied-Save Parsing Robustness
Expanded synthetic tests in `phase20/test_read_only_v1.cpp` independently verify:
- Immediate rejection of null pointers, zero-length buffers, and malformed buffer sizes (1, 512, 4096, 65536, 131071, 131073, 263168, 526400 bytes).
- Comprehensive detection of sector checksum corruption across all 28 bank sectors (both content bodies and `0xFF6` footer checksums).
- Detection of corrupted Pokémon structures (`partyCount > 6` or invalid block checksums).
- Robust stream failure handling (null streams, 0-byte streams, and truncated files fail closed, leaving visible snapshots unchanged).

### 2.3 Single-Edit Offline Editor Boundaries
Expanded synthetic tests in `phase20/test_offline_edit_v1.cpp` verify:
- Boundary conditions across all five edit operations: Money (`0..999,999`), Existing Items (`1..999`), IVs (`0..31`), EVs (`0..252`, `total <= 510`), and PP (`0..64`).
- Rejection of out-of-bounds party slots, invalid bag slots, and empty move slots (`moveId == 0`).
- UI navigation flow: cancellation via Key::B at `ConfirmOne`, `ConfirmTwo`, and `Value` stages, and reset to original via Key::X.

---

## 3. Analysis of Permitted SDMC Operations vs. Prohibited Installed Writes

| Operation Category | Mechanism / API | Status | Safety & Boundary Policy |
| :--- | :--- | :--- | :--- |
| **Permitted SDMC Drafts** | `FSUSER_OpenArchive(ARCHIVE_SDMC)` + `FSUSER_CreateFile` | **ACTIVE & VERIFIED** | SD-only file creation in `/backups/` and `/drafts/`. The source import remains read-only. |
| **Prohibited Installed Game Saves** | `FSUSER_OpenFileDirectly(ARCHIVE_SAVEDATA)` | **ABSENT & PROHIBITED** | No live GBA VC save title archive handle is opened or requested. |
| **Prohibited Live Restoration** | CMAC signing, title injection | **ABSENT & PROHIBITED** | Flags `canModifyInstalledGame` and `canRestore` are hardcoded to `false`. |
| **Prohibited System Calls** | `FSFILE_Write` to non-SDMC handles, `FSPXI_`, `AGBSAVE` | **ABSENT & PROHIBITED** | Verified absent from executable symbol table. |

---

## 4. Review of Emulator Evidence vs. Physical Hardware

### 4.1 Azahar CI Verification (Run #41338410580)
- Verified that the real `PKSMRoguePhase20.3dsx` ran in Azahar 2126.2 under Xvfb.
- Followed on-screen inputs to validate a synthetic H168 save, adjust money (+1), complete double confirmation, and output `phase18_original_001.sav` and `phase18_draft_001.sav`.
- `verify_emulator_draft.cpp` confirmed that the draft matched the single authorized edit with valid save checksums.

### 4.2 Hardware Limitations
- **Emulator Results Are Not Hardware Proof:** Emulator execution does not prove physical flash durability, NAND bus contention safety, or AGB_FIRM hardware save handling.
- **Physical 3DS Hardware Gate:** Physical testing on real 3DS hardware with expendable media has not yet been executed.

---

## 5. Release Blockers for v3 Batch API Integration into 3DS Menus

When integrating the multi-edit v3 engine into the native 3DS interactive UI, the following blockers must be addressed:
1. **Interactive Batch Queue & Cancellation Semantics:** The current native UI only supports an immediate single edit (`OfflineFlow`). A multi-edit workflow requires a staged batch queue UI where users can view pending edits, delete individual queued items, or cancel the entire batch without partial execution.
2. **Double Confirmation of the Aggregate Batch:** Before invoking `v3::exportCopiedBatch`, both confirmation screens (`ConfirmOne` and `ConfirmTwo`) must present a complete scrollable summary of all staged modifications (before -> after values, affected party/bag slots, and total EV allocations).
3. **Source Invalidation During Batch Staging:** If the underlying import file on the SD card is modified or deleted while a user is staging a multi-edit batch in RAM, the exporter must detect the SHA-256 fingerprint mismatch and refuse export, clearing the batch safely.
4. **ARM11 Heap Exhaustion on Large Batches:** While the stack usage is low, running v3 multi-edit preview repeatedly allocates intermediate candidate buffers and diff footprints. On 3DS systems with limited available linear heap, batch sizes should be conservatively capped (e.g., maximum 16–32 edits in native UI) with explicit `std::bad_alloc` handlers.
5. **Clear Error Reporting for Failed Batches:** If batch export fails midway, the UI must explicitly indicate that no approved draft was generated, list any uncertain partial files, and preserve the original save untouched.

---

## 6. Deliverables & Integration Instructions

- **Superceded PR:** PR #13 (unmerged, identified as superseded due to base branch file pollution).
- **Clean Branch:** `feature/phase20-qa-improvements` created directly from `feature/phase20-readonly-integration` HEAD (`7ddd7a20e4ec6b9a6294f7b060754956da722be9`).
- **Clean PR Scope:** Strictly contains 3 files: `phase20/QA_FINDINGS_REPORT.md`, `phase20/test_read_only_v1.cpp`, and `phase20/test_offline_edit_v1.cpp`.
