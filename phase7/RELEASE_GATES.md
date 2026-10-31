# PKSM Rogue — release gates (31 October 2026)

This is an EXPERIMENTAL PKSM fork. **Do not install on a physical Nintendo 3DS.**

## Evidence
- Full native build: successful ELF, 3DSX and CIA, run [41153365442](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41153365442).
- Emulator CIA installer: installed `000400000F73A200` into a fresh isolated Azahar 2126.2 profile, content `title/00040000/0f73a200/content/4ac1c5d4.app`, confirmed `Installed CIA successfully.` in run [41156018395](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41156018395).
- Installed content starts PKSM startup code in Azahar, but the app reports `PKSM ext data appears to be corrupted`, with `archive: ext data not mounted`.
- Direct-path installed-content launch with an A-key attempt produces a different but unreadable/visually corrupted screen, run [41156216534](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41156216534). That **is not** verified navigation into PKSM Rogue Inspect.
- Host-source tests of a copied candidate save passed earlier, but this does not prove game-compatibility or restore safety.

## Remaining REQUIRED before an installable release
1. Determine the precise extdata failure and resolve it while preserving data; **never auto-reset extdata on hardware**.
2. Navigate to `RogueInspectScreen` in emulator, verify SELECT, B, screen switching and error handling.
3. Test staged draft exports against a *disposable* input with byte-wise before/after, correct checksum, and the same ROM/build as the user's Emerald Rogue.
4. Validate backup-and-restore recovery separately. The experimental app must never directly overwrite an installed game save.
5. Review SD folder isolation: the fork changes its title ID but inherited PKSM's `/3ds/PKSM` configuration/assets/bank paths. A separate title ID alone does not protect the standard PKSM data from conflicts.
6. Verify emulator launch/exit under normal assets, then verify a hardware-safe release candidate with a recovery plan.

This document and GitHub Actions logs contain no personal game save or ROM. **A green GitHub Actions status is not proof of a successful in-emulator guest launch.**
