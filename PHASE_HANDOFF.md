# Phase 14 handoff — authenticated restore prototype

Date: 31 October 2026
Branch: `feature/phase14-authenticated-restore`
Base: `feature/phase7-native-ci` at `64cb5f0d4c393e23c4b2da70a4643c5b81f68d3a`
Status: source implementation and host recovery tests; **live writes disabled, no hardware restore claim**.

## Progress

Read PROJECT_HANDOFF.md, RELEASE_GATES.md and Phase 11–13 implementation/tests before starting. Inspected pinned Checkpoint and PKSM upstream sources (phase14/REFERENCES.md). Added an isolated phase14 directory and CI; no existing development source, workflow or release gate was modified.

Implemented authenticated full-container planning, stable console MAC-service adapter, strict title/size/CID/counter guards, reuse of Phase 13 narrow edit validation, older-slot payload/header/CMAC-last transaction, whole-container verification and rollback, shared bounded recovery record format, immutable POSIX and native SDMC recovery stores, and explicit restart recovery. Original active slot and all unrelated bytes are preserved. Native handles are read-only and native writer is permanently gated. No ROM, personal save, console key, CIA or private binary is committed.

## Validation

- Local strict-warning host compile and ordinary tests: PASS.
- Local AddressSanitizer + UndefinedBehaviorSanitizer: PASS with leak detection disabled because this execution host's ptrace restriction prevents LeakSanitizer. CI retains leak detection.
- Phase 11 header, Phase 12 backup, Phase 13 preflight and live-draft host regressions: PASS.
- Tests cover SHA-256 known answers; stable/stale/failing/unstable MAC service responses; malformed/foreign/CID-mismatched containers; counter ties/overflow; both slot directions and edit operations; modified plans; failed backup/readback/stale source before writes; each partial write; interrupted transactions after each of four writes; fresh journal reload/recovery; failed rollback; unrelated live changes; corrupt/truncated journals and non-overwrite.
- The host MAC service is a deterministic test double, NOT Nintendo AES-CMAC validation.
- GitHub CI native devkitPro compilation and host sanitizer results: pending first branch run; update this section once observed.
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
