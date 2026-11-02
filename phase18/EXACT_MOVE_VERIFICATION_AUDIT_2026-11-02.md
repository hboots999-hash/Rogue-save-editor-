# Phase18 PR #14 — exact-ROM move verification audit (2 Nov 2026)

**Status: CONTINUED RESEARCH. Apply/export for move replacement remains DISABLED.**
No ROM/CIA bytes, full binary tables, private save, authenticated VC container,
or generated replacement were committed. No installed-game writes or restore.
All evidence below derives from the **locally supplied** 32MiB GBA ROM whose
whole-image SHA256 is `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`.
Static findings do not prove reachable behaviour in every actual game mode.

## A. Reproducible callsite inventory: table A and B

The previous report of *two* checked callsites was not an exhaustive binary
inventory. The new read-only, SHA-pinned
[`tools/audit_h168_pp_selectors.py`](tools/audit_h168_pp_selectors.py)
does a raw static sweep up to the first PP table and verifies that the
selector at GBA address `0x081D11B8` is exactly **`movs r0,#0; bx lr`**.
Observed measurements for this specific image:

| Evidence | Number / location |
|---|---|
| Literal references to table A `0x08BD27D0` | **303** word-aligned matches |
| Literal references to table B `0x08BD8E30` | **308** word-aligned matches |
| A literals with B literals within 16 bytes | **241** |
| Raw candidate Thumb-1 BL opcodes targeting `0x081D11B8` | **757** |
| Those followed by `cmp r0,#0` and a conditional branch | **599** |
| Constant-zero selector instruction bytes | `00 20 70 47` |

**Interpretation:** where a reachable caller truly uses that selector to
choose between the two tables, it will take its **zero-return** branch.
For example, exact machine code at `0x080159AA` and `0x08015F94`
uses this selector, and a verified maximum-PP routine at `0x08191634`
selects table **A** on the zero-return path. However, a memory literal and
a nearby BL do **not** prove they are connected; raw BL patterns can include
non-code. The 303/308 pointers have **not** all been assigned to reachable
functions, nor has their mode-specific predicate been proved. There is
therefore **no global "A always active" assumption** in the editor. All
**40 base-PP-conflicting moves remain disallowed** for edit, regardless
of apparent selection on the inspected paths.

### To reproduce *locally* without extracting/committing game bytes

```sh
python3 phase18/tools/audit_h168_pp_selectors.py \
  "/local/Pokémon Emerald Rogue v2.1.2.cia" --cia \
  --json /local/private/pp_selector_audit.json
```

The script extracts the game in RAM only, rejects a ROM that fails the
complete SHA256 check, and writes **metadata-only JSON**. GitHub CI tests
the Thumb decoder on verified small instruction bytes and rejects forged
synthetic ROM images, but the proprietary/personal full ROM is **not**
available in CI and therefore the original complete-byte audit is a
separate **local** verification, not a green GitHub integration assertion.

## B. Exact-ROM PP-Up formula, masks and helper call references

The following 12-byte ROM sequence is unique and was independently read
at ROM offset `0x010A9984`:

- `03 0C 30 C0`: four selected-slot PP-Up masks (2 bits/slot).
- `FC F3 CF 3F`: four PP-Up clearing masks.
- `01 04 10 40`: four PP-Up increment values.

Independent literal references at `0x0019167C` (get mask) and
`0x001916B4` (clear mask) link the actual compiled functions to these
bytes. Disassembling Thumb routines shows:

- **`0x08191634`: maximum PP calculation.** Loads base PP at record +5
  from the active selected battle-move table, reads the 2-bit counter for
  a chosen move slot, and computes
  `basePP + floor(basePP * ppUps / 5)`. With 0..3 PP Ups that is
  `basePP + floor(basePP * 20 * ppUps / 100)`. The inspected code path
  selects PP table A when the selector stub returns zero.
- **`0x08191684`: party-Pokémon PP-Up clear helper.** Masks the selected
  lane of Pokémon's full PP bonus byte and writes the result back via
  Pokémon data setters. It does not prove *which learn/forget flows call it*.
- **`0x081916B8`: battle-data bonus clear helper.** Applies the same
  four clearing masks to the battle Pokémon's PP bonus byte.
- **`0x081806D4`: separate move-swap/reordering routine.** Reads two
  move slots' IDs, current PP and the Pokémon's combined bonus byte;
  swaps both slots and transposes their two 2-bit PP-Up counters.
  This is **not** equivalent to teaching a new move and does not dictate
  replacement reset semantics.

Static Thumb BL pattern inventory shows **14** callsites to the maximum
PP helper, **9** to the party clear helper and **2** to the battle clear
helper. These are references, not verified executed reachability. We
still have not mapped the exact direct replacement/learn/forget routine
completely enough to prove whether it calls the clear helper, resets
current PP, preserves old PP Ups, or shifts bonuses when removing a move.
The earlier research policy **clear replaced slot's PP Ups, refill new
move to base PP** remains **TEST ONLY** and may differ from gameplay.

## C. Additional offline editor/queue hardening performed

- `phase18/move_replacement_v2.hpp`: `detail::construct`, including
  actual encrypted move-ID/PP/checksum mutation code, is now compiled
  **only** with `ROGUE18_TEST_MOVE_V2`. No production build exposes it;
  production `preview`/`verify` still always reject. An immutable
  `releaseBlockers` enum lists PP mode, exact replacement behaviour,
  species legality, cached battle state, emulator reload and installed
  ROM identity.
- `phase18/move_queue_v4.hpp`: a queued move replacement now conflicts
  with **all** IV, EV and PP edits to the same party Pokémon and with any
  second queued replacement to that Pokémon. Such edits could otherwise
  invalidate the original Pokémon checksum before a future atomic
  composer rebases each operation. Separate money/item edits and
  different Pokémon can still be staged. `verifyStaging` rechecks
  original source fingerprint, version, all scalar guards, duplicate
  ownership, Pokémon identity and approved metadata (but it **does not**
  apply replacement and is not an export permission).
- New strict host *production-without-test-macro* regression ensures the
  public `preview`/`verify` and queued replacement preview fail
  closed. ARM11 builds now compile both test-only and production-disabled
  paths with 4096-byte frame ceiling and reject privileged archive symbols.
- The pre-existing synthetic edit suite still verifies **576 combinations**
  (24 encryption permutations × 6 party slots × 4 move slots), stale
  SHA256, 28 corrupt sectors, exact 8-byte footprint, invariants and an
  independent Phase9 Python check of the emitted synthetic save.

## D. Status for each requested release blocker

| Gate | Current status | Needed before any export/Apply |
|---|---|---|
| All PP table uses / modes | **Partially traced** | Thumb control-flow review of all relevant reachable table consumers; mode-by-mode runtime trace; do not conflate byte-pattern counts with executed calls |
| Exact learn/forget routine | **Not fully mapped** | Identify compiled `SetMonMoveSlot`, direct forget/relearn and PP-Up helper callsites and check whether saved PP and bonus bits are reset |
| PP-Up layout / max PP | **Compiled formula and lane masks observed** | Prove that replacement uses those helpers and mode table; test with 0,1,2,3 PP Ups |
| Learnability / invalid moves | **Unproved** | Exact species/form/ability handling, ordinary move legality and disabled special moves; 807 browse candidates are NOT an Apply allowlist |
| Cached battle/session state | **Unproved** | Inspect exact-binary battle party-copy, temporary battle mon PP/moves, quest/rogue state and safe load/re-save on a *disposable* imported copy |
| Offline encrypted bytes | **Synthetic tests passed** | Independent end-to-end edited draft/gameplay read-back with exact matching build |
| Real CIA installed identity | **Dump fingerprint verified, installation unverified** | Read-only independent confirmation currently installed game still matches the GodMode9 dump |
| Phase20 combined editor | **Staging and conflict checks implemented, Apply blocked** | New reviewed atomic copy-only contract, immutable backup/draft, readback and 3DS memory tests; never retask v3 PP-only `Kind::Move` |
| Real export & restoration | **Disabled** | No enablement here; installed-game writes remain forbidden |

**Decision:** insufficient evidence for a release-enabled offline move
replacement draft. PR #14 remains **DRAFT**; no Phase20 modifications,
no merge, no real save mutation, no installed-title write or restore.

## E. Exact Phase20 handoff

1. Pin PR #14's exact HEAD rather than `main` or a moving branch.
2. Retain the SHA-authenticated read-only `move_v2::Catalog` and the
   no-heap 12-row `page`/`search` UI. Disable `Apply`; if useful,
   display `releaseBlockers` in developer information.
3. Use `move_queue_v4::appendScalar` and `appendReplacement` to
   represent future intent; now all same-Pokémon IV/EV/PP/move ownership
   conflicts fail. Use `verifyStaging` for read-only validation only.
4. Never define `ROGUE18_TEST_MOVE_V2` in a native user-facing CIA:
   it exposes research construction against synthetic/disposable fixtures.
   Native production `move_v2::preview` is hard-disabled and
   `move_queue_v4::preview` refuses ANY queued replacement.
5. Do not wire v3 `Kind::Move` into new ID changes. It only adjusts
   **existing move current PP**.
6. Once all remaining evidence is obtained, request an **explicit new
   reviewed contract/version**, with independent multi-edit byte-proof
   and **copy-only** immutable export; never silently flip a boolean.

Existing v2 and v4 source headers are fully documented in
[`MOVE_V2_PHASE20_HANDOFF.md`](MOVE_V2_PHASE20_HANDOFF.md).
