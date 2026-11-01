# Emerald Rogue source audit — Phase 18 (1 November 2026)

This document distinguishes **source-confirmed EX-v2.1 definitions** from the **observed private GBA-P-H168 v2.1.2 installation**. Upstream source is [Pokabbie/pokeemerald-rogue](https://github.com/Pokabbie/pokeemerald-rogue/tree/EX-v2.1), release commit `5597923` (EX v2.1). That public source has NOT been proved byte-for-byte identical to the installed user's ROM SHA-256 `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`. Source facts must not by themselves promote an edit to hardware-safe.

## Pokémon layout and semantics

Source: `include/pokemon.h`, `src/pokemon.c`.

- `struct BoxPokemon` size **80**; `struct Pokemon` size **104**. Encrypted secure substructures start at +0x20 and use personality/OT-XOR with 24 permutations, 48 plaintext bytes and a 16-bit word checksum at +0x1C.
- EX source's Growth first 32 bits pack **species 11 bits**, held item low 10 bits, Tera type 5 bits and held-item high bit. Simply decoding first 16 bits as the species yields false species when an item is held. Phase18 now uses the source-derived bit masking and includes a nonzero-held-item test. Matching-ROM confirmation is still needed.
- EX source's Misc +4 word has six IVs (bits 0..29), egg bit 30 and **Gigantamax flag bit 31**. The previous Phase18 code mistakenly labelled bit31 as ability. Corrected. Ability number instead occupies **bits 29..30 of Misc +8**, a three-way slot (0,1,2, with 3 not supported). `GetAbilityBySpecies` maps this slot to the species' selected ability, potentially with rule/revision changes; arbitrary ability IDs cannot be set through this field. Phase18 edits that two-bit slot only after explicit `verifiedAbilitySlot` evidence, and verifies Gigantamax remains unchanged.
- `BoxPokemon` byte +18 is the 3-bit language and 5-bit `hiddenNatureModifier`. Upstream `GetNature` returns `(personality % 25) XOR hiddenNatureModifier`, and `SetNatureBoxMon` changes the modifier rather than the personality. Phase18 implements the source-described, single-header-byte nature change, gated behind `verifiedNatureModifier`, preserving PID/OT, encryption, language and checksum. Actual v2.1.2 gameplay/stat impact remains untested.
- The EX source Misc +8 also packs gender/shiny/ribbon/tutor fields alongside the ability number. Phase18 edits only ability bits, preserves the other flags and verifies exact write footprint.
- All IV, EV, move and PP mutations still re-encrypt correctly and repair the per-mon and section checksum. New move IDs are blocked unless separately verified; `include/constants/moves.h` in EX-v2.1 cannot be assumed to match this exact v2.1.2 ROM build.

## Bag layout, upgrades and insertion policy

Source: `include/constants/global.h`, `include/constants/rogue.h`, `include/constants/item.h`, `src/item.c`, and `src/rogue_controller.c`.

- `BAG_ITEM_CAPACITY = 450` physical slots, **50 reserved** for key items. `ITEM_BAG_MAX_CAPACITY_UPGRADE = 37`, `ITEM_BAG_SLOTS_PER_UPGRADE = 10`. `Rogue_GetBagCapacity` returns **min(80 + 10*upgrades, 450)**, leaving 30..400 nonreserved slots and 50 reserved key slots. Stack quantity maximum in the source is **999**.
- The EX build uses nine ordered pockets: items 1, held items 2, medicine 3, stones 4, balls 5, TM/HM 6, berries 7, Pokéblocks 8, key items 9. Existing bag slots are sorted by pocket, not freely appendable.
- **Offset discrepancy resolved at the source geometry level:** public EX-v2.1
  `include/global.h` retains outdated comments (`money 0x490`, bag `0x560`) from a
  100-byte party stride. The source now asserts `sizeof(struct Pokemon)==104`;
  six records beginning at `0x238` end at `0x238+6*104=0x4A8` (money).
  The following four-byte money, four-byte mode/upgrade/reserved block and
  50 four-byte PC item slots place the bag at `0x4A8+8+200=0x578`,
  matching the historical independently observed H168 fields. The associated
  public-source candidate upgrade byte is `0x4AD` but has **not** been verified
  in a private actual 2.1.2 save. `layout_geometry.hpp` encodes and statically
  asserts all derived offsets. This resolves stale source comments without
  claiming full ROM identity.
- Source-derived bag capacity upgrades are now compared against the **read-only
  candidate byte** in the active copied save and an explicit external
  `verifiedH168BagUpgradeOffset` flag before insertion, rather than trusting an
  arbitrary caller-supplied upgrade count. Any mismatched/unverified value is
  rejected.
- The insertion implementation now REQUIRES explicit externally verified upgrade count (0..37), pocket classification for **every existing item and proposed new ID**, observed sorted pocket order, append-only placement after the final existing pocket, source-backed reserved/normal slot counts, and a confirmed inserted item-ID allowlist. It still rejects by default. Appending a new medicine item after key items would break pocket ordering and is forbidden. New insertion also cannot identify all custom IDs from the public v2.1 table reliably.
- Existing-item quantity editing remains an experimental offline-only feature with 1..999 source ceiling and exact slot/ID/current-count guards. Source semantics are not a substitute for disposable game-load tests.

## PC storage source geometry (not confirmed on H168 2.1.2)

Source: `include/pokemon_storage_system.h` and `src/save.c`.

- BoxPokemon stride 80; 14 **physical** boxes of 30 Pokémon (10 normally usable + 4 leftover), 9 logical save sections **5–13**, each section with 0xFF4 data bytes. In the C storage struct, `currentBox` starts at +0; aligned `boxes` at +4, `boxNames` at +0x8344, wallpapers at +0x83C2; four fusion slots follow.
- `pc_readonly.hpp` reconstructs a section stream using the active bank's verified sector map. It verifies populated 80-byte box records by decrypting and checking checksum, including records crossing a logical sector. It rejects invalid box indices, nonzero ambiguous empty records, corruption, and incomplete saves.
- It returns `StructuralCandidate` only if at least one occupied, checksummed box mon is observed. An entirely empty unconfirmed PC returns `UnconfirmedEmpty`; failures return `Unsupported` or `Corrupt`. **These names deliberately do not mean confirmed H168 ROM compatibility**. All PC editing is disabled. The default Phase19 model does not inspect PC unless the caller opts into research-only inspection.

## Comparison, remaining proof and Phase 16 boundary

Private pre-existing handoff: `phase9/profiles/confirmed_h168_save_layout_READ_ONLY.md` establishes 28/28 checksummed sections, observed section offsets, 104-byte party stride and XOR keys for a specific private save copy. No private bytes enter Phase18 or GitHub.

Phase 16's reported on-device authentication and recovery-journal persistence after restart improve the wider project's recovery evidence, but **cannot validate EX-v2.1 edit semantics against v2.1.2**. Phase18 has no live game IO or restore path.

Still needed to enable ROM-specific editing: an independently identified exact v2.1.2 source revision or a private, separately authorized ROM data-table comparison, a verified H168 bag-upgrade byte and pocket ID map, then disposable in-game edit/load/re-save tests and checks for PC, abilities, nature and move IDs. Until then the Phase19 UI must present them as blocked, not claimed complete.


## Phase19 snapshot integrity and 32-bit save counter rollover

- `source_digest.hpp` implements standalone SHA-256 with published empty/abc
  known-answer regressions. `SaveView.sourceSha256` seals the entire 131072-byte
  copied input; `beginRequest` and `beginMoveRequest` attach that digest.
  `propose` rejects changed copies even when the selected value and save
  counter happen to match. Direct engine users may still supply a request
  without a digest for backwards-compatible RAM-only tests, but Phase19 must
  always supply the pinned digest.
- The Phase10 inspector used a numerical
  maximum for newest bank; at a legitimate uint32 save-counter wrap from
  0xFFFFFFFF to 0 this selects the wrong bank. Phase18 now requires adjacent
  modular counters and recomputes active money, party and bag counts after
  selecting the true successor. CI tests both directions, gaps and equal
  counters.
- New `beginMoveRequest` handles slots 0–3 with slot-specific
  expected move ID and PP and blocks absent/empty move slots.
- **No
  source-derived ability/nature/move or pocket ID means that an actual v2.1.2
  game build has been verified.** These features remain gated as described in
  `PHASE20_FEATURE_MATRIX.md`.
