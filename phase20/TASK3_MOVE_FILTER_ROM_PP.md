# Task 3 — ROM-aware Pokémon move browser (21 November 2026)

## New read-only / experimental controls
- On a selected Pokémon, **Y** opens the move browser. From the Party screen, **X** opens the six-Pokémon move picker. Both work without an L shoulder button.
- Select one occupied move slot. The catalogue is alphabetical and name-searchable through the existing **Y** on-screen keyboard.
- From the move catalogue, press **X** (or tap **FILTER**) to cycle:
  1. **ALL** — every metadata row, including invalid/special research entries.
  2. **PP MATCH** — ordinary candidate rows whose base PP matches both compiled ROM tables. This alone never proves a legal move or authorises a draft.
  3. **PP CONFLICT** — rows with different PP in the two compiled rulesets (the UI displays both values, never picks one).
  4. **43 RESEARCH** — only the deliberately bounded 43 ordinary research move IDs in Phase18 Move V3. This is only a *first-stage* eligibility screen.
- **Left / Right** or **PREV / NEXT** navigate pages. Y / FIND edits name search; A opens review; B returns.
- Move slots show existing names when the authenticated catalogue is present; otherwise only IDs. The comparison screen labels **PP mode conflicts** and whether the selected move is a potential 43-ID SD-only draft.
- On Review, **Y** requests an *experimental SD-only draft* only if the previously existing SHA/identity/PP/43-ID proof accepts it. Touch is disabled for a locked draft. Two physical A confirmations are still required before the unique-name immutable SD backup/draft writer can run. **The installed GBA VC game is never written.**
- An original Pokémon's move can have mode-dependent base PP. For example ROM move ID 815 (Rage Fist) has base PP **10/15** in the two compiled tables. This source is **not** included in the 43 reviewed replacement IDs. Do not auto-recompute it or guess a mode.

## Exact original ROM metadata (owner-local files only)
- GBA ROM SHA-256: `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`
- Local catalogue file: `sdmc:/3ds/PKSM-Rogue/catalogs/exact_rom_move_metadata.bin`
- Exact file size: **14,040 bytes**. SHA-256: `3d2d63e5fd77371a76540d07a6c5a4d20a9ddae7acea9f03035d86936ee34fa4`.
- Includes 936 names/two PP variants. It is validated against the exact pinned ROM digest and matched byte-for-byte in the owner's local extract. **No owner ROM, CIA, personal save or private catalogue binary is committed** to GitHub.
- A 128 KiB raw copied-save is loaded only from `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`, not from the game's installed archive.
- `PP MATCH` displays **807** ordinary text-known records (excluding Struggle), while **40** ordinary entries have PP conflicts, and **43** are separately shortlisted for further SD-only draft review. These counts are different permissions; **none** authorises game restore.

## Automated checks and release gates
- `test_move_filter_v1.cpp`: reject bad indexes, preserve alphabetical order and active search, avoid stale selection after filter change, require matching catalogue source for 43-ID candidates.
- `test_move_touch_v1.cpp`: unique button hitboxes, filter/search/page navigation and no touch confirmation approvals.
- Strict compiler + ASan/UBSan host tests; ARM11 static native compile; Azahar snapshots of four filter modes and existing browser flow.
- All general 3DS `START+SELECT` Rosalina / broken L-shoulder workarounds retained.
- A user-provided matching ROM and structurally readable save do **not** prove that the game will apply the modified saved data after a live VC restore. The branch remains a **draft / experimental build**, and installed-game save writes and restore are disabled.

## Known blockers
- Ruleset-selection control flow for all 40 mode-dependent PP records.
- PP-Up semantics and recomputing max PP under exact build.
- Species-level legality and whether moves persist after actual gameplay.
- Actual-3DS testing and independently verified restoration/recovery.
