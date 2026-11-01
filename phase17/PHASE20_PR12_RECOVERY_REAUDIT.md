# Phase 17 — integration safety re-audit (1 November 2026)

**Scope:** Phase 17 recovery verification only. Existing branch `feature/phase17-rollback-rehearsal`, draft PR #9; independent Phase 20 offline integration PR #12 is reviewed, **not modified**. No real save files/ROMs used, no branch merges, no native GBA VC writer enabled.

## Counter policy (QA-02)

The 263168-byte GBA VC full-container wrapper and 131072-byte H168 raw inner banks are **different counter domains**. Phase14 `layout()`, inherited by Phase17, requires **two valid headers, matching expected title/CID, adjacent nonwrapping wrapper counters and a safe next increment**. Greater adjacent wrapper counter selects active; other slot is staging target. No single-slot fallback. Refuse equal counters, skipped/stale pairs (including 48/60), `0/UINT32_MAX` rollover and `UINT32_MAX-1`/exhaustion. The physically observed pair 48/49 passes in both orientations. This is a conservative safety constraint, not proof the physical game always uses this counter interpretation, especially at rollover. Future restoration is still blocked.

## New November 1 safety additions on PR #9

- `phase17/recovery_evidence_gate.hpp`: strictly enumerates `RecordState` and rejects **unknown numerical enum values** (notably unexpected receipt-state corruption), unchecked or unreadable SD presence/read probes, and any missing required file. Validates full backup bounds, exact journal envelope size, live-image size before decoding journal; then checks journal integrity, immutable full backup equality, fresh complete live snapshot, signed plan and both authenticated save images. A positive result is **RAM-only rehearsal input**.
- `phase17/readonly_status_producer.hpp`: constructs status in a local temporary and only publishes after all shape checks pass; any late failure leaves blocked/not-checked output. The C status ABI contains **zero writable capability bits**, no transaction plan, archive handle, filesystem location, authenticated signer or editor. Arbitrarily forged v1 structures can look plausible; **shape validity is not attestation**, and must never grant write/restore permission.
- `phase17/rollback_lab.hpp`: synthetic read failure injection for fresh live reads; no native filesystem writing.
- `phase17/test_rollback_lab.cpp`: each of five independent read/presence-failure probes, forged unknown state values (including terminal receipts), truncated/oversize backup and journal, short live image, partial/failed read, interrupted recover/retry, failed completion receipts, foreign changes, malformed wrapper header/counter combinations, 32 distributed journal mutations and forged status bit flags. Every result must block unauthorised mutations; no test writes a native save.
- Existing `phase14/test_restore.cpp` and `phase17/run_tests.sh` test adjacent/equal/nonadjacent/rollover counter cases, signed-plan refusal, short writes, crash/restart and rollback interruption. Strict C++17 and ASan/UBSan run on synthetic nonplayable fixtures. ABI cross-compiles with separately pinned Phase18 headers; native 3DSX/ELF, isolated emulator and nonrelease CIA pipeline compile.

## Phase 20 PR #12 review: recovery assumptions

1. **Offline import ≠ authenticated container.** Phase20 edits a standalone copied **131072-byte raw H168 save**; Phase17 verifies a **263168-byte authenticated GBA VC container**. A raw copy/export is NOT a CMAC-valid container, not a directly installable game save, and is not a source of permission to invoke Phase14 restore. No audited raw-to-container editing bridge exists.
2. **SD backup semantics.** `backups/phase18_original_NNN.sav` is a no-overwrite copy of the current raw imported SD file on **the same card**, not an independent off-device backup of the whole installed save. It must not be presented as fulfilling the separate full-container recovery requirement. The Phase16 263168-byte backup and 526400-byte rehearsal journal are a different evidence set.
3. **Export readback and exact-byte verification** detect common stale or corrupt offline-draft errors, but do not establish power-loss durability, native 3DS SD hardware safety, in-game acceptance/re-save, or reliable recovery after torn privileged writes. The single-call SDMC raw file writer and Phase14's one-byte receipt are not production-safe restore facilities.
4. **Status boundary:** Phase20 currently initializes Phase17 status to NOT CHECKED and treats even a plausible positive status as a RAM-only **claim**, with `canRestore=false` and `canModifyInstalledGame=false`. That is appropriate. A caller-forged P17 status cannot legitimately override this boundary; keep it strictly read-only.
5. **Narrow Phase14 preflight:** only money +1 / existing item quantity +1 are in its old authenticated simulation policy, whereas Phase20 offline editing supports wider money/item amounts, IV, EV and PP. None may use Phase17 native restoration or skip separately reviewed per-field ROM identity and signed-transaction gates.

The above concerns were also posted to PR #12. The Phase20 branch remains untouched.

## Hardware record and unclosed release gates

**Prior physical 3DS owner evidence:** Phase16 read-only authentication plus immutable SD full-container backup (263168 bytes), Phase14-format rehearsal journal (526400 bytes), verify-after-application-restart, source wrapper counter 49 and older 48. Phase17 ran normal restore/undo 1/1, short writes 4/4, simulated crash restarts 4/4, rollback short retries 4/4, crash retries 4/4, foreign-change refusal 1/1, exact recovered bytes YES — **all in RAM** on the console. No hardware test was requested during this re-audit.

**Unverified:** privileged installed-game write rights, restoration of any real save, durable native journal and signed terminal receipts, SD/GBA power interruption, independent recovery off the same SD, actual AGB_FIRM load/re-save and rollback, exact matching ROM field semantics, and low-memory/stack exhaustion on hardware. Phase14 native journal uses one-byte unbound receipts and lacks a production-grade durability protocol. Native code can allocate several full-container copies and journals in heap memory, so a future memory-budgeted recovery backend and failure-safe allocation handling require separate review.

## Phase20 allowed consumption

**Allowed now:** `readonly_status_v1.h` (stable C value envelope), `readonly_status_v1.cpp` linker, public denial and RAM-only readable **status** codes, counter/size/source SHA-256 **claims** from a trusted read-only Phase17 producer, and synthetic fault-test results. Keep Phase17's implementation in a distinct translation unit from Phase18 to avoid duplicate `h168` types.

**Absolutely forbidden:** Phase14 `execute/recover` or `NativeBackend` wired to a Phase20 UI, `FSPXI_WriteFile` or a privileged RW handle, direct interpretation of a raw save as a wrapper, a live restoration capability bit, automatic journal replay or any assertion that a success status authorises installed-game writes.

**Safety invariant:** `phase14::LiveWritesEnabled=false`; no real GBA VC writes and no production release approval. PR #9 remains draft/unmerged.

### Verification links

- Previous exact Phase17 CI at commit `9908eab15a79c5b64126335d746b140028d05af1`: [all six jobs passed](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41327737955).
- Updated regression CI (newer recovery checks): [run 41338012591](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41338012591) — evaluate the current live outcome, including both sanitizer and emulator stages, before making a final pass claim.
- Independent Phase20 PR #12: https://github.com/hboots999-hash/Rogue-save-editor-/pull/12

This document is **a handoff, not authorisation to restore a game save**.
