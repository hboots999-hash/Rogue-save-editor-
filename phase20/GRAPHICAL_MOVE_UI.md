# Phase 20 graphical Pokémon and move browser (experimental)

This graphical UI is built on top of the existing Phase 20 copied-save editor. **It never writes to an installed Emerald Rogue title, performs CMAC signing, or makes a move legal within the game.** Save import, independent offline draft validation, and the hard release gates are unchanged.

## The screens

- **Home and Party:** emerald-themed tiles with native 3DS touch and D-pad navigation.
- **Pokémon details:** species portrait, ability-slot and nature numbers, IVs, EVs, four move IDs and PP.
- **Move browser:** tap a Pokémon, occupied move slot, alphabetically sorted move, or a search-keyboard key. Use the on-screen Back/Search/Prev/Next actions or the usual buttons.
- **Review:** displays old and proposed moves, metadata base PP and a clearly labelled optional experimental *copied SD draft* action. Tapping this action still enters the existing independently gated flow; it never applies to the installed game.
- **Confirmations:** all **A/X confirmation actions remain hardware-only**, with existing release gating. There is no touchscreen acceptance/restore action.
- The graphical controls do not depend on the console's L button. START+SELECT remains reserved for Rosalina.

## Optional species portraits on SD

No official Pokémon artwork is bundled in this public repository or CIA. A neutral procedural portrait appears when there is no owner-supplied image. The user can optionally place personally obtained sprites on the SD card:

```
sdmc:/3ds/PKSM-Rogue/sprites/0149.bmp
```

The filename is the **four-digit species ID** (149 in the example). Required BMP format is **48×48 pixels, 24-bit uncompressed Windows BMP, bottom-up, fixed 54-byte BITMAPINFOHEADER, no colour palette**. Pure magenta RGB #FF00FF is treated as transparent. Other formats and corrupt files fail closed to the procedural portrait; neither moves nor save validation depend on sprites.

Sprite files are read-only and strictly size/format bounded; no installed-game ROM extraction and no write privileges are involved. Images are cached by species; restart the preview after adding a new image.

## Validation and what remains

Host layout tests cover hit boxes, bounds and blocked touch confirmation. BMP validation tests cover malformed metadata. The native CI builds ARM11 ELF/3DSX, packages a separate-title experimental CIA and performs a synthetic Azahar smoke test.

The upper display and scalar editors still use the older console interface. The move browser and 3DS lower-screen portrait are *first graphical UX steps*. This is **not** original PKSM, and full sprites, PC editing, nature/ability mutations, installed-save writing and real Emerald Rogue game save/reload remain outside the current release gate.
