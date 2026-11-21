# Phase 21: first real in-game Pokémon MOVE REPLACEMENT PASSED (21 Nov 2026)

## Verified milestone

For the first time, a modified **actual Emerald Rogue 2.1.2 (GBA-P-H168)** 128 KiB save was accepted in mGBA, displayed a replaced move in the game's own **Pokémon → Summary → Battle Moves** screen, was resaved using the *game's own* Save/overwrite menu, and that move was still present after **closing the emulator and launching a fresh emulator process**.

**Precisely tested change:** Annihilape (internal species ID 1370), move slot 2 **Covet (ID 343, 25 PP)** → **Shadow Ball (ID 247, 15 PP)**. Rage Fist, Encore, Low Kick, species/PID/OT/nature/ability unchanged. ID247 and base PP15 match both A/B compiled PP tables from the independently SHA-pinned exact ROM. No ability override or invalid PP mode assumption.

- Exact user-owned source ROM digest: `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3` (32 MiB game ROM extracted from owner CIA; never committed).
- Source owner-private raw save digest: `cfafa07b103c41fa608caca24786fca670f267651c3e8b1d6969ab2138d83051` (128 KiB; **unchanged**).
- Offline altered *separate* copy digest: `a041d62065bafc74cbf0e776231d96e27f599e200c4b436376c6be1fbd20ebf9` (128 KiB). Only **7 bytes** differ from the original: encrypted move ID (2), current PP (1), party Pokémon checksum (2) and sector1 footer checksum (2), all whitelisted. Inactive bank byte-exact to source.
- After the game's own Save and overwrite confirmation: mGBA 131088-byte save, SHA `991a1d2be8617ccbe5d305fd224e22912f54b19b73a10e2e6961fce30909a149`. Last **16 bytes are mGBA metadata**, not raw 3DS save bytes.
- Its 131072-byte raw portion SHA `e20d3dd250903e45e0120e2337e6a08b503008371e88b60e108b0e330e0041bd`. Full independent 28/28 sector checksum and active dual-save-bank validation PASS. Bank counters 156/157 before → **158/157** after, active bank switches to 0.
- Both resulting banks contain exact same Annihilape with [Rage Fist, **Shadow Ball**, Encore, Low Kick] and PP [10,15,5,20]. PID, OT, nature and ability unchanged.
- **Fresh emulator restart:** loaded this game-generated save in a separate copied ROM+save directory and re-opened Annihilape's in-game Battle Moves tab: **Shadow Ball 15/15 PP** displayed again.

No proprietary ROM, owner save, game-specific player information, or game screenshots were committed to GitHub. All test assets remain private to the owner and are packaged in a user-only local verification ZIP.

## What this changes

**Pass:** one real H168 species/move/PP mutation successfully passes *offline edit → actual GBA game reads move → normal in-game Save → fresh emulator restart → move remains*.

**Still BLOCKED:** installed GBA Virtual Console save writing/restoration to the actual physical 3DS, authenticated CMAC/AGBSAVE behavior, verifying full rollback after a hardware injection, PC storage, unrestricted moves, nature/ability override, new-item insertion, and save-game mode transitions. This single successful test does **not** justify enabling general installed-title writing. PR #16 remains draft.

## Next focus

1. Re-run original unmodified save in a separately copied mGBA directory to document reversible rollback in the emulator; original was previously observed loading the same game sequence and original upload remains byte-identical.
2. Implement a clear read-only verification in the native CIA: exact source fingerprint, bank counters, capacity and original external-backup presence, and display an explicit emulator-verified / hardware-unverified capability label. *Do not* inject saves.
3. Prepare physical 3DS backup/rollback rehearsal based on the user's installed title **without touching the live save**; validate the copied 131072-byte AGBSAVE file and double backup off SD before asking for approval to attempt any restoration.
4. After proven real hardware rollback, enable only individually validated, revertible SD-draft edit flows and then expand as justified by per-feature game evidence.

Date: 21 November 2026. Release gate remains closed for hardware Apply.
