# Restricted physical verification — copied files and read-only title access

**Updated 2 Nov 2026 for Phase20 multi-edit, 3DSX/CIA safety review and GodMode9 hashing. NOT a console task request, and NOT restoration approval.**
Owner runs this only when a physical 3DS is available, its complete SD is independently backed up, and the currently pinned Phase17/20 build has passed exact-commit CI. Previous HOME-menu start was delayed and an older experimental CIA crashed. Stop on abnormal behaviour.

## Absolute limits

- Installed Emerald Rogue title `0004000007D7BA00`: **open READ-ONLY or do not access**. No installed-save mutation, injection, removal, import/restore, cheat-writing, privileged FS write, CMAC modification, or journal replay.
- Use a clearly labelled **non-playable synthetic 131072-byte copied H168 fixture** for Phase20 test; never use a valuable original save as disposable input. Keep fixture offline/private; do not upload ROMs or save progress to GitHub/CI/chat.
- Only the app's separate SD directory may receive **new no-overwrite, expendable** backup/draft files. These are raw copies, not full-container authenticated recovery images. No delete/overwrite/cleanup of unknown or partial files.
- Use a vetted **3DSX via Homebrew Launcher**, not the older crashing CIA and not a new experimental CIA installed over any existing title. Avoid the broken L shoulder button. Owner's Rosalina shortcut is **START+SELECT**; preserve it and ensure normal exit remains possible.
- If HOME Menu is blank/delayed again, console freezes, storage errors occur, a write to the installed game is suggested, or the display/source is ambiguous: **STOP**. Do not attempt in-place repairs or destructive troubleshooting.

## Preparations on a computer (no actual game edits)

- [ ] Secure an offline copy of the entire SD filesystem on a separate medium, including `Nintendo 3DS`, `3ds/PKSM-Rogue/phase16/original_container.bin` (**263168 bytes**) and `phase16/rehearsal_only.rec` (**526400 bytes**). Verify copied file sizes and hashes locally; do not publish their hashes if they could identify private progress.
- [ ] Preserve the original `imports/00000001.sav` if it exists. **Never replace an unknown existing import**. Use a separately prepared test environment or postpone this step if the one fixed import path already contains valued data.
- [ ] Verify the new Phase20 test 3DSX's SHA-256 against its successful exact-commit CI artifact. Do not infer reliability from a separately built CIA or an older successful commit.
- [ ] Prepare exactly one expendable, synthetic **131072-byte** file as `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav` only where the path is confirmed safe and empty; record its SHA-256 and a directory listing.
- [ ] Record existing contents of `backups/` and `drafts/` to distinguish all pre-existing files from new test outputs. Do not auto-clean failed/partial exports.

## Console observations (only if explicitly opted in later)

- [ ] Boot HOME normally, open Homebrew Launcher, launch **Phase20 standalone 3DSX**. Check menus show an offline import and do not pretend to have authenticated the installed save.
- [ ] Navigate using D-pad, A, B, X and R **without L**. Test B cancellation at both confirmation levels and safe exit. Do not trigger Rosalina unintentionally via START+SELECT.
- [ ] Import the **synthetic 131072-byte copy**. Compare displayed synthetic money, item count, party count and supported numeric IV/EV/PP fields against the known fixture. The game ROM build identity must still read **unverified**.
- [ ] If deliberately testing SD-only output: create **one** small allowed synthetic money/quantity/IV/EV/PP draft after two explicit confirmations; verify output is a *different* `drafts/phase18_draft_NNN.sav` and a *different* `backups/phase18_original_NNN.sav`, both exactly 131072 bytes. Do not request installed-game import/restore.
- [ ] On a computer, hash and compare the **original import before/after** (identical), new offline backup (byte-identical to import) and draft (different only within independently verified allowed byte footprint); do not rely only on on-console status text.
- [ ] For read-failure tests, use a **second expendable** copy with invalid length/corrupt checksum, and an absent import, only if the fixed path has been safely emptied in the disposable setup. Confirm failure returns an error, never a synthetic fallback represented as genuine data and never creates a new draft.
- [ ] Optional **existing Phase17 read-only hardware diagnostic**: use only the already vetted, separate standalone tool's explicit X button to re-open full-container read-only authentication after confirming the prior Phase16 evidence remains intact. Do not test recovery writes. Do not run new native receipt or restore logic.
- [ ] Exit normally, reopen the HOME Menu and installed Emerald Rogue **without attempting to import a draft into the game**. Compare the SD's Phase16 files and preserved original import sizes/hashes against the pre-test recorded values.

## Phase20 multi-edit extension, confirmed CI snapshot

Source reviewed: Phase20 PR #12 at `4894d4a0e5ac9f1a6f703791795698e09d7defab`, with successful strict/sanitizer, ARM11 and Azahar synthetic native edit/export runs [#41349743303](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41349743303). These are not physical 3DS results.

- Batch queue uses **Y** to start/review; existing A-based Money, occupied Bag and non-egg Party IV/EV/PP views stage changes; X removes the last queued scalar during review, B cancels, and the final review requires two explicit export confirmations. There is no verified new-move replacement UI on this pinned Phase20 build. Do not attempt to test move replacement by editing unsupported IDs or writing the installed game.
- **First physical pass** should only open and exit the **3DSX** without a synthetic SD file, then load an expendable synthetic import and navigate/cancel without export. Only after those passes, try a **small two-scalar batch** (e.g. synthetic money +1 and existing stack quantity +1); do not stress the full 128-entry queue before the console's free-memory/performance limits are measured.
- Unlike a plain viewer, Phase20 does create new **application-owned SD files** when export is confirmed. 'No installed game write' does **not** mean 'no SD writes'. Save snapshots can remain intact even after an interrupted SD write, but an incomplete unique backup/draft may be present.
- `backups/phase18_original_NNN.sav` is an ordinary **131072-byte SD import copy** on the same card, **never** a substitute for `phase16/original_container.bin`. See [RAW_COPY_VS_VC_CONTAINER.md](RAW_COPY_VS_VC_CONTAINER.md).

### 3DSX and CIA are separate test decisions

1. **Preferred:** use only the CI-pinned Phase20 `.3dsx` in Homebrew Launcher after checking its SHA-256 on a computer. A good emulator result does not prove native filesystem behaviour. Do not rename the user's original import or overwrite an existing application folder.
2. **Optional later, separately opted in:** the experimental CIA is a **different title** (`000400000F73A700`). The pipeline checks package title ID differs from Emerald Rogue's `0004000007D7BA00`; it does **not** prove the CIA is stable on your console. Verify its artifact hash, title ID and that the installation will not replace another installed version or change the Emerald Rogue title **before** using FBI. If there is any title ID ambiguity, pre-existing installation or earlier HOME Menu instability, **skip CIA testing**; a successful 3DSX test is enough for initial SD safety validation. No CIA is approved as a release here.
3. Never trigger an installation by opening a save file, and never use GodMode9 to inject an edited draft or write to a title. This pilot is for separate copied SD files only.

### Power loss / crash / failed SD export: stop-and-inspect protocol

1. If the app returns **Export failed, Out of Memory, SD error, incomplete write or verification error**, stop editing. Record the exact message and whether any new file appeared. Do not press A repeatedly or attempt an automatic repair.
2. If either screen freezes or becomes black, **do not assume a native save was corrupted**. Stop input; shut down using normal hardware procedure if possible. **Do not remove the SD while powered on.** On restart, if HOME takes unusually long or crashes again, stop all experiments.
3. With the 3DS **fully off**, read the SD on a computer. Preserve the **complete SD image**, then record file names, byte lengths, hashes and modification dates for `imports/`, `backups/`, `drafts/` and `phase16/`. A partial `phase18_original_NNN.sav` or `phase18_draft_NNN.sav` may remain after a failed exclusive creation. Never consider it valid just because the name exists; don't rename/delete it or re-use its number.
4. Compare with the protected **pre-run** source snapshot and backup hash. Any change to `phase16/original_container.bin`, `phase16/rehearsal_only.rec`, the original `Nintendo 3DS` tree or the fixed SD import is a hard STOP, not permission to repair or restore.
5. A **131072-byte length is necessary but NOT sufficient**. The draft must match the independently generated expected entire raw bytes/approved edit footprint, remain a separate no-overwrite file, and be parseable by the pinned synthetic verifier. An editor's 'created' message, matching file size or SHA-256 *alone* does not prove game validity or ROM semantics.
6. If the pre-run snapshot is unavailable or the export partial/uncertain, report **INDETERMINATE SD OUTPUT; NO GAME RESTORE**. Preserve raw evidence. Do not launch experimental CIA or retry with valuable data.

### GodMode9 — file inspection and SHA-256 only (no broken L button)

These choices are supported by [GodMode9's own README](https://github.com/d0k3/GodMode9#readme). GodMode9 is a powerful tool **capable of destructive changes**, so perform only the actions below; a PC SD card reader is preferable for independent verification.

1. With the console fully off and the existing setup configured appropriately, hold **START while booting** to launch GodMode9 (do not press L). Choose **`0: SDCARD`**, then navigate `3ds/PKSM-Rogue/` and only the `phase16`, `imports`, `backups` or `drafts` folders. Do not enter `A: SYSNAND SD`, `S: SYSNAND VIRTUAL` or title archive controls.
2. Select an **individual file** (L is **not** needed), press **A → Calculate SHA-256** and record the result. Repeat for the pre-existing import, each new raw backup/draft and Phase16 files; compare against protected off-SD hashes recorded before the test. Press **B** to back out of menus. An import and its raw backup must be byte-identical; a changed draft should have a different hash, but that alone never proves the intended edit.
3. Avoid **X (delete), Y (copy/paste), R+X (rename), Hexeditor/edit mode, scripts, title injection, SD formatting, write-permission unlock, and any FS operations modifying existing files**. `START` in GodMode9 reboots the console. Don't open Rosalina from GodMode9; in the Phase20 app `START+SELECT` is still reserved for Rosalina, and standalone START exits only on release.
4. GodMode9 can confirm filename, size and SHA-256 equivalence, **not the actual Phase18 v3 field edits** by itself. For independent byte/semantic verification, transfer **expendable synthetic copies only** to an offline computer and run the pinned Phase18 v3 candidate verifier (or have the QA build a safe synthetic verifier). Do not upload private game saves.

## Record without exposing user data

Capture build SHA, CI artifact digest, hardware model/firmware versions if volunteered, steps and button paths, pass/fail/error codes, exact synthetic fixture identity, filenames and output sizes, before/after local hashes, abnormal startup and storage symptoms. Avoid screenshots showing private save contents, trainer data or other identifying files. A physical read-only PASS is **only** a limited I/O/UI observation.

## Before even discussing a separately approved live restoration pilot

The following are distinct, OPEN evidence gates and cannot be satisfied by the checklist above:

1. **Immutable independent recovery sources:** two independently checked **whole 263168-byte authenticated containers** maintained off the active 3DS SD, plus a verified external restoration/recovery route and no exposure of real game bytes to public services. An offline 131072-byte import copy is NOT such a backup.
2. **Exact title/build semantics:** independent identification of the installed ROM variant and mapping/semantics for every proposed field, sector and save bank, with byte-exact before/after change-set proof. No offset guessing.
3. **Console-bound authentication:** genuine PxiFS0 file/service-bound CMAC, current game/title/CID, both slots and counters, on-device fresh read immediately before any transaction, explicit rejection of stale/corrupt/unknown record formats.
4. **Bounded durable journaling:** reviewed exclusive full-container transaction log, signed/hash-bound terminal receipts, bounded chunked writes, fs flush/close/readback, space-exhaustion handling, unknown FS error handling, interrupted writes and power-loss detection with **expendable test data only**. The present one-byte Phase14 receipt is insufficient.
5. **Controlled hardware writer:** a separately reviewed and opt-in privileged writer compatible with the target native archive and AGB_FIRM semantics, only after a demonstrated successful off-device recovery path; cannot be enabled through Phase17 status or Phase20 UI.
6. **Persistence and rollback on disposable progress:** edited bytes applied, in-game load and re-save, independent exact readback and tested recovery after interruption with an intentionally expendable profile. No destructive trial on valued user progress.
7. **Independent QA and release review:** complete deterministic strict/ASan/UBSan and real ARM11 memory/heap tests, stack limits, current pinned integration CI, manual recovery documentation, independent technical signoff and explicit owner approval for a separate pilot.

**Decision:** `LiveWritesEnabled=false`. This checklist does NOT authorise any installed-game write, restoration or firmware change.
