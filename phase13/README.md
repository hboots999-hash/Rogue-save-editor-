# Phase 13 — Live-save-derived SD drafts, native restore preflight

**Status:** Source and host tests implemented; live title WRITE remains DISABLED. This phase reduces manual GodMode9 workflow, but does not yet change money or Pokémon in the installed game.

## What changed

- **Y** reads installed Emerald Rogue 2.1.2 GBA VC save via the already hardware-verified read-only Luma PxiFS0 bridge. It identifies target title `0004000007D7BA00` and validates 128 KiB/dual-bank checksums.
- From LIVE view, **A → A** continues to make a verified new, immutable `backups/live_NNN.sav` (Phase 12).
- NEW from LIVE money or existing-item page, **X → A → A** directly prepares a new +1 money or existing-item quantity draft **from a fresh live read**. It *first* produces an exact, fresh read-only `live_NNN.sav` backup, checks that the live save has not changed, edits a RAM copy, and writes unique `drafts/liveedit_NNN.sav` to the SD card. It rereads the draft, validates both save banks/Pokémon checksums, and compares against the untouched live backup. It rechecks the live save again before reporting success.
- It does **not** replace the live save. Older `imports/00000001.sav` workflow remains separate.
- New `restore_preflight.hpp` authorizes *only a controlled candidate shape*: on the latest valid bank, money +1 OR an existing item quantity +1, with only encrypted field bytes + current section checksum changed. It rejects stale/corrupt records, different sector mapping, modified Pokémon, inactive bank changes or unsupported edits. This authorization **does not permit live writing**.

## Checks

Host regression on a deterministic, **nonplayable synthetic save**: limited money and item +1, exact byte footprint, preserved older bank and original, unique backup and draft, 128-KiB roundtrip, bad checksum, modified unrelated byte, missing item, stale source and unsupported operations rejected. Native 3DSX/ELF compilation and Azahar offline smoke regression are in `.github/workflows/phase13-live-draft.yml`.

The live PxiFS0 read path is verified on user's physical hardware in Phase 11. The new live-derived draft action still requires an **independent physical 3DS test**; emulator runs cannot exercise privileged Luma service. Use only after an external GodMode9/Checkpoint recovery backup. Any new liveedit draft must stay **out of the installed game** until the separate CMAC-signed direct restore and rollback gate passes.

## What is still missing before normal PKSM Rogue Edition

1. A separate, properly authenticated direct GBA VC save restore module. Nintendo's AGB_FIRM validates a console/title-specific CMAC; copying raw 128-KiB bytes into the save archive without updating the CMAC is insufficient. The recovery path must be tested on expendable game progress, including game launch, save and return.
2. Port remaining party IV/EV/move edits, Pokémon selection, new items, party insertions, hub unlocks and intuitive PKSM UI from toy-only Phase 9 to the exact H168 real save format, with per-feature game-play validation. **Do not claim features are done from toy tests alone.**

Upstream restoration reference: [Checkpoint `3ds/source/gbasave.cpp`](https://github.com/BernardoGiordano/Checkpoint/blob/master/3ds/source/gbasave.cpp) (GPL-3.0). The only user ROM/save fingerprint permitted in the public repository remains metadata, not any copyrighted ROM or personal save bytes.
