# Phase 9 — independent, read-only 3DS runtime test

This project is **not the PKSM Rogue app release**. It is a separate homebrew 3DSX using exactly the same Rogue candidate bridge/observed parser code as the PKSM fork, without the standard PKSM extdata/archive or asset initialization that fails in the fresh Azahar environment.

## Build and launch
The devkitPro standard 3DS application template is used in GitHub Actions. See [Phase 9 diagnostic and emulator screenshots](https://github.com/hboots999-hash/Rogue-save-editor-/actions/workflows/phase9-readonly-diagnostic.yml). The input source core archive is verified using a pinned SHA256 before compilation.

It tries to read ONLY the SD copy `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`. No game title lookup, live archive access, writes, editing, backups or restore are compiled into this test. Without a copy it should display `No supported SD copy loaded` and still allow L/R page and START exit.

**Do not install an experimental CIA on a physical 3DS based on a successful test 3DSX run.**

## Remaining blockers for final PKSM Rogue CIA
- PKSM 10.4.2 (official and our fork) shows the same extdata startup warning in a clean Azahar 2126.2 environment.
- The standalone diagnostic is only a way to isolate native renderer and input from PKSM initialisation.
- The final PKSM fork still requires a verified full editor-screen session, exact modified Emerald Rogue build compatibility, and real game-backup/restore testing.
- No user game save or ROM is committed to this repository.
