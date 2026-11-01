# Restricted physical verification — copied files and read-only title access

**Draft test plan, 1 Nov 2026. NOT a console task request, and NOT restoration approval.**
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
