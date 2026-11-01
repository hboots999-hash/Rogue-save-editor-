# Phase 19 — explicit read-only copied H168 save integration

The native app **starts in the original fictional demo mode**. The seventh home menu item, "Inspect SD copy (read-only)", allows an explicit user-selected inspection of only a separately prepared *unwrapped, 131,072-byte* H168 save at:

    sdmc:/3ds/PKSM-Rogue/imports/00000001.sav

The importer opens the file in read-binary mode and closes it after checking. It never opens any installed title/game archive. A missing path, wrong size, 263,168-byte wrapped/authenticated GBA VC container, corrupt sections or inconsistent dual-bank counters is rejected without changing the current screen data. Never truncate a 263,168-byte backup or overwrite the game save to satisfy this path. Only use an independently verified **copy**.

Validated read-only display includes money, active bank counter, up to 6 party Pokémon (numeric species IDs, IVs, EVs, move IDs, PP, ability slot and effective nature index), and up to 450 existing bag item IDs/quantities. Party nickname/level and ROM-specific Pokémon/item naming are not supplied by the Phase18 Mon API; the UI shows "Species n" and "Lv?" rather than guesses. For actual data the PC reports UNSUPPORTED because Phase18 has not validated PC storage. Actual-data editing, new item insertion, draft export and restore are ALL disabled. Backups are NOT CHECKED IN APP. Demo mode can be reselected with X from the Import screen.

The Phase19 adapter uses Phase18's current EX v2.1 read-only decoder. IMPORTANT: ability slot is a **two-bit field, 0–3**, separate from the Gigantamax flag; the imported nature index uses the validated effective-nature modifier rather than only PID %25. This UI never calls any mutation, export, backup or restoration API.

## Pinned Phase18 build dependency

GitHub Actions checks out read-only dependency revision 596743d8eaa073759a736b5a8107db43f3a8c17b into a separate checkout directory, then uses the unchanged Phase18 parser and its bag_rules and Phase10 dependency to compile the Phase19 3DSX/CIA. PR #8 and branches Phase14–17 are not merged or modified. Later Phase18 API updates require explicit review and repinning.

Tests use a *generated, non-playable* 128-KiB dual-bank fixture from Phase18, not any private game save. Host tests validate displayed party, EVs, IVs, moves, PP, two-bit ability slot, bag, money, read-only controls, and corrupt/wrong-size input refusal. Build and emulator checks cannot certify a physical-console copy-read or authenticate the user's actual game state.

## Hardware safety steps

First verify that Emerald Rogue still starts and that independent original game backups remain safe. Test the replacement **3DSX**, not the previously crashing experimental CIA. Without any imported copy, opening the seventh menu entry and pressing A should report that the fixed file is missing, then B should return safely to the menu. Only later, if an independently verified 131,072-byte *unwrapped copy* is available, place it at the fixed path and test viewing its numeric party/bag/money data. Do not install a new CIA, edit actual save bytes, restore or inject any draft as part of Phase 19.

Status: integration candidate, not yet a physical-hardware read-only import PASS, PR #7 remains draft and unmerged.
