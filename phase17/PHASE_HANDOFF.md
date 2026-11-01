# Phase 17 handoff — RAM-only restore and rollback rehearsal

**Date:** 1 November 2026  
**Branch:** `feature/phase17-rollback-rehearsal`  
**Base:** `feature/phase16-recovery-journal-rehearsal`  
**Scope:** Physical hardware verification of a purely in-memory mock of the Phase 14 transaction engine, reading the immutable Phase 16 SD journal and current installed game via read-only access. **No installed-game writes.**

## Proven inherited Phase 16 physical evidence

The user already demonstrated both on the real 3DS:
- `SD JOURNAL VERIFIED`: full untouched 263168-byte GBA VC container backup, 526400-byte Phase 14-format recovery journal, live counter 49, current game unchanged.
- `RESTART VERIFICATION PASS`: independent journal/backup readback after exiting/relaunching the app; live data still unchanged.
These are not proof of power-cut durability, direct restore, game acceptance, or rollback after a real write.

## This branch

- `phase17/rollback_lab.hpp` implements an isolated `Shadow` IO, with **only heap-vector writes**, fake durable journal within RAM, unchanged source proof and no system filesystem access.
- The Phase 14 `execute` and `recover` state machines are exercised with current signed plans. Cases: successful full shadow restore/explicit rollback, partial short write at each of four substeps, simulated abrupt interruption at each of four substeps followed by recovery through a new IO instance, and a foreign byte mutation requiring recovery refusal.
- `phase17/test_rollback_lab.cpp`: synthetic nonplayable two-slot H168 fixtures with both slot directions, money+1/existing-item+1; journal corruption, truncation, original mismatch and MAC failure. `phase17/run_tests.sh` also runs Phase 16/15/14 and older regressions under strict compiler flags and ASan/UBSan.
- `phase17/main.cpp` reads the protected Phase 16 SD records and twice verifies the installed live game through a **read-only** console service before running a RAM-only rollback rehearsal against the user-specific signed plan. No file creation, file overwriting, import or restore UI exists.
- `phase17/rsf` uses **separate experimental title `000400000F73A600`**. It cannot replace the Emerald Rogue title `0004000007D7BA00` or Phase 16 experimental title `000400000F73A400`.
- `.github/workflows/phase17-rollback-rehearsal.yml` builds native devkitPro 3DSX/CIA and checks Azahar UI start; Azahar cannot verify real PxiFS0 authentication.
- Complete physical steps and privacy/limitations: `phase17/README.md`.

## Evidence and gates

- Host strict and sanitized tests: **check latest CI** at [Phase 17 Actions](https://github.com/hboots999-hash/Rogue-save-editor-/actions/workflows/phase17-rollback-rehearsal.yml).
- Real hardware Phase 17 shadow simulation: **not yet tested**. No claim of an actual installed-game restore; game-write gate remains hardcoded false in the inherited Phase 14 adapter.
- Existing Phase 16 files contain personal save data and are NEVER committed or uploaded. Phase 17 reads them unchanged.
- The next development phase is an independently reviewed, reproducible, *disposable-game-progress* authenticated write/rollback pilot. It needs prewrite independent backup and authorisation, durable multi-file journal, a fail-closed recovery path, AGB_FIRM/game-relaunch verification and actual rollback. A RAM simulation PASS cannot unlock direct writes.

## Branch integration

This feature depends on Phase 16/15/14 draft branches. Keep its PR draft and do not cherry-pick around recovery gates, merge into main, or alter Phase 18/19 work. Record actual physical console evidence in this handoff only after photographed results are provided.
