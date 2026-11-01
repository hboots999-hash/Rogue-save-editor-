# Phase 19 — Nintendo 3DS editor UI demonstrator

**Status: synthetic demonstration only, read-only with respect to all save data.** No real saves are read, no SD files created, and no native save or restore service is linked. This is an isolated new native homebrew UI, not a release of PKSM Rogue Edition.

## Implemented

Native top/bottom libctru console interface: home menu, three-member example party, two sample PC boxes with navigation, Pokémon details, IV/EV/move/ability/nature editing menus, item inventory and quantity proposals, money proposals, backup management and recovery help.

Values are fictional. Ranges in this demonstration: IV 0–31, EV max 252 and total max 510, move index 1–1024, ability index 0–255, nature index 0–24, money 0–999999, item quantity 0–999. These limits do NOT authorize edits for the actual Emerald Rogue v2.1.2 title. Every editing workflow previews in-memory numbers, requires two explicit confirmations, and **ends BLOCKED** without changing even the demo fixture. B cancels at every step. "Backups" and "Restore" never access files; they show an unavailable status rather than inventing a backup. No external private assets, screenshots, ROMs or saves are included.

## Controls

Home: Up/Down then A. Party/Bag: Up/Down and A. PC: L/R switches boxes, A opens an example Pokémon. Pokémon detail: A or X opens edit categories. Edit: L/R selects IV/EV/Moves/Ability/Nature, Up/Down chooses field, A opens a number proposal. In proposal: Up/Down +/-1, Left/Right +/-10, X resets, A reviews, B cancels. Next A shows confirmation two; final A shows BLOCKED. START exits safely.

## Build and isolation

GitHub workflow: .github/workflows/phase19-editor-ui.yml. Host C++ tests verify navigation, bounds, confirmations, cancel, demo data immutability and blocked backup/restore. CI builds a standalone 3DSX/ELF with devkitPro/libctru and packages a distinct **experimental** CIA, both with SHA256 receipts; optional isolated Azahar UI startup smoke captures screenshots but cannot test actual game or source identity.

Phase 19 preview Title ID **000400000F73A500**, separate from the installed Emerald Rogue title **0004000007D7BA00** and the previous native preview **000400000F73A400**. Check title IDs before any hardware test; prior prototype CI success cannot rule out real-console crashes.

## Integration requirements

1. Keep UI State independent of read/write adapters. Add a future **explicit read-only provider** after validation for the exact H168 save version; report unsupported PC layout as unavailable rather than guessing. No real data is loaded in Phase 19.
2. For a real editor, each field must have independent Phase 18 real-save mapping validation, valid species/item/move IDs, capability gating and Pokémon checksum verification. Synthetic limits alone are not enough.
3. Authenticated GBA VC restore and rollback are a separate gate; leave installed-game writes impossible until that gate and physical gameplay persistence tests pass.
4. Existing Phase 9–13 code, restoration modules and other branches were not modified. Do not merge this PR as evidence that real save editing works.
