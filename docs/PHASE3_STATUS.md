# PKSM Rogue Edition — experimental development checkpoint

**NOT AN INSTALLABLE RELEASE. Do not install the developer builds on your 3DS.**

## What exists
- Numeric, read-only Emerald Rogue candidate save parser and C API.
- PKSM `RogueInspectScreen` reads only a manually copied 128 KiB save from `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`.
- Scripts `isolate_pksm.py`, `apply_to_pksm.py`, and `restrict_startup.py` for a clean pinned PKSM source checkout. Run them **in that order**.
- The startup-restricted build opens directly into RogueInspectScreen; it skips normal PKSM bank/title/save scanning and prevents PKSM auto-update installation. B exits the main loop.
- Inherited absolute SD paths are mechanically rewritten to a separate `/3ds/PKSM-Rogue` directory; 76 occurrences in 16 core C/C++ files were observed in CI. This does not prove total runtime isolation.
- Candidate format is *not* a verified exact ROM fingerprint; Pokémon/item names and editing remain unsupported.

## Verification so far
- Host compiler, malformed-save test, and guarded overlay tests pass.
- A previous modified PKSM ARM ELF compiled and linked with the real devkitARM toolchain in [run 41136187630](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41136187630).
- **Correction:** earlier run 41137034257 was reported successful by GitHub Actions even though 3DSX packaging failed with `Permission denied` on a Python script; its downloaded artifact ZIP contained only an ELF, no 3DSX.
- Corrected build [41137575458](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41137575458) **passed** packaging, and inspection of the downloaded ZIP confirmed a 5,673,224-byte `PKSM_Rogue_Inspect.3dsx` (valid `3DSX` signature) alongside a 71,641,036-byte ARM ELF. These files are untested on emulator or hardware.
- An isolated Azahar emulator screenshot/log smoke test has been added, dependent on a *verified* 3DSX build. Do not assume it passed without reviewing its output.

## Release blockers
- Re-check current startup-restricted native build and real 3DSX artifact.
- Review any emulator screenshot and crash logs.
- CIA packaging tools and CIA exheader validation (priority, title ID) are not yet completed.
- A separate title ID and path rewrite do not guarantee no interaction with installed PKSM; inherited source still contains ordinary editing/write functionality outside the restricted UI.
- No actual 3DS device test, exact patched ROM validation, safe edit or restore.

## Source provenance
PKSM upstream: GPL-3.0-or-later with applicable additional notices. RogueSM candidate parser: MIT. Preserve all original notices and attribution.

No personal saves, ROM images, console keys, or SD dumps are committed to this public repository.

## Verified synthetic on-screen inspection — 30 October 2026
- [Azahar synthetic test run](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41141214229) passed. The screenshot was visually inspected after startup.
- Fixture created from code only: 128 KiB, candidate save marker 6, money 123456, party 0, numeric item IDs 1x50 and 2x75. No user save, ROM, or private data used.
- Host C++ snapshot test also passed, validating the exact money, zero party, and both item quantities.
- **Screenshot visibly confirms** Rogue Inspect title, marker 6, money 123456, save counter 0, party size 0, occupied bag slots 2, and read-only warning. L/R and B input not tested yet.
- Still unverified: real-save console display against exact ROM; item/party tab button navigation; clean exit; physical 3DS; any save modification/restore.
