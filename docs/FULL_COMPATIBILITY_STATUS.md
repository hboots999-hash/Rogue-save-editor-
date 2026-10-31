# Emerald Rogue EX compatibility — verified and unresolved

Last reviewed: 31 October 2026. **Experimental development only; not a physical-3DS release.**

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
- **Guarded new Master Ball insertion is implemented and Azahar E2E-tested** [run 41146923377](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41146923377). It requires a valid 2.1.2 candidate source, contiguous bag, recognized type-sort mode, a contiguous ball pocket and conservative spare capacity. Other item insertion is not implemented, and no real Emerald Rogue game-load acceptance test has passed. Arbitrary writes into empty slot 449 can yield invalid game bag state.

## Hub and progression
- Added **read-only candidate hub parser** using the Rogue custom PC storage block at logical sector 10 + 0xE00. Synthetic tests verify 11 named areas, 48 named upgrades, source-defined counts, header and strict refusal of corrupt metadata.
- Real historical samples show an **observed four-byte coordinate stride**, but pinned source declares `struct Coords8` with **two bytes**. This prevents safe hub writes until the exact ROM/ABI is identified.
- Added a read-only `Hub` tab, `docs/hub_upgrade_matrix.json` with all 48 source upgrade dependencies, and `tools/hub_plan.py` to report eligibility without editing.
- No verified game-specific hub serializer/round-trip or gameplay acceptance. **Do not write hub bits using guessed offsets.**

## ROM identity
- Added `tools/rom_identity.py` to compute a local-only GBA SHA-256 + validated header report; never upload a proprietary ROM or keys to this public repository. A hash alone is NOT a game-load test.

## Required before safe release
1. Cryptographically identify the *actual* installed patched Emerald Rogue ROM/version/edition and its VC save container.
2. Validate candidate profile against a game-generated save and independent gameplay tests (money, item quantity, Pokémon IV/EV).
3. Validate the guarded Master Ball insertion against a real game-loaded disposable save; implement other item-pocket-aware insertion only where the precise game capacity and sorting are proven.
4. Resolve the observed 4-byte vs source 2-byte hub coordinate discrepancy; then validate version-specific hub serialization, coordinates, area construction and dependency-aware upgrade writes on a disposable game round-trip.
5. Test physical 3DS isolated title/storage paths, reversible backup, clean exit, install collision checks and **recoverable** restore.
6. Maintain fail-closed checks: no live-game writes until the above have passed.

**Never install an experimental CIA onto a console holding irreplaceable save data.**
