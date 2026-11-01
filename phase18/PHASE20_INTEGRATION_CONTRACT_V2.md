# Phase 20 integration contract — Phase18 API v2 (QA PR #10)

**Status:** draft integration contract. No actual game write. No restoration adapter.
**Branch:** `feature/phase18-offline-editors` / draft PR #8.
**QA sources:** independent draft PR #10, issues QA-03, QA-04, QA-05,
QA-08 (duplicated H168 headers), QA-09 (Phase13/14 narrow preflight).

## 1. Strict ownership and version boundaries

- `phase18/raw_v1.hpp`: `rogue18::raw_v1` contains a **private scoped**
  snapshot of the Phase10 raw-copy inspection/sector helpers. The original
  `::h168` namespace is **not declared** by Phase18 anymore; Phase17's
  legacy `::h168` can coexist without redefinition. Raw v1 is an
  implementation detail, NOT a combined read/write protocol.
- `phase18/raw_v1_sd.hpp`: opt-in SD-only copied-import, immutable backup and
  draft file helpers. The pure-memory reader does not include SD write APIs.
- `phase18/integration_v2.hpp`: `rogue18::v2`, `ContractVersion=2`, is the
  **only supported new Phase20 reader/request/preview/verify entrypoint**.
  No global `h168`, Phase17 header, container type or restoration symbol
  leaks into this contract.
- `editor_model.hpp` remains a backwards-compatible **internal model v1**
  for Phase18 host tests; it is not the new external namespace v2 contract.
  In a future agreed shared library the raw helpers should be consolidated
  at one canonical path rather than copy-maintained across branches.
- A pinned Phase17 checkout at
  `132ef904c453dd5e793f9069fd55b5ac045bdced` is compiled in the **same
  translation unit** as Phase18 v2 in `test_unified_v2.cpp`; the host builds
  it three times with strict and ASan/UBSan diagnostics, and CI also compiles
  it for ARM11. This is a tested header boundary, not a combined executable
  CIA/hardware integration proof.

### Important Phase19 migration requirement (QA-01/QA-08)

The existing Phase19 `read_only_import.hpp` from PR #7 directly references
global `::h168::Info`, `::h168::item`, and the old
`rogue18::validate` interface. That **legacy source must not be declared
compatible with the Phase18 v2 branch without a Phase19-side update and CI
re-pin**. This PR deliberately does not change Phase19. Phase20 may read
via v2 `ReadSnapshot`, or coordinate a separate Phase19 adapter migration.
Do NOT copy its old header into a single Phase17/18 translation unit and
assume it will build without edits.

## 2. Entry points Phase20 may call

```cpp
#include "phase18/integration_v2.hpp"
using namespace rogue18;

auto snapshot=v2::inspect(copiedBytes, length);  // exactly 131072 bytes
if (!snapshot.data.valid) {
    // Show unsupported/error read-only state.
} else {
    // Display money, party IVs/EVs and numeric moves/PP, numeric bag IDs.
    // PC is deliberately not inspected in v2.
    Request req{};
    if (v2::beginMove(snapshot, partyIndex, moveIndex, req)) {
        req.pp = reviewedNewPP;
        auto proposal=v2::preview(copiedBytes, length, req);
        if(proposal.ok &&
           v2::verify(copiedBytes,length,proposal.candidate.data(),
                      proposal.candidate.size(),req)){
            // Show RAM-only candidate for review. No game archive access.
        }
    }
}
```

- `v2::inspect(raw,len)`: read-only structural validation, money, party,
  numeric IDs and quantities, active raw-bank counter, whole-copy SHA-256.
  Any failure is a blocked view. Explicit `pcStatus` is unsupported by
  the default v2 call, not a verified box.
- `v2::capability(Kind)`, `v2::evidenceText(Kind)`: stable operation
  declaration. `canCreateOfflineDraft` is true **only** for money,
  existing stack quantities, IVs, EVs, and existing move/PP edits.
  `canRestoreInstalledGame` is ALWAYS false.
- `v2::begin(snapshot, Kind, slot, req)`: read-only optimistic locks for
  guarded money/quantity/IV/EV edits. `v2::beginMove(snapshot, party, 0..3,
  req)` populates the actual selected move/PP guards.
- `v2::preview(raw,len,req)`: requires a nonzero SHA-256 digest from a
  read-only snapshot. Refuses changed-move IDs, ability slots, natures and
  insertions even when a caller has populated research Evidence elsewhere.
  It makes a RAM vector, re-encodes section/mon checksums and revalidates
  a byte-specific change footprint.
- `v2::verify(before,n,after,afterSize,req)`: reproduces the exact approved
  RAM candidate and checks ALL 131072 bytes (both banks and the tail).
- `exportCopied` (separate `offline_export.hpp`): fixed user-imported
  SD path only. It invokes v2 preview and verify; it cannot accept an
  Evidence allowlist or write an installed title. Creates a unique
  read-verified immutable backup and a **separate** offline draft.
- **Never call `rogue18::propose` directly from Phase19/20.** That older
  low-level research function accepts synthetic experimental `Evidence`
  controls to test unverified bitfields and is intentionally not a
  production/export authorization point.

## 3. QA-09 preflight is a separate policy domain

Phase13/14's `rogue_restore::Operation` enumerates exactly:
`MoneyPlusOne` and `ItemQuantityPlusOne`. Other changes fail its
preflight, even if Phase18 can construct a syntactically sound RAM candidate.
`v2::legacyPreflightShape(req)` is a **descriptive classification ONLY**:
it does not call Phase13/14, cannot sign any CMAC, does not construct
263168-byte full containers, and does not authorize staging or restoration.
`v2::mayRestoreInstalledGame(req)` is always false, including +1 requests.

Raw H168 inner-bank counters may wrap modulo 2^32 under the strict parser.
The Phase14 *outer wrapper* counter policy separately rejects rollover
and nonadjacent counters, as hardened on Phase17 PR #9. Never transfer
counter policy or approval between raw and authenticated wrapper formats.
The missing raw-to-wrapper patch/CMAC/restore bridge is **intentionally
not provided**; Phase16/17 RAM-only/recovery evidence does not grant it.

## 4. Required evidence per operation

| Operation | v2 offline RAM preview and SD draft | Required input/evidence | Installed restore |
| --- | --- | --- | --- |
| Money | Enabled | Valid dual-bank raw copy, SHA-256 pin, expected money/counter; 0..999999 | Forbidden |
| Existing item quantity | Enabled | Occupied slot, exact old item ID and quantity, SHA-256; 1..999 | Forbidden |
| Party IVs | Enabled | Existing non-egg mon, PID/OT/species/checksum, SHA-256, 6×0..31 | Forbidden |
| Party EVs | Enabled | Same mon lock, 6×0..252, sum<=510; cached battle stats unresolved | Forbidden |
| Existing move/PP | Enabled for **same ID only** | Nonempty move slot 0..3, old move ID and PP, SHA-256, PP<=64 | Forbidden |
| Replacement move ID | **Blocked** | Matching installed-ROM ID and maximum PP map absent | Forbidden |
| Ability slot | **Blocked** | Matching installed-ROM species/ability semantics absent | Forbidden |
| Nature | **Blocked** | Matching installed-ROM effective nature/stat recalculation absent | Forbidden |
| Insert item | **Blocked** | Exact H168 pocket map/upgrade byte/custom ID provenance absent | Forbidden |
| PC Pokémon editing | **Blocked** | Populated exact-ROM PC layout/slot data unverified | Forbidden |
| Live GBA VC write | **Not present** | Phase20 release gates unsatisfied | Forbidden |

## 5. Memory and corruption boundaries

- Exact input size: 131072 bytes, never a 263168-byte authenticated container.
  Every Phase18 proposal and export requires a full valid dual-bank layout.
- Cryptography: whole-source SHA-256 locks the *entire* raw copy; per-Pokémon
  48-byte XOR/checksum and H168 section checksums are independently checked.
  A checksum is not ROM identity, cryptographic authenticity or write authority.
- `pc_readonly.hpp` has been moved from a ~36.8 KiB **stack** buffer to
  explicit heap-backed scratch storage for optional research-only inspection.
  Core proposal candidates and imported backup copies use heap allocations,
  not enormous stack frames; peak 3DS heap/resource stress remains a
  separate actual-device test before enabling anything beyond read-only UI.
- Invalid slot index, empty move, corrupt old bank, malformed/equal/gapped
  counters, SHA-256 stale inputs, invalid/empty length, unsupported IDs,
  oversized EV/IV/PP/money/quantities and changed unrelated tail bytes fail
  closed in synthetic regressions. Save-counter rollover is allowed only
  for the **inner raw** domain.
- No supplied synthetic .sav files or copyrighted ROM bytes are committed.
  Host strict, sanitizer and ARM11 tests are compile/fixture evidence only.

## 6. Phase20 decisions

**Ready to wire as read-only/experimental copied-save interface:** v2
inspection, capability display, SHA-256-locked request creation, offline
RAM candidate preview/whole-buffer verification, and separately named
SD-only draft export.

**Not integration-ready:** reusing older Phase19 global-h168 header without
an adapter migration; passing an offline candidate to Phase17 restoration;
verified exact-ROM move IDs, nature/ability mapping, item insertion,
real populated PC parsing, recalculated battle stats, and hardware
save persistence. Keep PR #8 draft and do not merge Phase17/19.

**Source note:** Public Pokabbie EX-v2.1 (80-byte BoxPokemon, 104-byte party)
explains the observed H168 money/bag offsets by correcting stale comments.
It does NOT identify the exact private 2.1.2 patch or verify its ROM metadata.
See `SOURCE_FINDINGS.md` and `PHASE20_FEATURE_MATRIX.md`.
