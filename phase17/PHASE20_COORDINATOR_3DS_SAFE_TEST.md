# Phase20 coordinator: safe physical 3DS experiment (3DSX first)

> **2 November evening update:** Phase20 CI-pinned source `5a79bcbc522ad4e8d37f14bebfa9e7ac12143de2`. The new move browser is **read-only**. On Pokémon detail **Y** can open the browser; on Home/Money/Bag/Party **Y** handles the existing batch queue. The browser supports searchable metadata only, **not** replacement. Its optional 14,040-byte external catalog has a verified SHA-256, but does not attest the game ROM on the console. START+SELECT stays reserved for Rosalina. Test the vetted **3DSX first**, never the CIA by default.
>
> For a no-write independent PC check after a synthetic export, use `phase17/verify_offline_sd_hashes.py` with previously saved original and Phase16 SHA-256 hashes **and** an independently produced expected Phase18 candidate SHA-256. Full command and negative-case instructions are in [PHYSICAL_READONLY_VERIFICATION_CHECKLIST.md](PHYSICAL_READONLY_VERIFICATION_CHECKLIST.md). A failure means preserve files and STOP; no automatic cleanup or restore.


**Phase17 read-only safety handoff, 2 Nov 2026. Not an instruction to install or modify a game save.** The user's L shoulder is broken; Rosalina shortcut is **START+SELECT**. Experimental Phase20 CIA title `000400000F73A700` is **not a release**, and the valued Emerald Rogue title is `0004000007D7BA00`. Confirm both independently before any optional installation. Never overwrite or uninstall the game.

## Before touching the console

1. Turn the 3DS fully off. On a computer, preserve a full SD card copy including `Nintendo 3DS`, `3ds/PKSM-Rogue/phase16/original_container.bin` **263168 bytes** and `3ds/PKSM-Rogue/phase16/rehearsal_only.rec` **526400 bytes**. Retain the untouched archive on a *different* device (preferably a second separate backup as well). Compute/check local SHA-256 for each Phase16 file and do **not** upload its private contents.
2. List the names and sizes already present in `3ds/PKSM-Rogue/imports`, `backups` and `drafts`. If the fixed `imports/00000001.sav` exists and is valuable or its provenance is unknown, **do not replace or rename it**. Stop before editing; the app has a fixed input path.
3. Get the exact successful-commit Phase20 **3DSX** from its CI artifact, compare its SHA-256 on a PC with the manifest. Do not rely on an earlier CIA or a same-named unverified file. Keep other apps/installations unchanged.
4. The first pass can run with **no input file**, to verify that missing SD input is refused cleanly. For a later permitted export test, place only a separately generated **nonplayable synthetic 131072-byte raw save** at the fixed path, if empty. The fixture must not be a valued real game save.

## Minimal L-free 3DSX test

1. From a stable HOME Menu, open Homebrew Launcher and launch the vetted standalone Phase20 3DSX. Do not test the CIA initially.
2. Use **D-pad, A, B, X, R and Y**; no step requires L. Confirm the Missing Copy error with no input. **A read error must not turn into an apparently genuine Pokémon view or a created draft**.
3. With an expendable fixture, re-open the import menu and compare known synthetic money/party/items against the fixture. **B must cancel** the value editor and both confirmation screens.
4. For the small *optional* multi-edit export: press **Y** to start the queue; stage two independent valid synthetic scalars through their normal A menus; press **Y** to review, inspect the list and use X only if intending to remove the last entry. Approve two distinct final confirmation screens; the only permitted new files are `backups/phase18_original_NNN.sav` and `drafts/phase18_draft_NNN.sav`, each **131072 bytes**, each under the application SD folder. Don't test move replacement; it is not in the audited native Phase20 build.
5. On exit, **release START alone** for app exit; hold **START+SELECT** only when intending to open Rosalina (the app reserves the combo). Verify HOME remains stable. If any crash, freeze or unusual black-screen delay occurs, stop all testing.

## Optional experimental CIA, separate consent only

**Do not install by default.** Only consider it after the 3DSX and full SD external backup have passed, HOME is stable, and the owner expressly opts into a separate title installation. Independently verify the CI CIA SHA-256 and title ID `000400000F73A700`; the installer must not update/replace an existing app or the game title `0004000007D7BA00`. If a previous app uses the same title ID, **skip CIA**, do not overwrite it. Never treat a successful emulator CIA boot as approval for hardware. First CIA test must be boot/menu/cancel/exit only, then only synthetic SD copy operations in a separately approved pass. Don't open any installed save-restore service.

## Independent evidence after the test

- Power down **before removing SD**. Compare the full Phase16 container and journal byte lengths and SHA-256 to the off-device protected originals: **both unchanged**. Also check the original `Nintendo 3DS` contents and any pre-existing import hashes, without moving/deleting them.
- For a created raw export: the input import and new `phase18_original_NNN.sav` must have matching hashes and 131072-byte sizes; the draft must be independently compared byte-for-byte to an expected synthetic candidate produced by Phase18 v3 (and reparse successfully). The **draft's hash differs** when an actual edit occurred; that alone does not demonstrate semantic correctness or game compatibility.
- For low-complexity checks **only**, GodMode9 can browse `0: SDCARD` → `3ds/PKSM-Rogue` using D-pad/A/B (no L). On an individual file, choose **A → Calculate SHA-256**, record it and return with **B**; compare to protected pre-test values. The official [GodMode9 README](https://github.com/d0k3/GodMode9#readme) documents this. **Never use X/delete, Y/copy/paste, the Hexeditor edit mode, scripts, title injection, unlocked write permissions or NAND drives.** GodMode9 SHA-256 does not independently establish Pokémon edit semantics; a PC verifier is still required.
- If export is incomplete, the console crashes or a partial file remains: **retain everything**, record filenames/sizes/hashes, do not delete/retry/overwrite, label the output INDETERMINATE and stop. No game restore or journal replay.

## Phase17/20 boundary and release decision

**131072** is one ordinary unwrapped save copy. **263168 = 2×(512-byte authenticated wrapper header + 131072-byte raw slot)** is the original GBA VC full-container format. **526400 = 64 + 2×263168** is the earlier RAM rehearsal journal. A Phase20 backup on the same SD is **not** a separate whole-game recovery archive. There is no authorised Phase20 draft-to-installed-container adapter, ROM-verified move replacement, CMAC writer, or production durable journal.

**Open gates before a *separate disposable-progress gameplay restoration pilot* can be discussed:** two off-device independently validated full containers and a proven independent recovery route; exact ROM/title/field mapping; fresh console-bound CMAC and both slot counters; chunked durable receipt/journal and interruption trials; bound memory/failure testing; explicit new opt-in and independent QA review; physically observed test-game restore, load, re-save and rollback on disposable progress. None of those are authorised by this 3DSX/CIA guide. `LiveWritesEnabled=false`.

For the exhaustive stop conditions see [PHYSICAL_READONLY_VERIFICATION_CHECKLIST.md](PHYSICAL_READONLY_VERIFICATION_CHECKLIST.md).
