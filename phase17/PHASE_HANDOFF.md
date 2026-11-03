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
- Real hardware Phase 17 RAM-only shadow simulation: **PASS, 1 November 2026** (user-supplied physical 3DS photos; details below). No claim of an actual installed-game restore; game-write gate remains hardcoded false in the inherited Phase 14 adapter.
- Existing Phase 16 files contain personal save data and are NEVER committed or uploaded. Phase 17 reads them unchanged.
- The next development phase is an independently reviewed, reproducible, *disposable-game-progress* authenticated write/rollback pilot. It needs prewrite independent backup and authorisation, durable multi-file journal, a fail-closed recovery path, AGB_FIRM/game-relaunch verification and actual rollback. A RAM simulation PASS cannot unlock direct writes.

## Branch integration

This feature depends on Phase 16/15/14 draft branches. Keep its PR draft and do not cherry-pick around recovery gates, merge into main, or alter Phase 18/19 work. Photographed physical console evidence is recorded below; do not commit photos, game saves or journal contents to this repository.

## Physical Nintendo 3DS Phase 17 rehearsal — PASS (1 November 2026)

User supplied three photographs of the actual Phase 17 screen. First shows `PKSM ROGUE - PHASE17 RAM ROLLBACK`, `LIVE GBA VC: READ ONLY`, `RESTORE ENABLED: NO` before verification. The next screen shows `Phase16 journal: VERIFIED`, **Backup 263168 bytes**, **Journal 526400 bytes**, **Source counter 49**. The final photographed result is `TEST RESULT: RAM ROLLBACK TESTS PASSED` with:

| RAM-only hardware check | On-console output |
| --- | --- |
| RAM restore / explicit undo | 1 / 1 |
| Short-write recoveries | 4 / 4 |
| Crash restart recoveries (simulated process interruptions) | 4 / 4 |
| Rollback short retries | 4 / 4 |
| Rollback crash retries | 4 / 4 |
| Foreign-change refusals | 1 / 1 |
| Bytes recovered exactly | YES |

These results demonstrate on-device read-only verification of the preserved Phase 16 data and execution of the Phase 14 recovery algorithms against **RAM-only copies** using the console's current authentication session. The test application did not attempt an installed-game write. The test's simulated interruption cases are not a physical power-loss test. **Still unverified**: native live-save write permissions; production journal durability across abrupt power-off; AGB_FIRM accepting and re-saving an edited save; recovery after an actual torn filesystem write; restoration of an untouched original save; verified live rollback. No Phase 17 hardware pass authorises direct game-save writes.

GitHub Actions: [run #41259255688](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41259255688) — all five CI jobs passed at code head `10598f1cc188b66b4cb5f4d6389e6f05c88b9900` prior to this documentation update. PR #9 remains draft. Leave the two private Phase 16 SD files intact. Next steps are independent recovery backup/restore readiness, hardened journal lifecycle, and an explicitly reviewed pilot on **expendable** progress, never a direct write on this evidence alone.
