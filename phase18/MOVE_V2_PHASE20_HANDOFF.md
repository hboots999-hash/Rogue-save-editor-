# Phase18 move_v2 + move_queue_v4 — Phase20 integration handoff

**Status:** fully implemented, independently testable **offline replacement core**, but release **Apply is hard-DISABLED** because exact-ROM runtime PP modes, PP-Up replacement semantics, move legality and game load/re-save still need evidence. PR #14 remains draft. No Phase20 or Phase18 v3 source changes.

## Pinned build identity

- User-provided GodMode9 CIA was inspected locally; GBA ROM SHA-256:
  `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`.
- Indexed move name/PP geometry: 936 rows, ROM name table `0x4C283C` stride 13; PP table A `0xBD27D0`, table B `0xBD8E30`, both stride 28, byte +5.
- A *canonical metadata index* consists of **936 × 15 bytes**: [13 encoded name bytes, basePP A, basePP B] for IDs 0..935. Its SHA-256 is
  `3d2d63e5fd77371a76540d07a6c5a4d20a9ddae7acea9f03035d86936ee34fa4`.
  The 14,040-byte metadata index is **not committed** (nor are the CIA or ROM). `move_v2::authenticate` verifies this complete SHA and the pinned ROM identity before lookup.
- Exactly **807 ordinary IDs** exclude Struggle, specials, unrecognized strings, PP=0, and the **40 IDs with disagreement** between PP variants. These are **search-only candidates**. No species-move learnability/eligibility is established, hence **none are authorized for Apply**.

### Binary PP-table selection investigation

The GBA ROM includes direct pointers to **both** tables in many compiled Thumb routines. In two inspected callsites (`0x080159AA` and `0x08015F94`) the code branches based on a call to `0x081D11B8`, which in this exact image consists of `movs r0,#0; bx lr`. For example, the branch at `0x080159B0` takes table **A** when the predicate returns zero and only selects **B** if nonzero. The other inspected caller at `0x08015F94` similarly selects A on the zero return.

**This establishes table A for those call paths in this binary. It does NOT establish that all callers / gameplay modes always use A**: both full tables have many pointers, and other predicates or contexts may reach B. No mode-selection assumption is put in the API, and conflicting 40 entries stay blocked. More static control-flow coverage or an actual mode-by-mode emulator trace is required.

### PP-Up replacement semantics

Public EX-v2.1 `src/pokemon.c` uses four 2-bit PP-Up fields (one per move slot) and
`maxPP=basePP+floor(basePP*20*PPUps/100)` (PP Ups 0..3).
The synthetic engine applies a deliberately conservative research policy:
**clear the replaced slot's two PP-Up bits and reset its current PP to the new base PP**.
This is not proven to be the installed build's exact replacement routine:
public `SetMonMoveSlot` does not itself clear old PP Ups. The actual installed
binary routine and in-game UI behaviour need to be verified before Apply is
authorized. The unchecked PP variant cannot be inferred from a move name.

## Local-only, authenticated catalogue generation

```sh
python3 phase18/tools/extract_2_1_2_move_catalog.py \
  "/local/path/Pokémon Emerald Rogue v2.1.2.cia" \
  --out "/local/private/moves.csv" \
  --packed "/local/private/exact_rom_move_metadata.bin"
```

The extractor fails closed unless the entire GBA ROM SHA matches the above
fingerprint. It also requires the exact 14,040-byte metadata SHA before
writing a packed file. **Never upload or commit the CIA, ROM, generated packed
catalogue or private save.** The generated CSV is informational only.

## Versioned APIs (Phase20: read-only navigation, no Apply)

```cpp
#include "phase18/move_queue_v4.hpp"

// loadedMeta points to the locally generated 14040-byte index.
// It remains immutable during every transaction.
rogue18::move_v2::Catalog db{
    loadedMeta,
    rogue18::move_v2::PackedBytes,
    rogue18::move_v2::RomSha
};
if (!rogue18::move_v2::authenticate(db)) return;
// Note: db.romSha is a metadata claim, NOT a hardware ROM hash attestation.

// Read-only 10-row or 12-row pagination:
rogue18::move_v2::Move page[12]{};
const size_t shown = rogue18::move_v2::page(db, 0, page, 12);

// Read-only selected party/slot:
const auto view=rogue18::v2::inspect(importedSaveBytes,131072);
rogue18::move_v2::Request selected{};
if (!rogue18::move_v2::begin(
        view,importedSaveBytes,131072,partySlot,moveSlot,selected))
    return;
if (!rogue18::move_v2::choose(db,selected,desiredMoveId)) return;

// Compile-time disabled until new reviewed version:
const auto proposed=rogue18::move_v2::preview(
    importedSaveBytes,131072,db,selected);
// proposed.ok is ALWAYS FALSE; verify() is ALWAYS FALSE.

// Staging for eventual combined queue, not export:
rogue18::move_queue_v4::Queue pending{};
if (!rogue18::move_queue_v4::begin(view,pending)) return;
rogue18::move_queue_v4::appendScalar(
    view,pending,rogue18::Kind::Money,0,0,125000);
rogue18::move_queue_v4::appendReplacement(
    view,importedSaveBytes,131072,db,pending,
    partySlot,moveSlot,desiredMoveId);
const auto batch=rogue18::move_queue_v4::preview(
    importedSaveBytes,131072,pending);
// If there is ANY staged replacement batch.ok is ALWAYS FALSE.
// A v3-only scalar batch still uses the unmodified v3::preview().
```

`move_queue_v4` rejects replacements that conflict with a v3 PP-only edit
of the same party and move slot. It rejects duplicate replacements to a slot,
stale source snapshots, invalid IDs and missing authenticated metadata.
It deliberately **does not apply a staged replacement** and has no SD writer,
game archive, restore adapter or CMAC pathway. There is no silent "enable"
parameter, runtime override or caller-supplied verified-ID list.

## Pure offline core implementation

`move_v2::detail::construct` implements raw 128KiB H168 re-encryption for
a selected existing move slot: replacement u16 move ID, new PP, target 2-bit
PP-Up clearing, Pokémon checksum and active section checksum. It verifies
original whole-copy SHA256, active counter, PID, OT, species, per-Pokémon
checksum and expected move ID/PP/PP-Ups; rejects a replacement duplicating
another active move. Every decrypted payload byte outside exactly selected
move-ID/PP and the target PP-Up byte is preserved. A **whole-file whitelist**
permits at most the two move ID bytes, one PP byte, one G PP-Up byte, two
Pokémon checksum bytes and two sector checksum bytes. It reparses both
banks and all visible invariant Pokémon fields. The public `preview` and
`verify` never call this core until a separately approved version gate.

Tests in `test_move_replacement_v2.cpp` use **fabricated IDs** and
`ROGUE18_TEST_MOVE_V2`, and cover 24 permutations × six party slots × four
move positions = **576 generated replacements**, wrong IDs/no-ops, duplicate
moves, source tampering, all 28 corrupt sectors, PP-Up/move/checksum guard
changes and forbidden field preservation. A separate Phase9 Python decoder
reads the generated unplayable edited save, verifying 28 sector checksums
and the new move ID/current PP. Native ARM11 compilation instantiates the
actual engine and v4 queue under a hard 4096B per-frame ceiling.

## Remaining release blockers

1. Independently prove exact installed title's ROM fingerprint (not just
   the supplied CIA filename) with read-only hardware verification.
2. Complete full **mode-specific** ROM PP-table selection trace: for all
   new moves, identify which table drives initial/current PP and PP Ups.
   Never guess from the presence of two tables; 40 conflicts remain blocked.
3. Verify the actual 2.1.2 learn/forget move routine in compiled code and
   confirm **PP Ups are reset or preserved** in the expected game behaviour.
   Test on an isolated disposable imported copy in emulator, NOT installed
   save; check species legality, forbidden specials, cached battle stats.
4. Integrate a separately reviewed **new gated contract** in Phase20 native
   menus: explicit user confirmation, immutable backup, exclusive standalone
   draft, independent readback/proof and low-memory safeguards. Never adapt
   v3 `Kind::Move` from PP-only into move ID replacement.
5. Independently validate a new edited draft boots and re-saves correctly.
   Keep all game archive writes, CMAC re-signing, Phase14/17 restore
   authorization and actual GBA VC injection disabled.

**No permissions here unlock new moves. PR #14 remains draft/unmerged.**
