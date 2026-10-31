# Hub progression compatibility research — READ ONLY

**DO NOT EDIT HUB FLAGS, COORDINATES OR UPGRADES YET. This is not a safe writable profile for an installed Emerald Rogue ROM.**

## Reconstructed storage position

Pinned public source: [Pokabbie/pokeemerald-rogue @ 518b7b4d](https://github.com/Pokabbie/pokeemerald-rogue/tree/518b7b4d8381ce865febb581c1e534d67d19d56d).

`src/rogue_save.c` stores its Rogue-specific block after the first 10 PC boxes, at `&gPokemonStoragePtr->boxes[TOTAL_BOXES_COUNT][0]`. `include/pokemon_storage_system.h` defines 10 active boxes, 30 slots per box and 80 bytes per boxed Pokémon, and the storage struct begins those boxes at a 4-byte aligned offset. The serialized storage stream begins in logical GBA sector 5, with 0xFF4 bytes of save payload in each sector.

Therefore the first 24,004 bytes of the Pokémon storage stream are reserved for normal boxed Pokémon, placing the Rogue block at **logical sector 10 + 0xE00**. This explains the familiar `06 00 02 DF 6B 01` candidate marker.

This is a candidate format derived from public source **and an observed layout**, not proof that any exact user-installed ROM matches the source commit.

## Recognized EX 2.1.2 candidate layout

`SerializeRogueBlockInternal` uses fixed-size arrays prefixed by 16-bit lengths. The read-only probe requires:

| Element | Elements | Observed byte length |
|---|---:|---:|
| Version + mode + secret + Rogue version + species count | header | 8 |
| Quest states | 222 | 8 per record |
| Campaign states | 7 | 4 per record |
| Area-built flags | 2 | 1 per record |
| Area coordinates | 11 | **4 per record observed** |
| Upgrade flags | 6 | 1 per record |
| Home decorations (next-length sanity check) | 192 | record details not interpreted |

The **critical mismatch** is `struct Coords8`: the pinned public `include/global.h` declares `s8 x, s8 y` (2 bytes), but observed serialized saves have four bytes per coordinate entry, typically two bytes plus two zeros. Using a two-byte stride fails the subsequent upgrade array and decoration-size checks. This could be a patched build/compiler/ABI change, and it blocks all hub writes until proven otherwise.

The hub probe accepts only a consistent, explicitly recognized profile. It never scans freely for a convenient pattern, never writes a byte, and rejects malformed lengths, signatures and nonzero padding.

## Available read-only tools

- `bridge/rogue_hub_probe.{h,cpp}`: C-compatible immutable report with 11 area flags, 48 upgrade flags, counts and detected coordinate stride.
- **Hub page** in the experimental PKSM Rogue UI: use L/R to navigate to Hub, then Up/Down to inspect all named areas and upgrades. No hub write controls exist.
- `docs/hub_upgrade_matrix.json`: 48 source-defined upgrades with target area, build cost, up to two prerequisite upgrade IDs and quest/reserved cautions.
- `tools/hub_plan.py --area-mask 0300 --upgrade-mask 050000000000` (synthetic example): reports already-owned, eligible, area-blocked, dependency-blocked and quest/reserved upgrades. Does not open or modify any save.
- `tools/rom_identity.py /path/to/your-own-rom.gba`: outputs a *local-only* ROM SHA-256 and GBA header metadata. **Do not commit or upload ROM contents or private saves to the public repo.**

## Checks before enabling hub writing

1. Prove the exact Emerald Rogue build and verify why coordinate stride differs from public source. A header/version marker by itself is insufficient.
2. On a game-generated, disposable copy, check that the hub data is decoded consistently before and after normal gameplay save cycles.
3. Verify required areas, adjacency/coordinates and upgrade prerequisites in the actual game; respect reserved and quest-granted upgrades.
4. Implement game-source-equivalent serializer, preserve all unrelated variable-length arrays, and verify both GBA save-sector checksums and live game load/restore.
5. Prove that rollback restores the original working save **before** enabling changes to any actual game save on a 3DS.

No user SAV, ROM, console key or private 3DS dump has been uploaded to this public repository.
