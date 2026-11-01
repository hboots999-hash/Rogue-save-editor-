# Phase 18 -> Phase 20 verified / experimental / blocked feature matrix

**Scope:** offline 131,072-byte H168 raw-copy candidates only; no PxiFS, no live save
writes, no GBA VC container restoration, no ROM or private save committed.
**Assessment:** 1 November 2026. **PR #8 stays draft.**

Terminology:
- **Observed H168 structure**: historical independent read-only examination of one
  private game save documented in `phase9/profiles/`. No private bytes in CI.
- **Source-derived**: facts from Pokabbie's **EX-v2.1 public source**. It has not
  been shown to be identical to the private **v2.1.2 GBA-P-H168** embedded ROM.
- **Host-tested**: deterministic **non-playable synthetic saves** with corrupt,
  rotated, rolled-over, checksum-repaired, encrypted and stale variants.
- **ROM-verified**: game build-specific data and gameplay verified with that
  precise matching ROM. **No Phase18 feature attains this level.**

| Feature | Source/observed basis | Host implementation and tests | ROM-specific standing | Phase 19/20 policy |
| --- | --- | --- | --- | --- |
| 128 KiB copied-save sector authentication | Historical 28/28 sector checks; 14 sectors × 2, correct footer/key | Checks both banks; rotated sectors; adjacent counters including uint32 rollover; rejects bad IDs, signature, counter gaps and corruption | Structural only | Inspect copied raw save only |
| Money 0..999999 | Observed H168 section 1 +0x4A8; public source field-size calculation independently gives 0x4A8 | RAM edit, u32 XOR, active footer only; old value/counter/SHA guards; exact rollback | No altered-game-load proof | Offline preview and experimental draft |
| Existing bag-stack quantity 1..999 | Observed H168 section 1 +0x578, u16 XOR section-0 key; EX source 999 stack ceiling | Slot/ID/old-value checks, checksum, byte footprint and rollback tests | Custom ID/pocket rules not fully confirmed | Offline preview and experimental draft; never invent IDs |
| Party encrypted header/structure | Observed 104-byte stride; EX-v2.1 source 80-byte BoxPokemon +20-byte status/party stats +4-byte Rogue extension | 24 shuffle permutations, growth 11-bit species and split held item, shiny/gender/Gmax preservation, checksum verification | Exact private ROM flags unconfirmed | Numeric, read-only summary; no insertion/species edit |
| IVs 0..31 each | EX Misc +4 thirty IV bits | All 6 fields; preserves egg/Gmax bits; checks every modified byte, checksum/rollback | Party derived battle stats not recalculated offline; game-load untested | Experimental offline RAM candidate only |
| EVs 0..252 each, total <=510 | EX Effort six u8 | All 6 fields; rejects oversized totals; protects other Pokémon bytes | Party derived stats may remain stale until game recalculates | Experimental offline RAM candidate only |
| Existing move slot PP | EX Attacks moves and PP at 12/20 canonical offsets | Guarded 4-slot selection; rejects empty slots/invalid PP >64; exact checksum/rollback | Move-specific maximum PP cannot be confirmed from generic range | Experimental offline, review in-game semantics before restore |
| Replacement move ID | Public EX numeric constants only | Requires separately validated `Evidence.verifiedMoveIds`; no defaults | Exact installed ROM ID table not verified | **Blocked by default** |
| Ability selection | EX Misc+8 bits29..30 gives 3 slot indices; Misc+4 bit31 is **Gigantamax** | Corrected slot bitfield; rejects index 3; preserves Gigantamax | Species/revision ability IDs unverified | **Blocked by default**, even though RAM field logic exists |
| Nature selection | EX header byte+18 stores 5-bit modifier, language low 3 bits | Corrected PID%25 XOR modifier, preserves PID and Pokémon checksum; tests reverse edit | Real ROM semantics/stat recalculation unverified | **Blocked by default** |
| Bag insertion | EX physical 450 slots; 50 key-item reserved; 80 base +10 per upgrade, max37 | Append-only, external complete pocket map, verified upgrade count must match source candidate byte+0x4AD, rejects disorder/duplicates | Pocket classification and actual upgrade field not verified on H168 ROM | **Blocked by default**; no mid-pocket shifts |
| Box PC Pokémon | EX 14×30×80 records over save sectors 5–13; 10 normally usable, 4 leftover | Read-only section-stream candidate, checksum, corruption/empty/partial-slot tests | No independently verified populated v2.1.2 PC dump | **Read-only research candidate; no editor** |
| Phase19 API | `editor_model.hpp` v1 | `SaveView`, `beginRequest`, `beginMoveRequest`; SHA-256 snapshot pin protects against same-counter stale copies; capabilities default disabled for unsupported features | UI/in-game save editing untested | Read-only screen integration only |
| Offline SD export | Phase10 copied-import storage helper | Never overwrites source; unique immutable backup; verified separate draft; stale checks | No real 3DS SD durability test for Phase18 | Experimental offline copies only |
| Live GBA VC write / restore | Separate Phases 16–17 safety work, **not Phase18** | Not implemented or linked | Explicitly out of scope | **Disabled** |

## Source-offset discrepancy — resolved at C struct geometry level

Upstream `include/global.h` comments for money `0x490` and bag `0x560`
are based on the old 100-byte party record. The same EX source asserts
`sizeof(Pokemon)==104`. Six party records from `0x238` therefore finish at
`0x238 + 6*104 == 0x4A8`, the observed money position. Money (4 bytes),
bag sort mode/upgrade/reserved bytes (4 bytes), and 50 four-byte PC item
records give bag offset `0x4A8 + 8 + 50*4 == 0x578`. Code asserts
these values in `layout_geometry.hpp`. **This reconciles the two documented
money/bag positions; it does not establish that every private H168 2.1.2
field, or the predicted upgrade byte at `0x4AD`, is correct.**

## Remaining Phase20 gates (no physical console required yet)

1. Locate a publicly verifiable exact v2.1.2 source snapshot, or obtain
   independently auditable metadata extracted from the precise ROM while
   keeping the copyrighted bytes private. Never equate EX-v2.1's release
   label with the specific patched 2.1.2 build.
2. Independently verify bag-upgrade byte, item pocket-to-ID mapping, move
   identifiers/PP rules, ability and nature semantics, and PC records before
   lifting default blocks.
3. Use disposable emulator runs and game-side re-save/reload to check whether
   IV/EV/nature edits update cached party stats and preserve run/hub progress.
4. Retain byte-perfect rollback and immutable untouched backups throughout.
   The existing Phase16/17 recovery test results do not replace save-format
   verification.
5. Advance Phase19's read-only integration independently; **do not merge this
   draft PR or enable live restoration** merely because synthetic CI passes.

**Test commands:** `.github/workflows/phase18-offline-editors.yml` compiles strict
C++17 and ASan/UBSan tests, cross-checks the independent Phase9 Python
inspector, and compiles headers for native ARM11 with devkitPro. No actual
ROM, private save, or hardware is needed for this test matrix.
