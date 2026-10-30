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
- The packaging workflow was corrected to pass `PYTHON=python3`, fail on packaging errors, and confirm the resulting 3DSX begins with the `3DSX` magic.
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
