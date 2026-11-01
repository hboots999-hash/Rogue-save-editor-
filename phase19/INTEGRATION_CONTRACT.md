# Phase 19 integration contract — current READ-ONLY interface

**Status:** Active copied-SD reader wired to a **pinned** Phase18 H168 decoder. Host and Azahar work are experimental, and this newer import-enabled build has **not** been validated on the user's physical Nintendo 3DS. PR #7 remains DRAFT.

**Canonical future contract:** [PHASE20_INTEGRATION_CONTRACT.md](PHASE20_INTEGRATION_CONTRACT.md). This summary describes the implementation, not authorisation to write game saves.

## Current reader implementation

- The default 3DSX launch is fictional sample data. The **seventh** home-menu option, “Inspect SD copy (read-only),” explicitly reads only `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav` using `fopen(...,"rb")`.
- Only an **independently prepared, unwrapped 131072-byte H168 copied** file is accepted. The 263168-byte GBA VC authenticated backup container is **not** parsed here, and must never be truncated or substituted for the installed game's save.
- `inspectReadOnlyStream(FILE*, Demo&, reason)` checks exact file size, complete read, EOF/read errors; `inspectCopiedBuffer(raw,len,Demo&,reason)` calls `rogue18::validate` and populates bounded `ReadOnlyProvider` values. Failed inspections leave the existing visible snapshot unchanged.
- Phase18 source pinned in GitHub CI at commit **596743d8eaa073759a736b5a8107db43f3a8c17b**, checked out separately; PR #8 is not merged or changed. Structural validity is not proof of ROM identity or game-playability.

## Available validated structure fields

| Domain | Source | UI |
| --- | --- | --- |
| Money / active bank | `h168::Info.money`; `banks[active].counter` | Read only, 0–999999 |
| Party 0–6 | `rogue18::partyAt`, `rogue18::Mon` | Numeric species, IVs, EVs, move IDs, PP, ability **slot**, effective nature, egg |
| Pokémon names / levels | Not exposed by current pinned `Mon` | Unknown, shown as Species ID and Lv? |
| IV/EV | `Mon.ivs` / `Mon.evs` | IV 0–31; EV 0–252 and total ≤510 |
| PP/moves | `Mon.pp` / `Mon.moves` | Numeric only; no verified ROM move-ID name mapping |
| Ability / nature | `Mon.abilitySlot`, `Mon.effectiveNature` | Ability **2-bit index 0–3**, not Gmax or an ability ID; effective nature 0–24 |
| Existing bag 0–450 | `h168::item`, `Info.items` | ID and quantity 1–999; no insertion or pocket assumptions |
| PC storage | No independently proven Phase18 layout | **Unsupported**, never fictitiously populated for imported copies |
| Backup/restoration | Not wired to this UI | **Unverified/disabled** |

A **legitimate empty party, empty bag, or both** is accepted and displayed as EMPTY. Invalid, unsupported, corrupt, missing, wrong-sized and wrapped inputs fail closed.

## UI trust boundary and controls

`ReadOnlyProvider` offers only `summary, partyCount, partyAt, bagCount, itemAt, pcBoxCount, pcBoxSize, pcAt` and explicit `Availability` status. `captureInto` populates an external fixed-size `Demo` snapshot. `State` holds a **const reference**, not a stack-allocated copy (mandatory `sizeof(State)<256`). `State.demoPreviewEnabled` is **an independent session flag**; `switchView(state,false)` permanently blocks proposals for an imported view until the explicit fictional demo reset. `populateMock` overwrites the entire previous snapshot before restoring mock fixture records.

- R / D-pad Right moves to next PC box, edit category or bag page.
- X / D-pad Left reverses without needing the broken L shoulder button.
- D-pad Up/Down changes selection; A opens; B cancels/returns.
- In **fictional proposal** mode only, Up/Down changes ±1, Left/Right ±10, X resets; two confirmations lead to **BLOCKED**, never a saved change.
- START **alone on release** exits. START+SELECT is reserved for **Rosalina**, tested in either key order. L is optional.

Every real-data Pokémon, money or item interaction is **read-only**. PC edits, new move IDs, ability/nature modifications, item insertion, draft exporting, live title archive access, CMAC and save restoration are **disabled**.

## Tests, unknowns and Phase20

Strict host and sanitizer tests cover all mock screens and proposal cancellations, read-only imported-data guards, synthetic/broken-L alternatives, Rosalina key chord, typed status/empty data, exact read-only stream canary, corrupt historical sectors, 450-stack paging, console line wrapping, and no oversized `State`. Native devkitPro build audits the ELF for project-specific writer functions; Azahar runs isolated boot/interaction attempts and unchanged SD canary, **not** physical 3DS verification.

For Phase20, require the separately authored [contract](PHASE20_INTEGRATION_CONTRACT.md): actual ROM identity and correct move/item/ability mappings, independently established PC parser, reliable backup/recovery and authenticated container, verified immutable source fingerprint and exact byte-diff evidence, plus renewed physical hardware acceptance before **any** edit/restore capability is considered. Do not edit Phase17 or Phase18 branches or merge this draft without those gates.
