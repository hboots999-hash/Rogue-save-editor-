# PKSM Rogue Edition — release gates (31 October 2026)

**Current artifacts:** (1) Phase 9.4 full-feature *synthetic-only* lab, not for installation; (2) Phase 10 minimal H168 SD-copy-only preview CIA/3DSX with physical startup unverified and explicitly experimental.

The branch has an extdata-free native 3DS editor that reads a copied 128-KiB SD import. Phase 9.4 links already-developed offline editors for money, quantities, party IVs/EVs, moves (ID/slot/PP), and permitted ball / Rare Candy stack updates or insertion. It supports range-limited on-device numerical controls and two A confirmations. A **full-file exact synthetic fixture match** is required before any draft export; any other file is view-only. Exports create unique untouched backup and unique separate draft on SD with byte readback. No installed-game archive is mounted or restored.

This is *not* release-equivalent to an installed game save editor, even if compiling ELF/3DSX/CIA and launching in Azahar succeed.

## Gates

| Gate | Required evidence | Status |
| --- | --- | --- |
| A. Source and synthetic host regression | Editor API bounds, 510 EV sum cap, items/ball pocket validation, unchanged input, distinct backup and draft, exclusive create, stale import, failures, ASan/UBSan | **PASS** [41194538435](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41194538435) |
| B. Native emulator guest operation | L/R navigation, money, quantity, adjustable IV/EV, move slot/ID/PP, Master Ball / Rare Candy actions; byte-level draft/backup plus cancel and altered-save no-write checks | **PASS synthetic-only** [41194840094](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41194840094) |
| C. CIA installed-content test | Experimental isolated title installs in Azahar, loads synthetic copy and creates a verified offline draft without touching the original | **PASS emulator-only** [41194840094](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41194840094); not a hardware HOME Menu proof |
| D. **Exact game build match** | CIA title/ROM fingerprint verified; matching GodMode9 128KiB save independently read-only validated across 28/28 sector checksums, dual bank counters, independent XOR keys, money/bag fields, decrypted Pokémon and 104-byte party stride. Full species/item/move semantics and ROM-specific hub/PC mapping still unverified | **PARTIAL: structural READ-ONLY mapping verified; actual-game write safety NOT verified** |
| E. Disposable game-save restore | Two minimal privately tested in-memory edits and exact byte-perfect reversals passed structural and independent bank/Pokémon validation. A private emulator kit was built from user-supplied ROM and save, but **gameplay boot, in-game re-save and disposable title restoration/rollback are NOT tested** | **PARTIAL offline only: NO EMULATOR-GAME OR HARDWARE RESTORE PROOF** |
| F. Physical device lifecycle | Phase 11 CIA direct live H168 GBA VC reader was physically confirmed (money 556800/counter 150 vs older SD 556799/149). Phase 12 build adds two-confirmation READ-ONLY **live-to-SD backup** with double live snapshot, unique file and independent readback, five CI tests passed [41219627930](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41219627930). Physical live backup, exit/recovery, authenticated CMAC restore and full editor still need testing | **PARTIAL: live READ proven on device; LIVE BACKUP awaiting device; direct WRITE blocked** |
| G. Release approval | All above passing, checksummed release, installation and rollback guide, verified no personal saves/ROMs in artifacts | **NOT APPROVED** |

**Phase 11 live GBA VC reader (read-only):** New `phase11/` native bridge triggers only with Y and uses Luma PxiFS0 read-only access to the exact installed H168 GBA VC title; title-ID, slot, and dual-bank validation are required. **No native direct writes/restores or CMAC handling are enabled.** Hardware compatibility of this reader remains unverified; Checkpoint's separate confirmed restore path requires title-specific CMAC signing. See `phase11/README.md`.

## Allowed and forbidden work now

Safe research work is limited to **synthetic or disposable copied SD save files**, and does not allow the native editor to write to live VC save, delete PKSM extdata, or restore into the user's installed title. The exact-user-ROM gate must remain fail-closed until positive evidence is available. The toy synthetic save is intentionally **not playable**.

To advance gates D/E safely, obtain the game build identity and a disposable corresponding save **privately**. Do not publish either a copyrighted ROM or personal save to this public GitHub repository. A build label or screenshot alone is not sufficient evidence that offset mappings are safe. Keep PR #2 draft until gates D–G are met.

## Source of truth

- [PROJECT_HANDOFF.md](PROJECT_HANDOFF.md) — chronological verified achievements.
- [Phase 9 implementation](phase9/) — extdata-free toy-lab UI, range validation, backup/export and tests.
- [GitHub Actions](.github/workflows/phase9-readonly-diagnostic.yml) — native builds, Azahar guest operations and CIA assembly/install tests.
- [Synthetic host regression](.github/workflows/phase9-sandbox-tests.yml) — negative and sanitized tests.
