# PKSM Rogue Phase 9.4 — configurable synthetic-only offline editor

**EMULATOR EXPERIMENT ONLY — DO NOT INSTALL CIA ON A PHYSICAL 3DS.**

This extdata-free 3DS application is separate from full PKSM Rogue. It reuses the previously tested Phase 7 candidate parser, numeric editor, IV/EV editor and move editor, without accessing an installed game save, restoring a game, or touching PKSM extdata.

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

Phase 9.3 connects the existing IV/EV and move writers. On the one known toy Pokémon, navigate to Party and select it: X stages six IVs of 31, Y stages the test EV spread (HP 4, Speed 252, SpAtk 252), and SELECT stages replacing move slot 1 with numerical move ID 85. Press A twice to export after confirmation; B cancels. The same byte-exact synthetic fixture gate, unchanged import, separate verified backup/draft and one-export-per-launch restriction still apply. Host and ASan/UBSan regressions passed: [41179780305](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41179780305). Azahar guest tests for all three button paths run in [Phase 9.3](https://github.com/hboots999-hash/Rogue-save-editor-/actions/workflows/phase9-readonly-diagnostic.yml), and require file-level verification, not just screenshots.

All other save files remain read-only. No species, nature, ability, held item, PC, hub area, upgrade or live-restore editor is available here.

## Phase 9.4 configurable inputs (31 October 2026)

In the synthetic lab viewer, select **Summary** to edit money, **Party** to edit Pokémon, or **Bag** to modify item quantities. X stages the default operation. On Party, X stages IVs, Y EVs, SELECT a move slot/ID/PP change. On Bag, X stages the selected existing item's quantity, Y ensures Rare Candy ID 113, and SELECT ensures Master Ball ID 1. All operations work on the **exact deterministic synthetic fixture only**.

On the first confirmation screen, use **Up/Down** to select the numerical field, **Left/Right** to change it by one, and **L/R** to change it by ten (money 10,000). Press **A** to review the proposal; press **A** again to create a **separate** SD draft. **B** cancels. Money is capped at 999,999; existing item quantities at 999; IVs at 31; EVs at 252 each and 510 total; move IDs at 1–848; PP at 1–40. Move slots use internal numbering 0–3. Insert-item IDs are restricted to the test ball subset 1–24 and Rare Candy 113, not arbitrary items. If a permitted item already exists, it updates that stack; insertion into a new slot uses the conservative Phase 7 pocket routine and can refuse unknown layouts. There is still only one export attempt per app launch.

Verified in the host synthetic suite and Azahar-native guest tests, including adjustable money, IV, EV and move values; existing Rare Candy/Master Ball stacks; and preserved backups. The installed-CIA guest workflow is also rerun. See [release gates](../RELEASE_GATES.md) and the newest [GitHub Actions](https://github.com/hboots999-hash/Rogue-save-editor-/actions/workflows/phase9-readonly-diagnostic.yml).

**No real game-save edits are enabled.** Never install this toy CIA onto a real console or attempt to restore one of these synthetic drafts into Emerald Rogue.

## Next steps before an installable release

1. Isolated direct-content CIA installation and launch now passed in Azahar. Verify normal HOME Menu launch/exit, restart, and recovery separately before any hardware test.
2. Phase 9.4 now supports adjustable numerical controls, conservative toy item updates and independent guest tests. Next validate unknown-copy refusal in the guest, cancel, forced disk failure and recovery; do not touch live game files.
3. Validate exactly the user's patched Emerald Rogue ROM/build and disposable save; add build-specific compatibility allowlist only with positive evidence. Unverified saves remain read-only.
4. Validate live-archive backup and disposable game-save restore with correct ROM, then console safety and rollback without touching the user's installed game until the release gates pass.
5. Resolve the full PKSM UI's extdata startup setup and confirm isolated title and SD paths; only then consider a hardware release CIA.

No personal ROM or .sav is in public GitHub; CI artifacts expire, but source and PROJECT_HANDOFF.md remain.
