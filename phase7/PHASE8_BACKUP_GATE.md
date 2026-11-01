# Phase 8 — Verified imported-copy backup gate

The first usable PKSM Rogue Edition release remains **experimental and NOT safe to install**. This source-only safeguard is applied after the Phase 7 overlay and the separate SD-folder patch in the GitHub Actions native build.

## What changes
- On the first **modified draft** of each editor session, reopen `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav` and confirm it has not changed since the editor loaded the save.
- Make an exclusive, readback-verified backup `sdmc:/3ds/PKSM-Rogue/backups/import_NNN.sav`, with names limited to 001–099.
- Refuse export if this **import-copy backup** cannot be verified; never write to an installed Emerald Rogue game save.
- Drafts remain exclusive, readback-verified `sdmc:/3ds/PKSM-Rogue/drafts/rogue_NNN.sav` files, with double confirmation and chained edits.
- Display backup state on the summary screen.

## Test evidence
Host checks on a copied 131,072-byte candidate save:
- Wrong-size, unreadable and invalid save rejection
- No write before two confirmations
- A deliberate failure where `backups` was a regular file: draft **refused**, imported save unchanged
- A changed import mid-session: draft **refused**
- Verified untouched backup created before the first edited draft
- A second session creates a **distinct** import backup rather than replacing the first
- Move-edit followed by Master Ball insertion preserves the first edit in the second draft
- Protected preexisting draft names, isolated other save slot and correct checksums
- AddressSanitizer and UndefinedBehaviorSanitizer host screen test passed

The user's actual copied save is **not** committed to this repository. The source testing package is available through the existing ChatGPT conversation.

## Still required before a release
1. Native 3DS compilation and integration of this patch.
2. Resolve/characterize emulator initialisation behavior without deleting any user's PKSM banks. Unmodified PKSM 10.4.2 exhibits the same extdata issue in clean Azahar.
3. Verify the custom Rogue editor screen on a working emulator and the exact Emerald Rogue ROM/build.
4. Verify backup/recovery and manually restored game-save behavior in a disposable setup, then conduct a controlled physical-3DS launch test.

This is **not** a live installed-game backup or restore solution, and is not evidence that any user-generated draft is safe to restore. Avoid installing experimental CIA artifacts on a 3DS.

## Verified cloud results

- **Native Phase 8 CIA compilation: PASS**, including SD-path isolation and backup-gate patch: [GitHub Actions run 41157334536](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41157334536).
- Compiled artifact file checksums verified against the workflow's own SHA256SUMS; ELF, 3DSX and CIA present. **Artifact is experimental, DO NOT INSTALL.**
- **Synthetic host regression: PASS** on GitHub: [run 41157776550](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41157776550). Builds and tests the very same patched Rogue screen using a wholly synthetic 128-KiB save (no personal files) and passes AddressSanitizer and UndefinedBehaviorSanitizer.
- These tests do not certify launch on physical 3DS, game ROM version compatibility, or any direct installed-game backup/restore functionality.
