# Phase18: experimental party move replacement — Phase20 handoff

**Scope:** Pure-memory research on an exact 131072-byte copied H168 save. **NOT approved for Pokémon Emerald Rogue 2.1.2 gameplay, SD draft export, GBA VC authentication, restoration, or installed-game writes.**
**Branch:** `feature/phase18-move-replacement-research`; stacked on Phase18 PR #8 at `3625e160a17f805821098cf35a47a0f96aba8e16`. The existing v2/v3 interfaces are unchanged.

## What public EX-v2.1 research establishes (and what it does not)

Official public source tag: [Pokabbie/pokeemerald-rogue EX-v2.1](https://github.com/Pokabbie/pokeemerald-rogue/tree/EX-v2.1), published source for a *different version* of the user's privately packaged 2.1.2 game:

- `include/constants/moves.h` (Git blob `485a9fb71cc981c9ae98745294c26a60d3f9d864`) defines enumerated numeric move IDs; e.g. public EX-v2.1 `MOVE_POUND=1`, `MOVE_TACKLE=33`. **These numeric values are NOT authorized for the installed ROM.**
- `src/data/battle_moves.h` (blob `10a48540d382b35a1a55f9952cf1cf1df266d362`) defines `gBattleMoves_Mainline` and the base `.pp` property for each published move, including variants controlled by build macros.
- `src/data/text/move_names.h` (blob `0b851168708367e49885d7b735625eff551eca3f`) supplies public move names; language and build-variant checks are needed.
- `include/pokemon.h` (blob `e5222649c04a52865f3bbf86e485ed86c1bb7102`) shows `PokemonSubstruct0.ppBonuses` at byte +8 and `PokemonSubstruct1.moves[4]` / `.pp[4]`.
- `src/pokemon.c` (blob `5d0fcbd6adc05dd694507efe25569860351a56f6`) defines a single byte of PP bonuses: each move uses a **two-bit counter 0–3**, shifted by two bits per slot. `CalculatePPWithBonus` returns `basePP + floor(basePP * 20 * PPUpCount / 100)`. `SetMonMoveSlot` initializes new PP from `gBattleMoves[move].pp` without itself clearing old PP Ups; the research engine deliberately takes the conservative alternate policy **clear only the replaced slot's PP Ups, set PP to the new proven basePP**. No proof yet that this policy matches the target game after loading.
- The H168 structural format already verified synthetically in Phase18 stores the encrypted A substructure as four u16 IDs then four u8 PP, and the encrypted G substructure contains the bonus byte. The Pokémon payload checksum and its active sector checksum must both be recalculated.

**Update 2026-11-01:** a GodMode9 CIA copy supplied by the user was inspected locally and its 32 MiB GBA ROM SHA-256 **is now known**: `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`. Its fixed-width indexed move-name table and two 936-row compiled battle-move tables have been located and independently SHA-256 hashed; 40 ordinary moves have conflicting PP across the two ROM-resident variants. The complete metadata and offsets are in [EXACT_ROM_2_1_2_METADATA_HANDOFF.md](EXACT_ROM_2_1_2_METADATA_HANDOFF.md). The variant-selection path, learnability rules, exact PP-Up reset semantics and in-game load/re-save validation are **NOT yet proved**. An arbitrary caller-provided `verifiedMoveIds` allowlist or a plausible EX-v2.1 name is not a ROM attestation. Matching a logo, title ID, or save game code is insufficient.

## Actual 2.1.2 ROM metadata, without game upload

A safe offline extractor is now available at `phase18/tools/extract_2_1_2_move_catalog.py`. It requires the **exact full-ROM SHA** above and exports only a local `move_id,name,pp_variant_a,pp_variant_b,pp_agreement,research_ui_candidate_only` CSV, never a ROM copy or a save. It is tested with synthetic CIA and name records; the source CIA/ROM bytes and local output CSV are **not committed**. A total of 807 ordinary IDs (excluding Struggle) are potential *read-only* UI catalogue candidates because both PP variants agree. **40 additional ordinary IDs disagree** and are blocked pending mode selection. This evidence is not permission to replace moves.

## Versioned production-safe interface

New header: `phase18/move_replacement_v1.hpp`.
All stable public entrypoints are in `rogue18::move_v1`.

```cpp
#include "phase18/move_replacement_v1.hpp"
auto source = rogue18::v2::inspect(copiedBytes, 131072);
rogue18::move_v1::Request req{};
bool selected = rogue18::move_v1::begin(
    source, copiedBytes, 131072, partySlot, moveSlot, req);
// partySlot 0..5, moveSlot 0..3, must be occupied non-egg Pokémon
// req.replacementMoveId would be filled from an EXACT-ROM-certified catalog.
// A SHA-bound local research catalogue now exists, but NO production setter.
auto proposal = rogue18::move_v1::preview(copiedBytes, 131072, req);
// proposal.ok ALWAYS FALSE until a reviewed, fingerprint-pinned future API.
static_assert(!rogue18::move_v1::ExactRomMoveTableApproved);
```

`capability()` reports `mayPreviewReplacement=false`, `mayExportReplacement=false` and `mayWriteInstalledGame=false`. `verify()` always returns false and `preview()` always rejects. **Do not expose a selectable move-replacement menu in Phase20 yet; present “Move database verified; PP variant and gameplay safety pending” as a disabled capability.** Even after the table has been verified, move replacement must remain an offline copied-save operation, with all live restoration gates separate and OFF.

The request pins the entire original copied save SHA-256, active bank counter, party slot, move slot, original PID/OT/species/payload checksum, original move ID/current PP and original **full PP-Up bonuses byte**. This prevents stale requests and a later slot swap masquerading as the originally selected Pokémon.

## Synthetic research engine — not for release

The **only** implementation which changes a move is behind `ROGUE18_SYNTHETIC_MOVE_TEST`:

```cpp
// Tests only, NEVER set this macro in a release build:
rogue18::move_v1::ResearchMove synthetic[]={{501,5,"Synthetic Ember"}};
req.replacementMoveId=501;
auto testCandidate=rogue18::move_v1::previewResearch(
    copiedBytes,131072,req,synthetic,1);
bool exact=rogue18::move_v1::verifyResearch(
    copiedBytes,131072,testCandidate.candidate.data(),
    testCandidate.candidate.size(),req,synthetic,1);
```

Test-only move IDs `501–504` and synthetic PP are *fabricated fixtures*, **not** approved real-game move definitions. Research accepts only a nonempty, unique synthetic table with bounded ID/PP/name and refuses unchanged IDs, empty slots, duplicate currently held moves, malformed identities, corrupt banks and source digest changes. It rebuilds encrypted move ID/PP and one G byte, recalculates the Pokémon checksum and sector checksum, re-parses both banks, checks IVs/EVs/species/OT/PID/ability/nature, checks all three other move IDs and PP, clears only target slot's two PP-Up bits, and restricts exact changed bytes to:
- 2 encrypted move ID bytes in A;
- 1 encrypted PP byte in A;
- 1 encrypted PP-Up byte in G (only its target 2-bit lane can change);
- 2 Pokémon checksum bytes;
- 2 active sector checksum bytes.

The **entire 131072-byte candidate** is independently regenerated and byte-compared for `verifyResearch`. All other bytes, inactive bank, save-tail, fixed 104-byte party extension and party attributes remain unchanged. No SD writer/exporter is reachable from this research header. Research is compiled on ARM11 only for static compiler/stack proof and is **never executed on actual hardware**.

## Phase20 future searchable move list

Implement only after independent exact-ROM admission. Suggested future read-only `MoveCatalogV1` should embed/build a fingerprinted index `{uint16_t id, uint8_t basePP, name, type, flags}` extracted from that exact ROM, with a deterministic digest and version; reject unknown, undefined, invalid, disabled, G-Max, Z-specific or dummy move IDs until build variants are verified.

A future native 3DS UI can provide a list of 10–12 entries per page, alphabet and category filters and a case-insensitive ASCII substring search (no heap-heavy fuzzy matching). The *synthetic-only* `searchResearch` function implements a memory-bounded 12-result, stable-order substring/page reference, **not a live move list**. Search display does not authorize applying any move. An independent safety gate must verify the exact ROM and selectability/learnability before enabling the Apply action.

## Further release gates

1. Obtain independently validated **installed** ROM build identity / SHA-256 through a safe read-only process, never committing or uploading private ROMs. Cross-check build-dependent `moves.h`, `battle_moves.h` and `move_names.h` or extract the exact binary's move table.
2. Verify replacement PP-Up policy and move compatibility on a copied synthetic or deliberately disposable save without writing an installed game; determine whether cached party battle structs depend on move IDs/PP.
3. Introduce a **new reviewed version** of `move_v1` with an immutable, approved exact-ROM manifest and test-approved source; do not simply flip a boolean or accept a caller-supplied evidence claim.
4. If integrating with v3 batching, add a **new batch contract** that individually verifies each move replacement against the *current* Pokémon checksum and incorporates the correct G+8 PP bonus byte, not the existing v3 `Kind::Move` PP-only scalar. Until then, **v3 move replacement is unsupported**.
5. Re-run strict C++17, ASan/UBSan, ARM11 stack and new Phase20 native/UI CI; validate fresh SD-only backup/draft failure and restart semantics separately, then independently test target gameplay and re-save. Installed-game write/restore remains forbidden.

**PR is DRAFT.** No Phase20 edits, merges, privileged archive APIs or personal save/ROM uploads.
