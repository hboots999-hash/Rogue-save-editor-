# PKSM Rogue Edition (experimental)

A **separate** Nintendo 3DS save-editor project targeting Pokémon Emerald Rogue, built from a read-only RogueSM compatibility bridge with a future PKSM integration.

## Current status — Phase 2
- Host-tested read-only candidate save parsing for money, party members and bag quantities.
- No PKSM source integration yet; **no installable CIA**.
- No writing to live game saves, restore, or on-console verification.
- The candidate save structure must not be treated as proof of exact patched ROM compatibility.

## Safety
- Do not upload real saves, decrypted ROMs, console data or private backup files here (this repository is public).
- Do not install test CIAs on a console containing irreplaceable saves.
- Do not replace the original PKSM title ID. Maintain separate application identity, paths and backups.
- Never enable writing until exact ROM profile, checksum and transactional rollback behaviour are validated.

## Development stages
1. Import and host-test Phase 2 read-only bridge from the downloadable source package.
2. Pin an upstream PKSM commit (including submodules), retain GPLv3 licence and additional notices.
3. Wire a separate Rogue file selector and readonly inspection view into PKSM.
4. Build and run in an emulator, then validate safely on dedicated test hardware.
5. Add staged offline edits and strict binary diff validation only after verified version matching.

Upstream: https://github.com/FlagBrew/PKSM
