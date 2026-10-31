# PKSM Rogue Phase 9.2 — synthetic-only offline editor

**EMULATOR EXPERIMENT ONLY — DO NOT INSTALL CIA ON A PHYSICAL 3DS.**

This extdata-free 3DS application is separate from full PKSM Rogue. It uses the previously tested Phase 7 candidate parser and numeric editor, without accessing an installed game save, restoring a game, or touching PKSM extdata.

## Verified achievements

- Phase 9 numerical party/bag read-only viewer launched with synthetic save: [41158813605](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41158813605).
- Phase 9.1 navigation, details, host sanitizers, native build and Azahar: [41159666627](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41159666627).
- Phase 9.2 integrates the EXISTING Phase 7 numeric money / existing-item quantity editors with a separate native SD-copy backup-and-draft gate (phase9/offline_sandbox.hpp). It accepts ONLY the exact, deterministic, non-playable 131072-byte Phase 7 synthetic fixture. All real or unknown Emerald Rogue saves are READ-ONLY.
- On synthetic data only, money can be staged at 999999 or a selected existing bag item quantity at 999. Import is reread, compared against the session original, backed up with exclusive create, byte-for-byte reverified and never overwritten. Drafts likewise use exclusive names and readback. Any failed step blocks export.
- Synthetic host tests, including stale-source, blocked directories, existing filename collisions, re-verification and ASan/UBSan passed: [41160327093](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41160327093).
- Native Azahar guest button sequence successfully created a synthetic 999999-money draft and untouched backup; the screenshot showed SYNTHETIC DRAFT: rogue_001.sav. Emulator test passed: [41160686927](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41160686927).
- Experimental CIA packaging, native build, guest synthetic edit test and host tests all passed: [41160794180](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41160794180). The resulting emulator-only artifact is named PKSMRogue-SYNTHETIC-LAB-NOT-FOR-INSTALL.cia. Experimental Title ID: 000400000F73A300, distinct from Phase 7 full fork 000400000F73A200. This is NOT a tested console release. The separate isolated installed-CIA emulator test also passed: [41160914286](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41160914286). Azahar reported CIA installed successfully in title path 00040000/0f73a300, direct installed-content launch displayed the synthetic save summary and a separate synthetic money draft confirmation. Not proof of hardware HOME Menu operation.

## Controls

L/R change summary, party and bag pages; Up/Down select; A opens details; B cancels or returns; START exits. On the exact synthetic fixture ONLY, Summary X then A then A stages a separate 999999 money draft, while Bag X then A then A stages the selected existing item's quantity at 999. At most one native draft export occurs per app launch; import and installed game save remain untouched.

All other save files remain read-only. Phase 7/8 move and IV/EV writers exist but are not yet connected to Phase 9.2. No species, nature, ability, held item, PC, hub area, upgrade or live-restore editor is available here.

## Next steps before an installable release

1. Isolated direct-content CIA installation and launch now passed in Azahar. Verify normal HOME Menu launch/exit, restart, and recovery separately before any hardware test.
2. Integrate existing offline move and IV/EV writers into this native UI with strict stale-data and failure tests.
3. Validate exactly the user's patched Emerald Rogue ROM/build and disposable save; add build-specific compatibility allowlist only with positive evidence. Unverified saves remain read-only.
4. Validate live-archive backup and disposable game-save restore with correct ROM, then console safety and rollback without touching the user's installed game until the release gates pass.
5. Resolve the full PKSM UI's extdata startup setup and confirm isolated title and SD paths; only then consider a hardware release CIA.

No personal ROM or .sav is in public GitHub; CI artifacts expire, but source and PROJECT_HANDOFF.md remain.
