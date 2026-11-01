# Phase20 copied-save offline editor (experimental)

This draft build now connects the Phase19 menus to Phase18's v2 editor and its SD-only exporter. The installed game is never opened for writing. This is NOT a final release and its edited copies have not been proven to load in the target game.

Supported single-field offline drafts: money (0–999999), quantity of an existing item (1–999), one IV (0–31), one EV (0–252 and total <=510), or PP of an existing party move (0–64). New item insertion, replacement moves, species, ability, nature and PC editing remain disabled.

## User interface

A previously prepared exact 131072-byte copied save must exist at:
`sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`

Use the seventh home option to inspect it; imported values appear in the Party, Bag and Money screens. Select Money and press A, or select a bag entry and press A, or select a party Pokémon and press A to choose IV/EV/PP. R/Right and X/Left change categories. A selects a field. Up/Down adjust by 1, Left/Right by 10, X resets. Two distinct A confirmations are required to create an offline draft; B cancels. Essential navigation does not require the L shoulder. START+SELECT remains reserved for Rosalina.

The output is always a separate, no-overwrite SD file:
- `sdmc:/3ds/PKSM-Rogue/backups/phase18_original_NNN.sav`
- `sdmc:/3ds/PKSM-Rogue/drafts/phase18_draft_NNN.sav`

The import is not modified. The exporter reopens the import, requires its whole-file SHA-256 to match the displayed snapshot, verifies the exact changed-byte footprint and re-reads both new files. Each draft contains only one change relative to the import, not a sequence of accumulated edits.

There is no installed-game title archive access, no CMAC rewrite and no restore interface. Gameplay validation, 3DS hardware acceptance, independent backups and actual game save/reload testing remain outstanding. Do not install experimental CIA builds as final releases.
