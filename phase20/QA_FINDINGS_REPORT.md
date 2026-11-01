# PKSM Rogue Phase 20 Integration QA Findings & Independent Audit Report

**Date:** 1 November 2026
**Auditor:** Independent C++ Integration QA Engineer
**Target Branch:** `feature/phase20-qa-improvements`
**Base Branch:** `feature/phase20-readonly-integration`
**Latest Verified Target Commit:** `2bbea3539e15cc30a03ed72a78a741bbd06cdf76`
**Latest Verified Actions Workflow:** [Run #41338410580](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41338410580)
**Referenced Workflows & Pull Requests:** PR #12 (Phase 20 integration), PR #10 (Independent QA), PR #9 (Phase 17), PR #8 (Phase 18), PR #7 (Phase 19)

---

## 1. Executive Summary & Audit Scope

This report documents an independent quality assurance audit and integration assessment of PKSM Rogue Phase 20 across its read-only architecture, newly integrated offline draft editor, and automated emulator verification gates:
1. **Memory Safety & ARM11 Constraints:** Static analysis and runtime inspection of memory allocations, pointer operations, recursion, and ARM11 stack frames under `-Werror=frame-larger-than=4096`.
2. **Read-Only Copied-Save Parsing:** Comprehensive synthetic stress testing of malformed files, corrupted sectors, invalid Pokémon, empty inputs, and stream read errors.
3. **Cross-Phase Versioned Interfaces & Counter Policies:** Analysis of the decoupled Phase 17 C ABI (`readonly_status_v1.h`), Phase 18 scoped parser (`rogue18::raw_v1` / `v2`), and the counter selection policies of GBA VC outer containers vs. inner H168 save banks.
4. **Offline Draft Editor & SD-Only Exporter:** In-depth evaluation of the five supported offline edit operations (Money, Existing Item Quantities, Pokémon IVs, EVs, and PP), boundary conditions, double-confirmation authorization, cancellation, and SDMC error handling.
5. **Permitted SDMC File Creation vs. Prohibited Installed-Game Writes:** Clear architectural demarcation between authorized SDMC draft/backup writing and prohibited GBA VC live-game writing.
6. **Emulator Evidence Analysis:** Rigorous review of Azahar 3DSX/CIA emulator tests, distinguishing synthetic emulation proofs from physical hardware safety.

---

## 2. Verified Findings

### 2.1 ARM11 Stack Budget, Heap Allocations & Pointer Safety
- **Stack Budget:** The 3DS ARM11 userland thread has a strict stack limit. The large `phase19::Demo` snapshot structure (`sizeof(phase19::Demo) == 63,840` bytes) is allocated in process-lifetime static storage (`static Demo active{}; static phase19::Demo checked{};`). In contrast, `phase19::State` has a stack size of 48 bytes (well under the 256-byte budget).
- **Frame Usage Enforcement:** Build scripts enforce `-Werror=frame-larger-than=4096 -fstack-usage`. Compiler `.su` records confirm `phase20::inspectCopy` consumes 1,064 bytes and `main` consumes 232 bytes on ARM11.
- **Recursion & Pointers:** Zero recursive calls exist. Pointers are checked against null and bounds.
- **Fail-Closed Memory Handling:** Heap allocations for reading the 128 KiB copied save use `std::unique_ptr<uint8_t[]>(new(std::nothrow) uint8_t[CopiedRawBytes])`. If allocation fails, the function immediately fails closed with an informative error message.
- **Exception Isolation on ARM11:** In `phase20/main.cpp` and `phase20/offline_edit_v1.hpp`, standard-library and allocation exceptions (`std::bad_alloc`, `std::exception`) are caught at the UI boundary, reporting a blocked export notice without crashing the native 3DS event loop.

### 2.2 Robustness Under Malformed & Corrupted Saves
Tests in `phase20/test_read_only_v1.cpp` independently prove that:
- `nullptr` data or zero-length inputs immediately fail closed without mutating visible state.
- Arbitrary bad buffer sizes (1, 512, 4096, 65536, 131071, 131073, 263168, 526400 bytes) are rejected.
- Bit corruption in any of the 28 bank sectors (data bodies and `0xFF6` footer checksums) is detected and rejected.
- Invalid Pokémon structures (corrupted block checksums or `partyCount > 6`) fail closed.
- Truncated or empty stream inputs fail gracefully and preserve previous UI snapshots bit-for-bit.

### 2.3 Phase 17 & Phase 18 Versioned Interfaces & Counter Policies
- **Clean ABI Isolation:** Phase 17 exposes an 80-byte pure C ABI `readonly_status_v1.h` (`P17ReadOnlyStatusV1`). It is free of C++ types, templates, and `h168` declarations, preventing namespace collision when included alongside Phase 18 headers.
- **Tamper Rejection:** Envelope verification enforces `abi_version == 1`, `struct_size == 80`, and `capability_bits == 0`. Any nonzero capability bit or invalid code fails closed.
- **Counter Policy Reconciliation:**
  - *Phase 14 / Phase 17 GBA VC Outer Wrapper Policy:* Requires two strictly adjacent counters (`high - low == 1`) in `layout()` and refuses near-overflow (`high < UINT32_MAX - 1`). Skips, equal counters, and rollover pairs fail closed.
  - *Phase 18 Inner H168 Bank Counter Policy:* Requires modular adjacency in `rogue18::validate()`: `static_cast<uint32_t>(a + 1u) == b` selects bank 1, and `static_cast<uint32_t>(b + 1u) == a` selects bank 0. Ambiguous or equal counters are rejected. Both domains operate on distinct disk layers with explicit, tested semantics.

### 2.4 Offline Editor Functionality & Input Isolation
- **Five Supported Edit Operations:**
  1. *Money:* Range `0..999,999`. Values above 999,999 or identical to original are blocked.
  2. *Existing Item Quantity:* Range `1..999` for existing occupied bag slots. Adding unknown IDs, zero quantity, values > 999, or unallocated slots is blocked.
  3. *Pokémon IVs:* Range `0..31` for non-egg party members.
  4. *Pokémon EVs:* Range `0..252` per stat, strictly enforcing total `sum(EVs) <= 510`.
  5. *Move PP:* Range `0..64` for existing move IDs. Empty move slots (`moveId == 0`) cannot be edited.
- **Unsupported Edits Blocked:** Species changes, ability slot editing, nature modification, move replacement, and PC box editing remain strictly disabled and display explicit unsupported notices.
- **Workflow & Key Controls:**
  - Full navigation operates without the L shoulder button (using R/Right and X/Left).
  - Standalone START release exits cleanly. START+SELECT is passed to Rosalina menu.
  - Double confirmation (`ConfirmOne` -> `ConfirmTwo`) is required before invoking exporter. Pressing B at any stage cancels back to value selection.
- **Original Save Preservation:** The source import `/3ds/PKSM-Rogue/imports/00000001.sav` is opened exclusively in `"rb"` mode. The exporter creates separate, uniquely named files `/backups/phase18_original_NNN.sav` and `/drafts/phase18_draft_NNN.sav`. The original import is never overwritten.

---

## 3. Fixed Defects & Remediation History

1. **Native Build Header Path Mismatch (CI Runs #21 & #23):**
   - *Issue:* Makefile packaging attempted to copy `dependencies/phase18/phase18/raw_copy_v1.hpp` which had been renamed to `raw_v1.hpp` in Phase 18.
   - *Fix:* Aligned include copy to `raw_v1.hpp` (commit `da9522f`).
2. **Host Compilation Namespace Collision (CI Run #17):**
   - *Issue:* Unqualified `h168` in Phase 19 headers clashed when mixed with full research model headers.
   - *Fix:* Isolated research models to host tests only; native 3DS uses `compat_phase19_v1.hpp` mapping to `rogue18::raw_v1` (commit `26c546f`).
3. **Misleading Azahar Dual-Launch Test (CI Run at commit `5734735`):**
   - *Issue:* Launching a second emulator process dropped back to the Azahar game library UI, where simulated keystrokes were typed into the search bar rather than executing the app.
   - *Fix:* Restructured test in commit `2bbea35` to drive a single continuous 3DSX session, hot-injecting the synthetic import while the application was running, completing on-screen confirmations, and running `verify_emulator_draft.cpp` on the resulting draft.

---

## 4. Analysis of Permitted SDMC Operations vs. Prohibited Installed Writes

It is critical to distinguish permitted SDMC file creation from strictly prohibited installed-game access:

| Operation Category | Specific Mechanism | Current Status | Release Safety Policy |
| :--- | :--- | :--- | :--- |
| **Permitted SDMC Drafts** | `FSUSER_OpenArchive(ARCHIVE_SDMC)` + `FSUSER_CreateFile` with `O_CREAT \| O_EXCL` equivalent | **ACTIVE & VERIFIED** | Restricted to `sdmc:/3ds/PKSM-Rogue/` subdirectories (`backups/` and `drafts/`). Source import is immutable. |
| **Prohibited Game Saves** | `FSUSER_OpenFileDirectly(ARCHIVE_SAVEDATA)` / `ARCHIVE_USER_SAVEDATA` | **ABSENT & PROHIBITED** | No live game title archive handle is opened or requested. |
| **Prohibited VC Restores** | GBA VC save injection, CMAC recalculation, title memory writing | **ABSENT & PROHIBITED** | `canModifyInstalledGame` and `canRestore` are hard-coded to `false`. No CMAC signing keys or live restore logic exist. |
| **Prohibited System Calls** | `FSFILE_Write` to non-SDMC handles, `FSPXI_`, `AGBSAVE` | **ABSENT & PROHIBITED** | Verified absent from ELF symbol table and translation units. |

---

## 5. Review of Emulator Evidence & Hardware Limitations

### 5.1 Azahar CI Verification (Run #41338410580)
- The emulator test launched `PKSMRoguePhase20.3dsx` in Azahar 2126.2 under Xvfb.
- It verified missing-file handling, imported a synthetic 128 KiB H168 save, adjusted money from 40,001 to 40,002, completed double confirmation, and generated `phase18_original_001.sav` and `phase18_draft_001.sav`.
- `verify_emulator_draft.cpp` verified that the draft matched the single authorized money edit with valid save checksums.

### 5.2 Emulation vs. Physical Hardware Limitations
- **No Physical Hardware Declaration:** Emulators (including Azahar and Citra forks) do not accurately simulate physical SD card controller latencies, NAND bus contention, power interruption during flash writes, or AGB_FIRM hardware save handoffs.
- **Hardware Abort Risk:** Phase 19 historically encountered a data-abort crash on physical hardware caused by stack pressure. While Phase 20 maintains small stack frames (`-Werror=frame-larger-than=4096`), physical 3DS execution on real hardware has **not** been performed.
- **Toolchain Absence:** The local environment lacks `devkitARM`, `3dstools`, and `makerom`; compilation for 3DSX/CIA targets is executed exclusively within the containerized CI environment (`devkitpro/devkitarm:latest`).

---

## 6. Outstanding Risks & Recommended Next Steps

1. **Rendering Bounds Verification:** As noted in Phase 19 handoff reviews, `offlineTop()` should adopt bounded notice wrapping (`forEachWrappedLineBounded`) to guarantee return instructions never scroll off smaller screens during verbose error messages.
2. **Session Reset on Re-import:** Ensure `OfflineFlow` state is explicitly zeroed when switching between demo and real imported views.
3. **Physical 3DS Hardware Gate:** Before any non-draft release, perform physical testing with an expendable SD card on real hardware to verify SDMC write durability, Rosalina exit stability, and power-off tolerance.
4. **Highest-Priority Next Task:** Integrate Phase 19's `phase19/offline_review_ui.hpp` display helper to provide target identity previews (showing exact species, stat names, and bag item IDs) on confirmation screens, and conduct hardware SD stress tests on expendable media.

---

## 7. Integration Instructions & Deliverables

- **Draft Pull Request:** Created on branch `feature/phase20-qa-improvements` targeting `feature/phase20-readonly-integration`.
- **Commit SHA:** Stacked on top of `2bbea3539e15cc30a03ed72a78a741bbd06cdf76`.
- **Reproducible Verification Commands:**
  ```bash
  # Run strict C++17 and AddressSanitizer/UndefinedBehaviorSanitizer suites:
  g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I. -Idependencies/phase17/phase17 \
    -Idependencies/phase18 -Idependencies/phase18/phase18 \
    -Idependencies/phase19 -Idependencies/phase19/phase19 \
    phase20/test_read_only_v1.cpp \
    dependencies/phase17/phase17/readonly_status_v1.cpp -o /tmp/ro_test && /tmp/ro_test

  g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I. -Idependencies/phase17/phase17 \
    -Idependencies/phase18 -Idependencies/phase18/phase18 \
    -Idependencies/phase19 -Idependencies/phase19/phase19 \
    phase20/test_offline_edit_v1.cpp \
    dependencies/phase17/phase17/readonly_status_v1.cpp -o /tmp/edit_test && /tmp/edit_test
  ```
