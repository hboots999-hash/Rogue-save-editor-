# Experimental Master Ball insertion — source-derived candidate, offline only

**Not validated against any player's installed Emerald Rogue ROM. DO NOT install this CIA on a valuable console or restore these edited drafts directly over a sole save.**

## Scope

The `rogue_make_master_ball_draft` engine creates a new **128 KiB** file with **one 999 Master Ball stack (item ID 4)** while preserving all previously occupied bag records. It does not alter the installed game, the original imported save, any existing item quantities, any party Pokémon, money, or hub flags. The application uses the existing two-step backup/export interface.

Use the PKSM Rogue experimental app's **Bag page → Y → START (stage and validate) → START (export separately)**. The original imported file is reopened and compared before export. The editor writes only new files under `sdmc:/3ds/PKSM-Rogue/originals/` and `drafts/`.

If Master Ball is already owned, insertion is refused; select the existing Master Ball stack and use X to stage its quantity to 999 instead.

## Pinned game-source evidence (not ROM identity)

Pokabbie/pokeemerald-rogue revision `518b7b4d8381ce865febb581c1e534d67d19d56d`:
- `include/constants/items.h`: `ITEM_MASTER_BALL = 4`; ball IDs 1–28.
- `src/data/items.h`: Master Ball is a Pokémon Ball pocket item.
- `src/item.c`: `UpdateBagItemsPointers()` assumes pocket-order contiguous grouping. Under type sorting `SortItemPlaceBefore`, ordinary Poké Ball, Great Ball and Ultra Ball have ordering scores 0/1/2, Master Ball has 3, other balls have scores >=10.
- `src/rogue_controller.c`: `Rogue_GetBagCapacity()` allows 80 base slots + 10 per upgrade, max upgrade 37, max 450 slots.
- `include/constants/global.h`: `BAG_ITEM_RESERVED_SLOTS=50`. For safety the editor counts **all** occupied items against only unreserved capacity, possibly refusing a valid bag rather than risking overfill.

## Fail-closed eligibility
- Only observed candidate format **EX 2.1.2 marker 6** (not marker 8).
- Only recognized and checksum-valid rotated sectors, including the other-copy integrity and parity checks.
- Only default type sorting (mode 0) and a plausible source-derived upgrade field (0–37).
- Existing bag is contiguous from slot 0, with **no holes and no items beyond the first empty slot**.
- Existing Poké Balls occupy a single contiguous group; basic ball IDs 1–3 appear in source sort-score order before other ball types. All ball IDs are source-defined 1–28.
- At least one Poké Ball stack must exist. Master Ball cannot already exist.
- At least one conservatively available bag slot after reserving 50.
- Only existing item-slot bytes starting at the insertion position through the newly occupied slot plus selected save-sector checksum may differ; all prior items, quantities, party and money are verified.

## Verification

- Local host C++ test and ASan/UBSan tests passed on two private historical 128 KiB saves. The latest copy had 245 occupied slots and accepted a 999 Master Ball stack with 131 modified bytes; older backup had 246 occupied slots and accepted with 74 modified bytes. No originals were changed, and **no personal saves have been committed to this public repository**.
- Synthetic CI regression tests also cover repeated insertion refusal, missing space, wrong sort mode, holes, corrupt checksum, unsupported marker, and unchanged output on refusal.
- A host simulated UI test uses Bag Y + START + START to export separate original and draft files.
- **Native ARM / 3DSX / experimental CIA compilation passed:** [build 41146520997](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41146520997). CIA 5,055,424 bytes, SHA-256 `88188526b5000c0d0019f6db7d9917bad6dbb0a5cd0b49e92131c57d982e5607`.
- **Actual Azahar Y/START/START test passed:** [run 41146923377](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41146923377) booted the compiled CIA with a synthetic source save. It generated one original backup byte-for-byte identical to the import and one new 128 KiB draft. Independently parsed bag contents: item IDs `[1,2,4]`, quantities `[50,75,999]`. Verified sector checksum and strict changed-byte range. The screen displayed `Saved original + draft with readback`.
- **An emulator-generated draft is not proof that the patched Emerald Rogue ROM can load the result.**
- Physical 3DS side-by-side installation, valid ROM fingerprint, actual game-load-and-save acceptance, and verified rollback remain outstanding.

## Still not implemented
- New Rare Candy insertion if not already in bag, arbitrary new item choices, species/move changes, hub/area unlocks, complete game restore or safe on-device editing.
