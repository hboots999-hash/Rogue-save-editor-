# Phase 20 integration contract — Phase19 UI ↔ Phase18 H168 parser/editor

Status: **integration design, NOT an implementation authorisation**. Phase19 is an opt-in read-only copied-save viewer. Phase18 has an experimental pure-memory edit engine on separate draft PR #8; Phase17's physical RAM-only recovery tests are not a license to restore or write live game saves. PR #7 stays DRAFT.

## 1. Verified current read-only boundary

Phase19 source: `phase19/read_only_import.hpp`; `phase19/adapter.hpp`, `phase19/ui.hpp`.

Read-only APIs pinned from Phase18 commit `ea7303e3d04e9c409febd44ee3e1f834de02cdc6`:
- `rogue18::validate(const uint8_t*, size_t, h168::Info&)`: structural checks on **exact 131072-byte unwrapped H168** copy. Both Gen III-style historical banks must pass sector checksums and contain adjacent counters. Checks conservative bag and party layout.
- `rogue18::partyAt(const uint8_t*, size_t, const h168::Info&, unsigned, rogue18::Mon&)`: bounded decoded party. Includes numeric species, IVs 0–31, EVs <=252 (sum <=510), numeric moves and PP, EX-v2.1 two-bit ability **slot**, effective nature and egg flag.
- `h168::item(const uint8_t*, const h168::Info&, unsigned, unsigned&, unsigned&)`: existing item numeric ID and quantity. No verified pocket or display-name mapping.
- `h168::Info`: `money`, `items`, `partyCount`, active bank index and counter.
- `phase19::Phase18ReadView` implements `phase19::ReadOnlyProvider` (summary, party count, party slot, bag count, bag slot, PC count/slot/box-size). All PC access currently **Unsupported**.
- `phase19::captureInto(provider, Demo&)` copies bounded data and status; UI uses a **non-owning const Demo&**. `phase19::inspectCopiedBuffer(raw, n, destination, reason)` commits a validated snapshot atomically; on failures previous visible data is unchanged.
- `phase19::inspectFixedSDCopy` opens **only** `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav` with `fopen(...,"rb")` after explicit user input. No live-game archive, title or restore services.

A structural H168 validation does **not** authenticate ROM identity, legal game semantics or game-playability. Unsupported party nickname/level are shown as numeric species with unknown level; no fabricated item or move names. Unknown save types (notably wrapped 263168-byte authenticated GBA VC containers), missing/corrupt files and invalid counters are **rejected**; never trim or rewrite the container.

## 2. Read-only data contract for future Phase20

`ReadOnlyProvider` methods return `Availability::{Ready, Empty, Unsupported, Invalid, Unverified}`. Empty is a legitimate result and not a fatal parse error. Numeric item, move, nature and species identifiers are **not** sufficient semantic proof to enable writing. Data are immutable in the UI; `State` maintains a separate `demoPreviewEnabled` gate from the snapshot's synthetic flag. Imported sessions clear this gate and reset all outstanding demo proposals.

Array bounds:
- party <=6
- PC boxes <=14 x 30 (currently unsupported by real parser)
- bag <=450 existing stacks (insertion not enabled)
- money <=999999
- IV <=31, EV <=252 and sum <=510, PP <=64
- ability is an EX-v2.1 two-bit slot 0–3, **not** an ability move/name ID or the Gmax flag
- nature is a validated effective index 0–24, not simply `PID%25`

If any member violates validation or coverage limits, report `Invalid` / `Unsupported` and display no editable value. Party/PC and bag selection indices must be checked **again** after each data source switch. A failed import cannot activate an imported session or change the previous snapshot. The native State must remain less than 256 bytes; use static or heap storage for the large display object.

## 3. Future editing interface: NOT linked to Phase19

Future Phase20, only after external verification, may introduce an **entirely separate** command interface:
- `ReadOnlySnapshotMetadata`: immutable source SHA-256 digest, source length, verified ROM build identity, bank counters, sector checksums, provenance (explicit copied import), versioned schema.
- `EditIntent`: explicit slot and numeric field, original value, proposed value, independent dataset/ROM mapping evidence, original party PID/OT/species/checksum where relevant.
- `PrepareResult`: either BLOCKED/UNVERIFIED with human-readable reason, or a **memory-only** candidate buffer and exact changed-byte allowlist; no file writer attached.
- `VerifyResult`: independently checks source freshness/fingerprint, unchanged inactive bank, sector checksums, field-level guards, double-confirmation review and round-trip parse before *any* storage action is considered.

Separate UI page and distinct evidence-gated command handler would be required. Imported read-only screens must **never** call `rogue18::propose`, `h168` draft-export helpers, SD write APIs or live save functions. Do not use an existing bool `synthetic` or `Availability::Ready` as write permission.

## 4. Release gates and acceptance

Blockers before editing can even be considered:
1. Independently verified Emerald Rogue 2.1.2 ROM/build identity and actual item/move/ability mappings.
2. Verified PC storage layout before any PC read, edit or movement.
3. Real 3DS **read-only copied-save** compatibility and source-size/path verification after current UI changes. Emulator success is not equivalent.
4. Independent original backup, recoverability, checksum and Phase17 recovery-journal integrity plus explicit user review of edit effects.
5. Memory-safe source-digest checks, no modification to originals and validated round-trip of drafts against exact format; no reliance on assumed code layouts.
6. Separate audited and user-approved restoration/injection implementation (not in Phase19 or this contract).
7. Host strict-warning and ASan/UBSan, native devkitPro 3DSX/ELF, Azahar startup/navigation/safe-exit as automated evidence, then console acceptance.

## 5. Input and usability contract

**Broken L:** essential reverse navigation uses X or D-pad LEFT; forward via R or D-pad RIGHT; D-pad UP/DOWN selects rows; A opens; B backs/cancels. Proposals use their own LEFT/RIGHT to alter amount and X to reset (the mode is explicit). Input hints must match actual behaviour on each screen.

**Rosalina shortcut START+SELECT:** application exit is triggered by **standalone START release**, never on the initial START keydown and never if SELECT was held/pressed during the same START press. Do not bind SELECT alone to a destructive action. Pressing a Rosalina chord in either order should not exit the app.

PR #7 must remain a separate unmerged draft until physical-read-only verification. Phase17/18 branches must not be modified while implementing this interface.

## QA #10 integration boundary clarification

On the 3DS **do not include** Phase18 `editor_model.hpp` or `pc_readonly.hpp` inside Phase19's native implementation: the research-only PC inspector has a 37,104-byte frame and unsafe unresolved ROM semantics. Phase19's native `Phase18ReadView` consumes only audited `rogue18::validate`, `rogue18::partyAt`, `h168::item` and `rogue18::sourceDigest` at pin `ea7303e3d04e9c409febd44ee3e1f834de02cdc6`. The host integration build independently compares party, bag, money, counters, SHA and metadata against `rogue18::SaveView` API v1; CI fails if the versioned API changes. In Phase20 **do not directly co-include Phase17 and Phase18's duplicate `h168` declarations**: use isolated translation units with a versioned read-only DTO bridge. No writer/restore bridge, source fingerprint alone or caller-provided evidence flag authorizes operations.
