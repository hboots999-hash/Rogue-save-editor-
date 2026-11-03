# Independent EX ROM compatibility audit — Phase 21 handoff

3 November 2026. Research and tests only. No editor, CIA, save-safety module,
installed-title writer or other developer's branch is changed by this PR.

**Decision for tonight's CIA:** retain the existing 43-ID experimental copied-save
move draft restrictions. Keep nature and ability export blocked. Do not enable the
40 PP-conflicting moves, broaden the allowlist, or claim gameplay compatibility
from this audit. Installed-game writing remains prohibited.

## Evidence boundaries

| Evidence class | What was actually examined | What it establishes |
|---|---|---|
| **Independently verified public source** | Pokabbie `EX-v2.1`, pinned to [`559792325f500d480b892799218ee015962f93a2`](https://github.com/Pokabbie/pokeemerald-rogue/commit/559792325f500d480b892799218ee015962f93a2); 14 public blobs identity-checked | C-level layout declarations, selector logic, mint recalc invocation, loader copying, ability fallback; host tests execute upstream function bodies |
| **Prior exact-2.1.2 observations, not independently rerun here** | PR #14 at [`df76d02bf85f90779fc550384c531898bd04e3c6`](https://github.com/hboots999-hash/Rogue-save-editor-/pull/14), its committed ROM metadata/disassembly reports | Reports identify the target ROM and measured table regions/helper addresses. They are references, not fresh ROM verification in this PR |
| **Exact-ROM checks supplied, not executed against a real ROM** | `check_exact_rom.py` | A local owner can recheck full-image identity, three region hashes, PP conflict count and the prior selector-stub measurement; passing is not proof of all control flow |
| **Unverified** | Exact compiled mint/ability accessors, effective species/form/mode/OT ability mappings, full PP consumer tracing, gameplay load/re-save | No new feature enablement or production evidence flag may be justified from these tests |

**No ROM, CIA, private save, extracted catalogue, or proprietary species/ability
table is present in this workspace or this commit.** No exact-ROM-positive result
is claimed. The public source ref does not prove binary equivalence to 2.1.2.
Issue [#15](https://github.com/hboots999-hash/Rogue-save-editor-/issues/15),
PR [#14](https://github.com/hboots999-hash/Rogue-save-editor-/pull/14),
PR [#16](https://github.com/hboots999-hash/Rogue-save-editor-/pull/16), their
handoffs/discussion, and research issue
[#17](https://github.com/hboots999-hash/Rogue-save-editor-/issues/17) were consulted.
PR #16's reviewed baseline is `880d5839a62f70e074dc53195d0e3cc524aaf665`.
This independent branch starts at `main` `6730002d0b11ed30e4698f37eabc8b527e351d17`.

## Move tables and PP selection

1. The pinned public source has **936 named initializers in each table and 40 PP
   disagreements**, verified by preprocessing the actual tables and comparing
   `.pp` fields. Mainline Ice Beam has PP 10; Revised has PP 15. The Revised file
   itself defines `ROGUE_DRAYANO`. This is a source comparison, not a binary match.
2. `include/pokemon.h` defines `gBattleMoves` as the result of
   `Rogue_GetRevisionModeActive() ? gBattleMoves_Revised : gBattleMoves_Mainline`.
   `CalculatePPWithBonus` reads that selected table and the chosen two-bit lane;
   max PP is `base + floor(base * ups / 5)` for ups 0..3.
3. **Build condition matters:** `include/constants/rogue.h:25` comments out
   `ROGUE_FEATURE_REVISED_MODE`. The inspected Makefile does not define it.
   With that feature absent, `Rogue_GetRevisionModeActive` returns FALSE even
   if saved configuration or the run cache requests Revised. Two compiled data
   tables alone therefore do not establish that both are selectable at runtime.
4. When compiled with the feature enabled, outside a run the setting selects
   Revised only for ALWAYS_ON. During a run the selector uses cached
   `gRogueRun.revisedModeEnabled`, initialized with `GetRevisionModeActive_Slow(TRUE)`
   before setting the active-run flag. NEVER=0, IN_RUN=1, ALWAYS_ON=2 in source.
   The current config is not reread during the active-run fast path.

| Feature compiled | Run active | Configuration / cache | Source result |
|---|---|---|---|
| No | Either | Any | Mainline |
| Yes | No | NEVER or IN_RUN | Mainline |
| Yes | No | ALWAYS_ON | Revised |
| Yes | Yes | Cached flag false / true | Mainline / Revised |

**Prior exact-ROM measurements:** target raw ROM SHA-256
`111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`, 32 MiB.

| Region | ROM offset (GBA CPU address) | Geometry |
|---|---|---|
| Move names | `0x004C283C` (`0x084C283C`) | 936 × 13 |
| Battle data A | `0x00BD27D0` (`0x08BD27D0`) | 936 × 28, PP at +5 |
| Battle data B | `0x00BD8E30` (`0x08BD8E30`) | 936 × 28, PP at +5 |

PR #14 reports a constant-false Thumb stub at CPU `0x081D11B8` and A selected on
the inspected max-PP helper path at `0x08191634`. A=10/B=15 for Ice Beam supports
**A=Mainline, B=Revised as an inference**. The disabled public feature explains
why the stub might be constant false; neither observation binds the exact source
symbol names to all ROM consumers. **Do not assert all contexts always use A.**
Literal/BL pattern counts are not a control-flow proof. The exact mode-setting
save offsets and all reachable direct accesses remain unverified here.

Do not silently widen the existing 43-ID experimental draft to the 807
name/PP-agreement candidates. Agreement avoids this PP ambiguity, but does not
certify species learnability, PP-Up learn/forget semantics or battle cache handling.
Public `SetMonMoveSlot` sets ID and base PP; it does not itself clear PP-Ups.
Calling code and exact ROM behaviour still matter. The existing draft's reset
policy is experimental, not a universally established gameplay rule.

## Nature Mints and loaded stats

The public BoxPokemon header shares byte +18 between `language:3` and
`hiddenNatureModifier:5`. Getter and setter cases use
`effectiveNature = (PID % 25) XOR modifier` and
`modifier = (PID % 25) XOR requestedNature`.
For valid requested nature 0..24, modifier may be 0..31. The candidate byte
formula is `(oldByte & 7) | (((PID % 25) ^ requestedNature) << 3)`.
Preserve language and PID/OT; a whole-byte modifier write corrupts language.
Malformed decoded natures 25..31 must not be treated as valid stat-table indices.
No save mutator is added here.

**Recalculation is an explicit operation in source.** `Task_NatureMint` calls
`SetNature`, which calls `SetNatureBoxMon` and `CalculateMonStats`.
`CalculateMonStats` uses `GetNature`, effective species stats and stored IVs/EVs,
and updates the party-stat cache. It also has HP/Shedinja behaviour; a generic
stock-Gen3 stat writer is not a substitute. `SetNatureBoxMon` alone does not
recalculate party stats. `BoxMonToMon` does recalculate when converting a boxed
mon, a separate path from party loading.

**Public `LoadPlayerParty` copies each entire stored Pokémon, including cached
stats; it performs no recalculation.** The probe seeds attack=777..782 and
executes that exact loader: all six stale values survive and the recalc observer
is never invoked. The loader is called by `CopyPartyAndObjectsFromSave`; public
`LoadGameSave` calls that before Rogue save hooks. Those hooks and later scripts
are not an exhaustively traced guarantee of repair. This establishes the loader's
behaviour, not that no later game event ever recalculates a Pokémon.

**Block nature export:** the exact compiled +18 packing/accessors and full
load/battle/re-save policy are unverified. The harness observes the recalc call;
it deliberately stubs arithmetic and does not certify correct game stats.
The same cached-stat concern applies to raw IV/EV edits; their existing offline
support must not be described as proof of gameplay stats recalculation.

## Species abilities, Hidden Abilities and Revised Mode

The public expansion path is:

`species/form ID + slot + OT + current player trainer ID + mode`
→ `Rogue_GetPokemonBaseStatsFor`
→ optional custom gift override
→ slot lookup and fallback.

- `PokemonSubstruct3.abilityNum` occupies bits 29..30 of its **MISC+8 word**
  (canonical decrypted payload +44), mask `0x60000000`; it is not in the IV word.
  Slot 3 is representable but not a valid selectable ability slot.
  Preserve bit 31, tutor/shiny/gender/ribbon bits, and all other fields.
- `gRogueSpeciesInfo` aliases `gSpeciesInfo` for expansion builds.
  `SpeciesInfo.abilities` is **u16[3]** at declared +0x18. Do not use stock
  Gen3 u8 ability IDs, stock species IDs, or a guessed SpeciesInfo stride.
- Revised copies `gRoguePokemonProfiles_Revised[species].baseStats` first.
  Three **independent group sentinels** then govern fallback: `baseHP==0`
  restores all six stats; `types[0]==TYPE_NONE` restores both types;
  `abilities[0]==ABILITY_NONE` restores the **entire** ability array.
  A nonzero first revised ability with empty slot 1 does not merge slot 1 from
  Mainline: the later ability resolver handles the empty slot by fallback.
- `GetAbilityBySpecies_ForRevised` applies a gift override only when
  `IsOtherTrainer(otId)`, custom ID is nonzero and custom ability count is nonzero.
  `IsOtherTrainer` compares OT against the loaded save's player trainer ID.
  Species/mode/OT alone omit this current-player dependency.
- The requested slot is used when nonempty. An empty hidden-slot request first
  searches hidden slots, then any nonempty ability from slot 0 onward. Thus slot
  2 may resolve to slot 0's ability. Even invalid slot 3 can produce a fallback
  display result: a successful lookup is not evidence that slot 3 is legal.
- The inspected gift code includes dynamic OT-encoded custom data, egg-species
  matching, and a Mega exception. Dynamic custom abilities can expose only slot
  0 while reporting a nonzero count, so a Hidden Ability request can fall back.

**Block ability export:** no independently authenticated exact-ROM species table
addresses/strides, Revised profile table, gift decoding or complete effective
species/form mappings are available here. There is no real-species allowlist in
this PR. The resolver tests use fabricated ability IDs and synthetic species rows.
Ability names must remain unknown or explicitly source-only until authenticated;
do not label a raw slot index as a verified Hidden Ability.

## Phase 20/21 acceptance requirements

| Feature | Experimental offline status recommended by this audit | Evidence still needed |
|---|---|---|
| Existing 43-ID single occupied move draft | Retain current restrictions/warnings and source/catalogue/identity proofs; no new enablement | Exact learning/forgetting policy, learnability and copied-save gameplay load/re-save |
| Other 764 agreeing candidates / 40 conflicting PP entries | Keep disabled for replacement | Explicit reviewed scope; for conflicts, all relevant exact-ROM PP branches and mode identity |
| Mint nature change | Display/research only; export blocked | Exact accessors/packing, mode-aware stat/cache policy including HP, independent edited-copy load/re-save |
| Ability / Hidden Ability change | Display/research only; export blocked | Exact species/form/table/mode/player-OT/gift mapping and cache/gameplay verification |
| Installed-save write/restore/CMAC | Prohibited | Outside this audit; no permission or integration supplied |

Reuse existing Phase18 `offline_engine.hpp` guarded mutators and
`integration_v2.hpp` eligibility gates. Do not flip `verifiedNatureModifier` or
`verifiedAbilitySlot` using this source probe, duplicate Jules's implementation,
or weaken stale-source/catalogue, checksum, byte-whitelist or backup protections.

## Reproduce and interpret the tests

```sh
python3 research/rom_compatibility/fetch_public_source.py /tmp/rogue-source
python3 -m unittest discover -s research/rom_compatibility -p test_audit_tools.py -v
python3 research/rom_compatibility/test_public_source.py /tmp/rogue-source
python3 research/rom_compatibility/test_public_source.py /tmp/rogue-source --sanitize
# Optional, LOCAL ONLY; requires the exact owner's legally held raw GBA dump:
python3 research/rom_compatibility/check_exact_rom.py /private/rogue.gba
```

`source_manifest.json` pins the public repository commit and each raw Git blob
identity. The fetcher fails on an altered cache rather than trusting local edits.
Tests extract actual upstream function bodies and bitfield declarations into a
temporary C11 harness. Both feature builds exercise 5,000 nature cases, 216
ability fallback cases, 16 config/run/cache states, 32,768 PP assertions, revised
group fallback, gift gating, recalc invocation, loader preservation and bit masks.
Only fabricated rows/dependencies are supplied; no editor or encrypted-save
writer is implemented. Host ABI measurements are not an ARM ABI proof.

Local result: strict host probe and ASan/UBSan passed; four audit-tool tests passed.
This sandbox requires `ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer cannot
run under its tracer. CI requests leak detection on its normal runner. No native
GBA/3DS execution, emulator gameplay, physical hardware or exact-ROM-positive
test was performed. CI validates **public source semantics and infrastructure**,
not ROM possession or gameplay compatibility.

## Pinned references

All public source links below use the inspected commit, not a moving branch.

- [Feature default](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/include/constants/rogue.h#L22-L25), [mode constants](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/include/constants/rogue.h#L618-L621), [selector](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/rogue_controller.c#L565-L594), [run cache](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/rogue_controller.c#L4681-L4692).
- [Table macro](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/include/pokemon.h#L538-L555), [Mainline Ice Beam](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/data/battle_moves.h#L1016-L1033), [Revised Ice Beam](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/data/battle_moves_revised.h#L1077-L1098), [PP helper](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/pokemon.c#L3391-L3407).
- [Header/MISC declarations](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/include/pokemon.h#L146-L254), [nature getter](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/pokemon.c#L2637-L2642), [setter](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/pokemon.c#L2959-L2966), [mint action](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/party_menu.c#L5733-L5795), [SetNature](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/pokemon.c#L4145-L4165), [stats/BoxMonToMon](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/pokemon.c#L1538-L1624), [party loader](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/load_save.c#L175-L183).
- [Base-stat/profile types](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/include/rogue.h#L655-L678), [revised fallback](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/rogue_baked.c#L1983-L2027), [ability resolver](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/pokemon.c#L3160-L3203), [trainer identity](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/pokemon.c#L5581-L5600), [gift identity](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/rogue_gifts.c#L573-L603), [gift abilities](https://github.com/Pokabbie/pokeemerald-rogue/blob/559792325f500d480b892799218ee015962f93a2/src/rogue_gifts.c#L657-L697).
- Prior exact-ROM reports: [table geometry/hashes](https://github.com/hboots999-hash/Rogue-save-editor-/blob/df76d02bf85f90779fc550384c531898bd04e3c6/phase18/EXACT_ROM_2_1_2_METADATA_HANDOFF.md), [compiled PP audit](https://github.com/hboots999-hash/Rogue-save-editor-/blob/df76d02bf85f90779fc550384c531898bd04e3c6/phase18/EXACT_MOVE_VERIFICATION_AUDIT_2026-11-02.md), [latest 43-ID experimental policy](https://github.com/hboots999-hash/Rogue-save-editor-/blob/df76d02bf85f90779fc550384c531898bd04e3c6/phase18/MOVE_V3_FIRST_CIA_HANDOFF.md). Earlier reports' disabled-v2 status is historical; the last link records the later experimental-v3 allowance.
