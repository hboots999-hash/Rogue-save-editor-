# PKSM integration checklist — read-only Phase 2

**Status:** The bridge is host-tested but has NOT been linked into PKSM or run on 3DS hardware.

## Baseline
- Upstream source: https://github.com/FlagBrew/PKSM (GPL-3.0 with notices). Preserve licence and provenance.
- PKSM root Makefile delegates its 3DS build to `make -C 3ds`.
- Use a recursive checkout with required upstream submodules.
- Do not connect the bridge to the writable retail Gen 3 Sav3 abstraction: Emerald Rogue compatibility is unproven.

## Source to incorporate from Phase 2 downloadable ZIP
- `bridge/rogue_bridge.cpp` and this repository's `bridge/rogue_bridge.h`
- `roguesm_core/include/{sector,candidate_profile,observed_profile}.hpp`
- `roguesm_core/source/{sector,candidate_profile,observed_profile}.cpp`

## Safe user flow (NOT implemented)
1. Add an **experimental Rogue inspect** menu that reads a separate *copied* raw 128 KiB SAV file, not game storage.
2. Verify exactly 131,072 bytes; call `rogue_snapshot_open`, reject a null result.
3. Display `CANDIDATE ONLY — ROM NOT VERIFIED`.
4. Display money, copy counter, party count, bag count using `rogue_snapshot_summary`.
5. Read numeric Pokémon and item values with snapshot getters. Avoid names without confirmed version-specific catalogues.
6. Release snapshot and input buffer. Do not persist edits or offer any write path.
7. Assign a separately checked application title ID and SD directory, avoiding existing PKSM and RogueSM.
8. Run native compiler, static checks, emulator launch and navigation tests. Validate clean shutdown.

## Blocking requirements for save writing
- Exact patched ROM identity/fingerprint and matching save structures
- Independently recoverable backup and rollback checks
- Draft output diff allowlist, checksums and replay tests on disposable saves
- Emulator and supervised physical 3DS verification

**Reminder:** This public repository should never contain personal saves, ROM images, keys, SD dumps or confidential backups.
