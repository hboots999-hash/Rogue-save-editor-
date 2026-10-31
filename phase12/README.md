# Phase 12 — Current installed Emerald Rogue save → verified SD backup

**Status:** The live NAND read was **verified on the user's real 3DS in Phase 11** (title \`0004000007D7BA00\`, money 556800, counter 150; the imported older SD file showed money 556799/counter 149). This phase adds a user-triggered **read-only backup of the installed title**. Do not confuse an SD backup with editing or restoring the game.

## UI behavior

- Start the installed experimental PKSM Rogue CIA, press **Y** to read the current installed GBA VC save (as in Phase 11).
- In **LIVE SAVE - READ AND BACKUP ONLY** view, press **A** to propose copying the current live save to SD. A confirmation page says that **A** makes the SD-only backup, **B** cancels.
- On confirmation, the code **reopens the installed title for READ ONLY**, reads the current 128-KiB raw save twice, independently verifies sector/party integrity and byte equality, then creates \`sdmc:/3ds/PKSM-Rogue/backups/live_NNN.sav\` using the already hardware-verified SDMC exclusive creation API. Each saved byte is read back and compared; then the live save is read a **third time** for a stale snapshot check. Existing files are not overwritten or deleted. Output is valid only on exact equality. Failure is displayed, and no restore is attempted.
- Once per application launch; restart for another live backup.
- The normal SD import path \`/3ds/PKSM-Rogue/imports/00000001.sav\` and its separate offline-money/item +1 draft function remain available from **B**. They do not operate on the live save directly.
- **No FSPXI_WriteFile, no automatic restore, no installed-title modify, no CMAC signing is implemented in this phase.** An SD backup must not be injected as an edit while we are still validating recovery behavior.

## Tests / gates

- New: \`phase12/live_backup.hpp\`, \`phase12/main.cpp\`, and \`phase12/test_live_backup.cpp\`.
- Synthetic-only host regression: exact reread, unique \`live_001\`/\`live_002\` backup creation, byte-perfect backing file, unchanged original, rejection of changed data before backup, warning when changed after backup, no output on read failure.
- Native 3DSX/ELF build using prior Phase 11 verified \`PxiFS0\` live reader.
- Azahar emulation of existing **offline** behavior with toy fixture. Emulator **cannot exercise** Luma privileged PxiFS0 live reads; that path must be separately tested on a physical 3DS.
- Packaging CIA and emulator-installed smoke regression. No personal ROMs, save bytes or user file hashes are committed to public source control.

## Expected live hardware proof

Screenshot should show \`LIVE backup OK: live_001.sav\` (or another available numbered file). Independently inspect \`SD:/3ds/PKSM-Rogue/backups/\` in GodMode9: the new file must be **131,072 bytes**. Confirm live game's money and gameplay remain unchanged, and that the title still opens after closing PKSM Rogue. Keep external GodMode9/Checkpoint recovery backups throughout.

**Next stage, separate:** re-read the live backup from the app as an offline input, prove backup freshness and exact matching, test disposable restoration/recovery with proper 3DS GBA VC CMAC (Checkpoint's implementation), and only then consider enabling live writes after two user confirmations. The existing physical live money+1 injection experiment was done with GodMode9; that does not validate the direct PKSM Rogue restore mechanism.
