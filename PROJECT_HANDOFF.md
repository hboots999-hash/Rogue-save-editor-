# PKSM Rogue Edition — PROJECT HANDOFF
Last updated: 31 October 2026
Repository: https://github.com/hboots999-hash/Rogue-save-editor-
Development branch: `feature/phase7-native-ci`
Review PR (draft, not merge-ready): https://github.com/hboots999-hash/Rogue-save-editor-/pull/2
This is the durable, privacy-safe checkpoint for continuing work in a new ChatGPT Project chat.

## User goal
Create a separate Nintendo 3DS PKSM Rogue Edition for a patched Pokémon Emerald Rogue 2.x Virtual Console save. MVP: safely work with an *exported/copied* 128 KiB save to view Pokémon party and bag, edit money, existing items, Master Balls/Rare Candies, Pokémon IVs/EVs and moves; eventually back up and restore game saves safely. After MVP: species, abilities, natures, held items, PC Pokémon, hub upgrades/areas.

## Verified achievements
1. Independent C/C++ Emerald Rogue candidate save parser/bridge reads summary, money, party and numeric bag; tested with synthetic and a private copied 128 KiB candidate save.
2. Offline editing code for money (999999), existing item quantities (up to 999), Master Balls / Rare Candies insertion, Pokémon IV/EV presets and individual move replacement; explicit changes, readback/checksum and distinct draft files; NOT verified by an exact user's ROM/game playthrough.
3. Phase 8 backup gate before first edited draft: re-read imported **SD copy**, reject if modified mid-session, create a unique untouched `/3ds/PKSM-Rogue/backups/import_NNN.sav` and read back byte-for-byte; refuse export on failure. Draft output `/3ds/PKSM-Rogue/drafts/rogue_NNN.sav`, no live game-save writes or restoration. Native Phase 8 build passed: https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41157334536 ; synthetic host + ASan/UBSan tests passed: https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41157776550 .
4. PKSM Rogue native source compiled into ELF, 3DSX and CIA via GitHub Actions with devkitPro; build attempts are CI artifacts only (typically expire after 7 days), NOT tested/safe releases. The fork uses separate title ID `000400000F73A200` and a patch isolating 76 inherited `/3ds/PKSM` SD references into `/3ds/PKSM-Rogue`. Title ID still requires conflict assessment.
5. Independent Phase 9 read-only 3DSX, `phase9/main.cpp`, uses the exact save parser without full PKSM extdata startup. Native compiled and booted in Azahar 2126.2, displaying a fully synthetic save: money 40,000, one party Pokémon, 3 bag entries, Pokémon species numeric ID 25 with HP IV 10. R-button navigation from summary to party page verified by captured screenshots: https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41158813605 . Read-only, no game save writes.
6. A local-only offline backup/draft/recovery **rehearsal** ran on temp files with synthetic and privately copied saves, including checksum, file-size, changed-byte and failure tests. This is NOT installed-game restore validation. The original private save and ROM are NEVER to be added to the public repo.

## Blockers — do NOT overclaim
- Full PKSM Rogue graphical editor has NOT been shown navigable in a working emulator. Both untouched official PKSM 10.4.2 and the fork show the same extdata-initialization warning in a fresh Azahar profile (control: https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41156495959). This does not establish a defect in Rogue code or a device-safe bypass.
- Exact patched Emerald Rogue ROM/build mapping is unknown. Synthetic/candidate save tests do not establish valid in-game behaviour for all edits, item IDs, hub flags or upgrades.
- Direct 3DS installed-game archive backup/recovery, restoration and launch/exit testing are unverified. Previous experimental standalone RogueSM CIA reportedly caused a HOME Menu data abort on the user's physical console.
- Species/ability/nature/held-item/PC/hub upgrade editing is not MVP-complete.
- **DO NOT install the experimental CIA on the user's physical 3DS, delete/reset PKSM extdata, write to live VC save, or distribute a "final" release.**
- GitHub Actions artifacts are temporary (7-day retention in these workflows). Permanent source is in the branch; this HANDOFF note, source archive and patches survive individual artifact expiry.

## Where the code lives
- GitHub repo branch `feature/phase7-native-ci`: `phase7/archive/01.b64` … `07.b64` together form a verified XZ TAR source-only archive (the Phase 7 base); `phase7/verified_backup_gate.patch`; `phase7/patch_sd_isolation.py`; `phase7/create_synthetic_fixture.py`; `phase7/host_backup_smoke.cpp`; `phase9/main.cpp`.
- Workflows: `.github/workflows/phase7-native.yml`, `phase8-host-regressions.yml`, `phase9-readonly-diagnostic.yml`; older emulator and stock-PKSM control workflows.
- Full code, compile logs and CI evidence linked in draft PR #2. Do not confuse read-only diagnostic 3DSX with the full PKSM fork or hardware release.

## Next exact work items (priority order)
1. Validate clean test-image configuration for *stock* PKSM extdata; if still blocked, test editor UI without unsafe extdata bypass, or port MVP UI into the already-working independent 3DS read-only shell with backup-gated **offline** drafts.
2. Test native interface controls (open, page switch, select slot, double-confirm, cancel, exit) and force-failure/readback behaviour in Azahar using entirely synthetic save files.
3. Add a ROM-version-specific compatibility gate: refuse potentially unsafe draft writes when game mapping is unverified; retain untouched backup and byte-diff log.
4. Exercise a disposable game-save backup/restore pipeline on correct versioned save/ROM, then a carefully planned hardware launch test that cannot overwrite the existing save/PKSM installation.
5. Only after all release gates pass, package signed/identifiable release assets with checksums and clear installation/restore instructions. Broader species/ability/hub editing later.

## How to continue in ChatGPT
Open a **new chat within the PKSM Project**, and say:
"Continue PKSM Rogue Edition from PROJECT_HANDOFF.md in our GitHub branch feature/phase7-native-ci. Read the checkpoint and the draft PR #2, inspect current CI results, then implement the next verified MVP work item. Do not redo completed phases, ask for my private save/ROM unnecessarily, claim a CIA is safe, or push private files into public GitHub. Work in source with tests and update the checkpoint."

## User-facing expectations
The user is eager to finish. Make actual tested changes where possible, provide working links and an honest distinction between source tests, native compile, emulator launch and in-game/hardware safety. Keep brief handoffs in GitHub/Project files as chats get long. Do not promise asynchronous delivery.
