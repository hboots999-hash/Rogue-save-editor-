# Emerald Rogue EX compatibility — verified and unresolved

Last reviewed: 30 October 2026. **Experimental development only; not a physical-3DS release.**

## Source profile under investigation
- Public candidate: Pokabbie/pokeemerald-rogue expansion ancestor `518b7b4d8381ce865febb581c1e534d67d19d56d` (EX 2.1.2-era source).
- Historical saves match candidate serialized marker 6 and sector structure, but **this does not identify the binary ROM**.
- Different Pokémon expansion field packing from ordinary Pokémon Emerald: 11-bit species; split item ID; nature modifier; two-bit ability slot.
- Candidate checksum sizes and party layout are independently probed with GBA ARM APCS for the pinned source revision.

## Verified offline functions
- Save validator: 128 KiB, sector IDs, candidate checksums and two-copy constraints, candidate marker 6/8.
- Money: 0–999999 edit; selected copy and checksum verified.
- Existing bag quantity: 1–999, item identity check and exact changed-byte allowlist.
- Party Pokémon IVs: 0–31 each; candidate box substructure decryption/re-encryption, Pokémon checksum and save-sector checksum.
- Party Pokémon EVs: 0–252 per stat and <=510 total; same constraints and identity guards.
- The IV/EV backend passed 24 synthetic encrypted substructure permutations and two private historical save samples with unchanged originals; synthetic 3DS Party X/Y screen-level host test passed [CI run 41145172921](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41145172921).
- UI: X on Party stages IVs all 31, Y stages 4 HP/252 Atk/252 Speed; START twice previews and exports separate new original and draft files.
- Prior native CIA with money/existing-item editing passed isolated Azahar A/X/START and file-export tests [run 41143432549](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41143432549).
- **The IV/EV CIA successfully built and passed an actual Azahar button/export test**: [run 41145723321](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41145723321). 3DS R/X/START exported six perfect IVs; Y/START exported 4 HP, 252 Attack, 252 Speed EVs on a separate draft. The emulator generated **two new originals and two new drafts**; each backup matched the unchanged imported synthetic save. Encrypted Pokémon bytes, internal Pokémon checksums and candidate save structure were decoded and verified for both outputs. This does not verify compatibility with a real released ROM or physical 3DS.

## Items
- Candidate constants from pinned EX 2.1.2 source: `ITEM_MASTER_BALL=4`, `ITEM_RARE_CANDY=113`.
- Two historical SAV samples contain Rare Candy quantities 899 and 954; neither contains an occupied Master Ball slot. These sample details are not stored in the public repo.
- **Inserting new items is NOT implemented.** Source `UpdateBagItemsPointers` requires contiguous grouping by pocket, while `SortItemSlotPlaceBefore` uses runtime sort mode and the game enforces reserved slots/capacity. Arbitrary writes into empty slot 449 can yield invalid game bag state.

## Hub and progression
- Candidate source serializes hub areas, coordinates and upgrades in variable-length sections after quest and campaign data, not a single fixed 1-byte unlock flag.
- No robust published-game-specific hub serializer/round-trip implementation has been verified. **Do not write hub bits using guessed offsets.**

## Required before safe release
1. Cryptographically identify the *actual* installed patched Emerald Rogue ROM/version/edition and its VC save container.
2. Validate candidate profile against a game-generated save and independent gameplay tests (money, item quantity, Pokémon IV/EV).
3. Implement and test item-pocket-aware insertion using pinned source sorting, per-pocket counts and capacity gates.
4. Implement version-specific hub serializer/upgrade dependency checks only after confirmed structure and game acceptance.
5. Test physical 3DS isolated title/storage paths, reversible backup, clean exit, install collision checks and **recoverable** restore.
6. Maintain fail-closed checks: no live-game writes until the above have passed.

**Never install an experimental CIA onto a console holding irreplaceable save data.**
