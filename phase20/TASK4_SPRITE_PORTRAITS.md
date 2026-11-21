# Task 4 — Graphical sprite portraits (21 November 2026)

## Implemented

- **Optional true per-species artwork**: `sdmc:/3ds/PKSM-Rogue/sprites/%04u.bmp` where the ID is the *exact game's internal species ID*. Sprites load from **owner-local SD files only**. Neither the Pokémon Emerald Rogue CIA nor its game archive is accessed, and no Pokémon image assets are published in the repo.
- Reader strictly accepts 48×48 **bottom-up, 24-bit BGR**, uncompressed Windows BMP, exact declared pixel size, fixed bounded offset and matching real length. Pure magenta `#FF00FF` is treated as transparent during rendering.
- **10-slot bounded least-recently-used sprite cache**, static pixel budget 69,120 B. The old `species%8` direct-mapped cache caused unnecessary collisions. Cache key always includes internal species ID; missing and malformed file results are cached separately.
- Missing/invalid portraits show a procedural ORIGINAL generic creature avatar with a visible question mark, and the details screen reports **NO ART**. They never produce an unrelated Pokémon's portrait.
- Cache tags reset after SD-copy import or switching to demo so updated local files are rechecked without modifying save data. Negative-cache entries are evicted naturally as other species are viewed.
- `phase20/tools/make_species_bmp.py`: offline PNG-to-exact-BMP conversion. Reads only a user-specified local PNG, uses Pillow on user's computer, writes output with `open(...,'xb')` to refuse overwrites. Resizes within 46×46 preserving pixel edges and places it on transparent magenta.
- `phase20/tools/generate_demo_sprite_badges.py`: synthetic, ORIGINAL abstract sunburst/crystal/constellation images **only for UI tests**, not depictions of Volcarona, Swampert or Gardevoir. No copied Nintendo/Pokémon artwork.
- `phase20/test_sprite_bmp_v1.cpp`, `phase20/test_sprite_cache_policy_v1.cpp`: malformed BMP, exact size and field constraints, LRU eviction, collisions, reset, missing/invalid negative caching, unsupported internal ID, counter rollover.
- Azahar CI installs synthetic BMP art into the demo *SD sandbox*, captures party/detail screenshots and verifies the art remains byte-identical after save edit / CIA boot.

## Using your own art

1. Obtain or draw a square transparent PNG that you are permitted to use. Do not share copyrighted artwork publicly without permission.
2. On a computer with Python and Pillow installed:
   `python3 phase20/tools/make_species_bmp.py 637 MyPortrait.png --out sprites`
3. This creates `sprites/0637.bmp` (6,966 bytes). Put it on your SD card at `/3ds/PKSM-Rogue/sprites/0637.bmp`.
4. Use the game's **internal species ID**, not blindly the National Pokédex number (form slots and Emerald Rogue custom IDs can differ). No game/save writing is required.
5. Open the editor and import the SD save copy to refresh local portrait cache. If a file is missing or malformed, the app retains its readable placeholder.

## Release constraints

- Every species' true front sprite from the exact owner ROM **has not been extracted** and is not bundled. No claims about sprite accuracy without actual source images.
- This is an independent graphical UI upgrade; direct installed VC save modifications, ability override, and unverified move replacement remain disabled.
- App still defaults to the experimental read-only/safe offline-draft flow. Azahar CI cannot substitute for testing on the physical Nintendo 3DS.
