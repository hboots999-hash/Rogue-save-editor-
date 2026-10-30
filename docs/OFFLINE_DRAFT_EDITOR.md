# PKSM Rogue Edition — experimental offline draft editor

**Development build only. DO NOT INSTALL on a console holding irreplaceable game saves.**

## What works in source
- Reads a manually **copied**, exactly 131072-byte Emerald Rogue candidate-format SAV from `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`.
- Displays numeric party, bag, money and candidate save metadata.
- `A`: stage a candidate money edit to 999999.
- `L/R`: navigate summary, party, bag pages.
- `Up/Down`: browse party/bag entries; on bag page the *first displayed entry* is the X edit target.
- `X` on the bag page: toggle an existing item's quantity to 999; item ID must still match the imported save.
- `START`: stage+verify an independent candidate draft. `START` again: export. `B` while awaiting export cancels; otherwise exits.
- Source file is re-read and compared byte-for-byte immediately before export; changed source is refused.
- Original copy is created exclusively under `/3ds/PKSM-Rogue/originals/`; edited draft is created separately under `/3ds/PKSM-Rogue/drafts/`. Both are read back and compared before success is reported.
- No game archive, live installed Pokémon save, or existing file is ever opened for writing by these new editor functions. No automatic restore.

## What has been validated
- Independent C++ draft API uses RogueSM's MIT-licensed tested `prepareEditDraft` routine and its strict byte-diff allowlist.
- Local GCC host tests, including ASan/UBSan, accepted **both previously supplied 128KiB saves** and synthetic fixtures with a 999999 money and 999 existing-item quantity draft. Sources were not changed. These files were NOT uploaded to GitHub.
- GitHub Actions compiles and tests the offline draft engine on synthetic data. Confirm CI and devkitARM run results at the current commit before treating any build as compiled.
- Previous *read-only* CIA reached the PKSM Rogue inspection screen in Azahar and displayed a synthetic save with 123456 money. That does NOT establish the new editor buttons work on 3DS.

## Important limitations
- Candidate save markers 6 and 8 do not independently identify a patched ROM; the game may reject an exported draft.
- Source ROM version, checksum profile and in-game viability remain unverified; **never replace your sole good save with an experimental draft**.
- Only **existing** item-stack quantities are editable, not new item insertion or Master Ball/Rare Candy mapping.
- Pokémon IV/EV, species, move and hub/area upgrades are not editable in this build. A separate game-format verification is required.
- No physical 3DS testing, controller-driven emulator export/exit validation, or safe restore to the installed game has been completed.
- Upstream PKSM contains other save functionality that is not offered through our restricted Rogue UI. Source-only path rewrites and isolated title ID do not prove every inherited service harmless.
- No packaged CIA should be called a tested release until all on-console/storage tests pass.
