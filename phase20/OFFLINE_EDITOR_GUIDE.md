# Phase20 copied-save offline editor (experimental)

This draft build now connects the Phase19 menus to Phase18's v2 editor and its SD-only exporter. The installed game is never opened for writing. This is NOT a final release and its edited copies have not been proven to load in the target game.

Supported offline scalar edits: money (0–999999), quantity of an existing item (1–999), one IV (0–31), one EV (0–252 and total <=510), or PP of an existing party move (0–64). New item insertion, replacement moves, species, ability, nature and PC editing remain disabled.

## User interface

A previously prepared exact 131072-byte copied save must exist at:
`sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`

Use the seventh home option to inspect it; imported values appear in the Party, Bag and Money screens. Select Money and press A, or select a bag entry and press A, or select a party Pokémon and press A to choose IV/EV/PP. R/Right and X/Left change categories. A selects a field. Up/Down adjust by 1, Left/Right by 10, X resets. Two distinct A confirmations are required to create an offline draft; B cancels. Essential navigation does not require the L shoulder. START+SELECT remains reserved for Rosalina.

The output is always a separate, no-overwrite SD file:
- `sdmc:/3ds/PKSM-Rogue/backups/phase18_original_NNN.sav`
- `sdmc:/3ds/PKSM-Rogue/drafts/phase18_draft_NNN.sav`

The import is not modified. The exporter reopens the import, requires its whole-file SHA-256 to match the displayed snapshot, verifies the exact changed-byte footprint and re-reads both new files. The default A-based single-edit path still exports one change relative to the original import. The separate Y-based batch queue combines multiple approved scalar changes into a single draft, always locked to that same original.

There is no installed-game title archive access, no CMAC rewrite and no restore interface. Gameplay validation, 3DS hardware acceptance, independent backups and actual game save/reload testing remain outstanding. Do not install experimental CIA builds as final releases.


## Multi-edit queue — native Phase20 Y button

With a validated unwrapped SD import open, press **Y** while browsing Home, Money, Bag or Party Pokémon to start **RAM-only batch mode**. All existing A/Up/Down/R/Right/X/Left/B editing controls remain available, without relying on the broken L shoulder.

1. Select Money, an occupied bag stack or an existing non-egg party Pokémon. Choose a supported IV, EV or existing move PP field, change its value and press **A** to review it; press **A** again to **stage in RAM only**. This creates **no SD file**.
2. Return to the original copy's other menus and repeat. The status line shows the queue size. Duplicate field keys and unsupported or unchanged values are refused. Up to **128** independent changes are supported by the Phase18 v3 contract, although larger batches remain unmeasured on physical 3DS.
3. Press **Y** while browsing to open the batch review screen. Use Right/R or Left to page through edits, **X** to remove the last queued change, and **B** to return to browsing without writing.
4. On the review screen, press **A**, then **A** again to reach the second confirmation, and press **A** a final time to export **one combined draft** with a separate new original backup. **B** at either confirmation returns to review without any SD write.
5. After success the output includes `backups/phase18_original_NNN.sav` and `drafts/phase18_draft_NNN.sav`. The original import stays byte-identical and is not automatically replaced by the draft. A new queue starts from that unchanged original, not the previous draft.

**Safety:** Every queued field is locked to the original save's full SHA-256, active bank counter and original Pokémon/item identity. Each intermediate edit is checked against the current RAM candidate; the complete final draft is independently reconstructed and checked byte-for-byte before the SD-only exporter is allowed. EV totals must remain <=510; queued EV reductions can free capacity for later queued increases. Failed or uncertain files are never reported as approved output. No restoration or installed-game archive writing is present.

**Evidence:** [Phase20 Actions run #41349356154](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41349356154) passed strict host and ASan/UBSan suites, native ARM11 compilation, isolated CIA packaging, and the mandatory Azahar UI test. The emulator staged money 40001→40002 and an existing quantity 10→11, then created **one** complete v3 draft; an independent host v3 verifier proved all 131072 bytes and original/backup immutability.

This remains a synthetic, non-playable test on an emulator. The Phase20 updated 3DSX and its batch UI are **not yet accepted on physical hardware**, and the edited private 2.1.2 game save has **not** been loaded or re-saved. Do not use an experimental CIA as a final release or inject an edited draft into the installed game.
