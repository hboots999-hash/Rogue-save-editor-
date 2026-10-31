# PKSM Rogue H168 — Independent CIA hardware preview (Phase 10)

**Experimental 3DS homebrew application, not finished PKSM Rogue Edition.**

This CIA is separate from the installed Pokémon Emerald Rogue game and uses a distinct native title ID \`000400000F73A400\`. It does not call or mount GBA VC save services, GodMode9, Checkpoint, Nintendo's live title archive, or PKSM extdata. It does not edit or restore the installed Emerald Rogue save. The CIA has no ROM or personal save inside it.

## What the screen actually does

- Reads a **separate user-copied** unwrapped 131072-byte save under \`sdmc:/3ds/PKSM-Rogue/imports/00000001.sav\`.
- Requires two complete valid banks of 14 sectors, section signatures and checksums, strictly increasing counters, all 104-byte populated Pokémon party encrypted-block checksums, contiguous bag item entries with quantities 1–999. This is structural validation, not a proof of specific ROM identity.
- Shows current money, party count, numeric Pokémon species ID, bag item ID/quantity, bank counter and page navigation.
- On **Summary**, X stages only **money +1**; on **Bag**, X stages only **selected existing item stack quantity +1**. No Pokémon editing in this preview.
- First A and second A confirm a single per-app-launch offline export. B cancels without writing anything.
- Rereads and compares the imported SD copy before backup and again after backup. Makes **new exclusive** untouched backup \`sdmc:/3ds/PKSM-Rogue/backups/import_NNN.sav\`, reads it back, then writes distinct draft \`sdmc:/3ds/PKSM-Rogue/drafts/h168_NNN.sav\`, reads it back. Does not delete/overwrite any copy.
- Only touches the target bank's encrypted numeric field and sector checksum, and re-parses the resulting draft to confirm the new value.

## This is NOT the finished app

The previously green Phase 9.4 toy-only code supports changing money/quantity, IVs, EVs, move slot/ID/PP, Master Balls/Rare Candies, **on a deterministic synthetic save only**. The new Phase 10 preview deliberately exposes only the two small operations whose corresponding formats were independently checked on a *copy* of the exact user's H168 v2.1.2 real save. It cannot restore any draft to the game, and the app does not have authority to write to the live save.

The user's prior GodMode9-injected money+1 test reportedly loaded correctly and displayed the expected amount on their physical 3DS. That is evidence of game-side numeric acceptance **but not of the CIA's 3DS startup stability**. Previous unrelated RogueSM prototype crashed the 3DS HOME Menu, which is why an isolated CIA hardware preview remains an **experimental test with known risk**.

## If user explicitly chooses to test this preview

1. Keep an independent **recent** GodMode9 original GBA VC save copied off the SD card and a recent Checkpoint backup. Never rely on just one copy.
2. Confirm no app/title collision with \`000400000F73A400\`. Old PKSM fork \`000400000F73A200\`, lab \`000400000F73A300\`, previous crashed RogueSM \`000400000F73A100\`, original game \`0004000007D7BA00\`.
3. Prepare a **copy only** of the original current GBA VC 128-KiB save, named \`00000001.sav\` under \`/3ds/PKSM-Rogue/imports/\`. The title's installed save is never read by this preview. If that copy was taken before recent gameplay, its money and bag contents are naturally out of date.
4. First install the independently packaged, checksummed experimental CIA from successful CI, launch it, inspect displayed money/party/bag and exit with START. **Do not stage edits until launch/exit is stable.**
5. Only if stable: X then A/A to export money+1 draft. Verify on SD that \`backups/import_001.sav\` equals the imported file and that a distinct \`drafts/h168_001.sav\` appeared. The app never injects it.
6. If HOME Menu or the app crashes, do not relaunch repeatedly. Get the crash screen/dump, power off and, if safe, remove **only the distinct preview title** through FBI. Do not remove/reinstall Emerald Rogue or PKSM itself.
7. Do not restore the draft to the installed Emerald Rogue game as part of this preview; restoring must be a separate deliberate GodMode9 experiment with an independent recovery procedure.

**No guarantee** that a homebrew CIA cannot cause a system crash, SD corruption, or interfere with other installed software. The distinct title ID and no-game-access design reduce potential damage but cannot eliminate all risks.

## Verification

- Native host and ASan/UBSan safety tests on purely synthetic two-bank fixture.
- Native devkitPro 3DSX/ELF compilation.
- Automated Azahar guest exercise of A+A offline money+1, with independent draft verification and untouched backup.
- Isolated Azahar install of built CIA and launched installed title content.
- All of these checks are required to pass on the latest source revision **before** sharing any preview CIA. Physical 3DS test remains pending.

Never add user's real ROM/CIA/flash save or private fingerprints to public GitHub.
