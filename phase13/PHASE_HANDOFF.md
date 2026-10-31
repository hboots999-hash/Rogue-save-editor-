# Phase 13 hardening handoff — 31 October 2026

**Task:** Finish Phase 13 safety regressions independently while Phase 14 authenticated restoration proceeds on another branch.

## Baseline already implemented
- Read-only live GBA VC fetch for the pinned H168 title ID.
- Repeated live snapshots, separate immutable 128-KiB SD backup, money+1 or existing-item-quantity+1 SD draft, strict changed-byte preflight, final freshness check.
- Installed game writes/CMAC restore remain **disabled**.
- Prior project status is recorded in `PROJECT_HANDOFF.md`, `RELEASE_GATES.md`, and `phase13/README.md`.

## Changes isolated in this branch
- Added `phase13/test_fail_closed.cpp`: synthetic-only fault injection for live-read checkpoints 1–5; tests require rejection at each failure and correct draft/backup presence after failure. A valid, checksummed stale save on checkpoint 4 must be blocked. Existing partial backup name must remain untouched and next unique name must be allocated.
- Added the regression to `.github/workflows/phase13-live-draft.yml` and enabled this workflow on `feature/phase13-hardening` pushes.
- Documented host test command and safeguards in `phase13/README.md`.

## Test status
- Existing Phase 13 source previously passed host, native build and emulator smoke in the development branch, but those results **do not** establish success for this hardening branch.
- New host regression status: check Phase 13 GitHub Actions workflow for this branch. If still running, label as **unverified**; do not report hardware proof.
- The privileged live-to-draft path cannot be exercised by Azahar and needs physical device confirmation. Confirm a current independent GodMode9/Checkpoint recovery backup first.

## Open problems / hardware verification
1. On physical 3DS open installed Phase 13 CIA, Y to read the live save; verify title and current money/counter; X → A → A to stage a new SD-only draft.
2. Inspect `SD:/3ds/PKSM-Rogue/backups/live_NNN.sav` and `drafts/liveedit_NNN.sav` in GodMode9 (each must be exactly **131072 bytes**); preserve existing copies.
3. Independently compare untouched backup to the currently live save and evaluate draft's exact limited delta; never inject a draft before Phase 14 CMAC/rollback gate.
4. Phase 14 separately owns authenticated direct restore, rollback, power-loss failure modes, and game launch/resave proof; Phase 13 does **not** enable any of these.

## Integration instructions
- Review this branch against `feature/phase7-native-ci` and run Phase 13 CI.
- If approved, cherry-pick only its three hardening files, or merge its draft PR once there are no Phase 14 conflicts.
- Do **not** merge directly into `feature/phase14-authenticated-restore` or `feature/phase7-native-ci` without review; avoid enabling live writes through a side channel.
- No personal save bytes, console secrets, ROM or CIA are committed here.
