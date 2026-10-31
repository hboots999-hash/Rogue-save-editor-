# Phase 15 handoff — read-only H168 hardware CMAC diagnostic

Date: 31 October 2026
Branch: `feature/phase15-readonly-hardware-validation`
Parent: `feature/phase14-authenticated-restore` at `6529b9bd531760a3ea5879a6f37eb995b5f47f2b`
Draft PR: https://github.com/hboots999-hash/Rogue-save-editor-/pull/5
Main development branch: `feature/phase7-native-ci` (untouched)

## Delivered
- `phase15/diagnostic.hpp`: two fresh full-container reads, byte-for-byte stable snapshots, strict two-slot structure/title/CID/counter/raw checks, stable file-bound console MAC verification for each slot; diagnosis only, no writes.
- `phase15/main.cpp`: native 3DS text UI with Y-triggered diagnostics and no direct restore.
- `phase15/sd_probe.hpp`: optional A→A SD-only 256-byte uniquely created flushed readback probe under `/3ds/PKSM-Rogue/phase15/`; no real saves, installed title writes, or recovery-journal mutation.
- `phase15/test_diagnostic.cpp` and `run_tests.sh`: synthetic-only test MAC double, corruption, stale live reads, both headers, title, CID, counters, raw, unstable/failing console service, and Phase 14 regression.
- `.github/workflows/phase15-hardware-diagnostic.yml`: strict-warning/sanitized host tests, devkitPro 3DSX/ELF, Azahar UI boot smoke and isolated experimental CIA packaging.
- `phase15/README.md`: step-by-step 3DS instructions and proof boundaries.

## Verified so far
- Host normal and ASan/UBSan synthetic diagnostic tests, inherited Phase 14 and Phase 11–13 regressions: **PASS** in GitHub Actions latest build before this handoff update.
- Native devkitPro 3DSX/ELF compile and explicit forbidden live-write symbol check: **PASS** in the same run.
- Azahar launch and CIA packaging: see [Phase 15 Actions](https://github.com/hboots999-hash/Rogue-save-editor-/actions/workflows/phase15-hardware-diagnostic.yml); do not assume PASS without a completed successful run.
- **NOT TESTED:** read-only full-container CMAC calls on the user's physical console, native SD scratch-file readback, real recovery journal lifecycle, durability under power loss, direct restore, rollback and gameplay resave. A deterministic host MAC DOUBLE is not Nintendo CMAC.

## Integration
1. Keep this PR draft and Phase 14 PR #3 draft. Phase 15 branches from Phase 14 so integration depends on Phase 14 reviewed first. Do not merge directly into `feature/phase7-native-ci` or overwrite Phase 13.
2. Download ephemeral `phase15-H168-READONLY-3DSX-NOT-RELEASE` and `phase15-H168-experimental-CIA-READONLY` GitHub Action artifacts after CI completion. Prefer Homebrew Launcher 3DSX for initial hardware test. CIA title ID `000400000F73A400` replaces only earlier experimental preview, not target Emerald Rogue title `0004000007D7BA00`.
3. With independent GodMode9 and Checkpoint backups, close game and press Y on Phase 15 to read/check current live container, screenshot slot statuses and native result. The optional A→A SD scratch test may follow. Never inject edited Phase 13 drafts as part of Phase 15.
4. Investigate any failed/mismatched title/container/CMAC result rather than weakening validation to force success. The two-slot policy might reject legitimate old/uninitialised data.
5. **Phase 16 release gate remains blocked**: console agreement is necessary but insufficient. Need independent raw/container backup, transactional journal recovery UI, exclusive write lifecycle, disposable live restore, authentic CMAC, rollback under injected failures and game re-save tests before any separate change permits live writes.

No game ROM, console keys, personal save or recovery record included in repository.

---

## Previous Phase 14 handoff (preserved)

# Phase 14 handoff — authenticated restore prototype

Date: 31 October 2026
Branch: `feature/phase14-authenticated-restore`
Base: `feature/phase7-native-ci` at `64cb5f0d4c393e23c4b2da70a4643c5b81f68d3a`
Draft PR: https://github.com/hboots999-hash/Rogue-save-editor-/pull/3
Status: source implementation, host recovery tests and native compile/link PASS; **live writes disabled, no hardware restore claim**.

## Progress

Read PROJECT_HANDOFF.md, RELEASE_GATES.md and Phase 11–13 implementation/tests before starting. Inspected pinned Checkpoint and PKSM upstream sources (phase14/REFERENCES.md). Added an isolated phase14 directory and CI; no existing development source, workflow or release gate was modified.

Implemented authenticated full-container planning, stable console MAC-service adapter, strict title/size/CID/counter guards, reuse of Phase 13 narrow edit validation, older-slot payload/header/CMAC-last transaction, whole-container verification and rollback, shared bounded recovery record format, immutable POSIX and native SDMC recovery stores, and explicit restart recovery. Original active slot and all unrelated bytes are preserved. Native handles are read-only and native writer is permanently gated. No ROM, personal save, console key, CIA or private binary is committed.

## Validation

- Local strict-warning host compile and ordinary tests: PASS.
- Local AddressSanitizer + UndefinedBehaviorSanitizer: PASS with leak detection disabled because this execution host's ptrace restriction prevents LeakSanitizer. CI retains leak detection.
- Phase 11 header, Phase 12 backup, Phase 13 preflight and live-draft host regressions: PASS.
- Tests cover SHA-256 known answers; stable/stale/failing/unstable MAC service responses; malformed/foreign/CID-mismatched containers; counter ties/overflow; both slot directions and edit operations; modified plans; failed backup/readback/stale source before writes; each partial write; interrupted transactions after each of four writes; fresh journal reload/recovery; failed rollback; unrelated live changes; corrupt/truncated journals and non-overwrite.
- The host MAC service is a deterministic test double, NOT Nintendo AES-CMAC validation.
- GitHub CI **PASS**, both `host` and `native` jobs: https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41252281936 (implementation commit `9f93fe8afb822f5bd98661481d54e7385cb7c005`). Host includes ASan/UBSan with leak detection, all Phase 11–13 regressions, write-gate and private-file guards. Native includes devkitPro compilation, relocatable linking with libctru, and confirmation that the probe has no privileged write reference. This subsequent handoff-only update does not change the tested code.
- No physical console or game/emulator restore was performed. No installable Phase 14 binary is shipped.

## Outstanding problems / required integration work

1. Native SDMC journal code is implemented but NOT hardware-verified: exclusive FSUSER creation, flushed exact write, checked close and reopened readback. A fixed recovery record blocks reuse even after completion; the parent recovery directory must exist. Wire pending-record checks into startup UI and verify actual SD durability. POSIX host fsync is not evidence of native SD durability. Partial records are preserved.
2. NativePxi now closes/reopens each snapshot; verify it on hardware. Implement lifecycle/exclusive access, independent raw recovery export, two-step explicit user confirmation and recovery UI. NativeBackend composes the journal and PXI adapter, but execute/recover immediately block because writable handles cannot be opened.
3. On a disposable console/title setup, establish console MAC agreement with existing slots before testing a writer; verify full container file size/layout and whether the strict two-slot policy is supported. The device adapter has never run here.
4. Verify authenticated restore, full close/relaunch, game re-save, and exact rollback with independent GodMode9/Checkpoint recovery. Test interruption behavior, ambiguous headers and AGB_FIRM fallback. Host byte-array tests cannot prove these.
5. Counter ties, wraparound, uninitialized/corrupt slots, other save sizes and foreign-title containers are deliberately unsupported, not repaired. A partial transaction containing bytes outside the saved before/after pair requires manual recovery rather than automatic overwrite.
6. Only after the above and release-gate review may a separate change open a writable PXI handle, enable the constant and wire live restore into the UI. Do not expose an enable flag in this PR. The SHA-256 record checksum is integrity detection, not a secret-key journal signature.

## Integration instructions for main development chat

Review this draft against `feature/phase7-native-ci`; merge/cherry-pick only when desired. This task does not merge or alter that branch. Existing Phase 11–13 behavior remains unchanged. Run `bash phase14/run_tests.sh` and the Phase 14 workflow. Read phase14/README.md for IO backend contracts; all lengths and flushed exact counts are mandatory. Never substitute Phase 11's unauthenticated raw snapshot for the authenticated full-container source. Do not reuse a transaction directory or silently delete recovery records.

The engine's `Restored` status means authenticated backend readback of intended bytes, not confirmed in-game acceptance. `RolledBack` requires byte-exact original container and fresh authentication. `RecoveryRequired` must remain a blocking state with preserved backup/journal. No result from this phase upgrades hardware release gates E/F/G.
