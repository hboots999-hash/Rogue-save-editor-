# Phase 17 — RAM-only recovery and rollback rehearsal

**Status: experimental diagnostic. This is NOT real restoration.**

This phase builds on the Phase 16 on-device hardware PASS: an authenticated, unchanged 263168-byte Emerald Rogue 2.1.2 GBA VC container, a separate immutable 263168-byte original backup, a verified 526400-byte Phase 14-format before/after journal, and successful read-only verification after application restart.

## Why Phase 17 does NOT enable live writes yet

The Phase 14 restore logic has passed synthetic interruption tests, but its *installed-title write adapter* is hard-disabled. The 3DS has NOT been shown to accept or survive an edited save write, a physical interrupted live write, a complete rollback, a power loss, or an AGB_FIRM launch/re-save of an edited save. A valid console CMAC for a **proposed** after-image, demonstrated by Phase 16, is necessary but insufficient. No code in Phase 17 enables live writes, creates a writable game handle or modifies existing SD journal files.

## Phase 17 implementation

- `rollback_lab.hpp`: **pure-RAM** implementation of Phase 14's IO contract. It copies the protected before-image and saved Phase 14 journal into private heap buffers, then calls the real Phase 14 `execute` and `recover` algorithms using an in-memory write interface. It never calls a native write service.
- `test_rollback_lab.cpp`: nonplayable synthetic saves; both wrapper slot directions; both narrow edit operations (money +1, existing item quantity +1). For each case it tests one normal shadow restore, independent explicit shadow rollback, four short-write failures, four abrupt simulated terminations followed by recovery from a new in-memory IO object, and rejection of a foreign/unrelated changed byte. Also checks invalid journal hashes, truncation, mismatched original and failure of the MAC provider before any operation.
- `main.cpp`: read-only on-device UI that uses Phase 16's existing SD files and *read-only* live PxiFS0 session. X independently revalidates both slots, existing journal and unchanged game. Y first revalidates those again, then tests all the simulated restore/rollback cases **entirely in memory** using the 3DS console MAC service for plan validation. Neither X nor Y can change Emerald Rogue.
- `rsf`: distinct experimental app title ID **000400000F73A600** (separate from Phase 16 test title `000400000F73A400`, Phase 19 UI demo `000400000F73A500`, and installed Emerald Rogue `0004000007D7BA00`).
- `.github/workflows/phase17-rollback-rehearsal.yml`: host strict/sanitizers, inherited Phase 16/15/14 regressions, native devkitPro compilation with no linked game-writer symbol in main, Azahar launch, experimental CIA packaging and installed-CIA smoke.

The "RAM-only writer" has an explicitly named `writesAllowed()=true`, **only to satisfy the preexisting Phase 14 transaction state machine over an isolated vector**. It does NOT alter the constant `LiveWritesEnabled=false` used by the console backend.

## On-device test (after GitHub CI passes)

1. Keep the original GodMode9/Checkpoint recovery backup. Leave the Phase 16 files on your SD card:
   - `SD:/3ds/PKSM-Rogue/phase16/original_container.bin`
   - `SD:/3ds/PKSM-Rogue/phase16/rehearsal_only.rec`
   These contain **private game data**. Do not upload them to GitHub or elsewhere, and do not rename, delete or inject them.
2. Close Emerald Rogue. Open the Phase 17 **3DSX** using Homebrew Launcher first. Its separately titled experimental CIA is an alternative, not necessary for this test.
3. Press **X**: it should display the previously verified Phase 16 snapshot and exact original/journal byte counts. The game save must remain unchanged since Phase 16; if it has been saved since then, the app should refuse to continue.
4. Press **Y**: it rechecks the same live game and runs only memory-based restore/rollback simulations. Success display should say `RAM ROLLBACK TESTS PASSED`, **RAM restore/undo 1/1**, **Short-write recoveries 4/4**, **Crash restart recoveries 4/4**, **Foreign-change refusals 1/1**, **Bytes recovered exactly YES**.
5. Photograph the result. START exits. There is no live restore control or SD write control.

If the app shows a non-PASS result, stop testing and share the photo. Do not try to fix the live game or delete private journal files. No interruption/power-loss experiment on the actual console is requested.

## What this proves and does not prove

A physical PASS would prove the on-console reader can independently reopen the already authenticated current game and the existing Phase 16 private record, and that the rollback algorithm reaches the expected results using a real file-bound MAC signer but **only RAM mutations**. It does NOT prove real filesystem rollback, power-loss persistence, real native title write permissions, actual edited-game booting, game re-saving, PC/Pokémon edit compatibility, or a production recovery mechanism. It does not authorize live writes or a merge into the development branch.

## Next gate: actual native restoration

Before any *separate* reversible, expendable hardware trial is authorised:
- Independent GodMode9/Checkpoint backup and restore walkthrough; verify full unchanged GBA container and original raw save after a console restart.
- Review Phase 14 native journal and write architecture (chunked durable storage, signed receipts, interrupted/power-loss recovery). Phase 16 rehearsal files are NOT a production journal.
- Gate direct writes behind a separately reviewed capability and explicit user confirmation on disposable progress, not only a compile flag.
- Prove authenticated edited slot acceptance, AGB_FIRM game load, save/relaunch, exact rollback on 3DS hardware and safe recovery on interruption. Never use a valuable unbacked save for destructive testing.

Keep this PR draft until all relevant gates are verified. Do not merge Phase 14–19 branches blindly.
