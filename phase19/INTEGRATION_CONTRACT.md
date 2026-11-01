# Phase 19 integration contract — Phase 18 PR #8 review

Status: **READ-ONLY DTO AND MOCK DRIVER IMPLEMENTED. REAL SAVE ADAPTER NOT LINKED.**

## Interface boundary

phase19/adapter.hpp provides ReadOnlyProvider (summary, partyCount, partyAt, bagCount, itemAt, pcBoxCount, pcBoxSize, pcAt), Availability (Ready/Empty/Unsupported/Invalid/Unverified), a bounded snapshot (up to 6 party, 450 existing bag stacks (6 per visible page), 14 boxes x 30 records), capture() and mockSnapshot(). The native app only creates mockSnapshot(). It never requests a live save or calls the Phase 18 exporter.

A future independently audited provider may call capture() only on the result of Phase 18's strict H168 dual-bank validation, from an independently verified **read-only** copied input. capture() checks DTO counts, money ≤999999, item quantities 1–999, IVs 0–31, EVs 0–252 and sum ≤510, PP ≤64, binary ability slot and nature index ≤24. It fails closed when provider data is Invalid, Empty, Unsupported or Unverified. This DTO-level screening is **not** independent verification of the underlying ROM/save.

Captured snapshots always have synthetic=false: State::canPreview() rejects all edits, including money and item proposals. No method in ReadOnlyProvider or State can stage a real edit, export a draft, request a title ID, access SD or restore data. Real edits require an entirely separate audited capability design and new release-gate approvals.

## Mapping table for a future approved Phase 18 read-only provider

| Phase19 display | Phase18 source | State and restrictions |
| --- | --- | --- |
| Summary money / counter | h168::Info.money; active bank counter from rogue18::validate | Read-only when validation passes; no direct live GBA VC service in this build |
| Party count | h168::Info.partyCount, maximum 6 | Validated party only; cannot infer PC from this |
| Pokémon species | rogue18::Mon.species from rogue18::partyAt | Use numeric IDs unless exact ROM name lookup verified |
| Pokémon level/name | Not provided by rogue18::Mon | Leave 0/unknown or implement independent verified mapping; never substitute synthetic nicknames |
| IV / EV | Mon.ivs / Mon.evs | View-only for actual saves; bounds and 510 EV sum verified |
| Move IDs + PP | Mon.moves / Mon.pp | Move ID substitution requires Evidence.verifiedMoveIds; PP editing is not enabled by this UI |
| Ability | Mon.abilitySlot (1 bit) | This is **not** a general ability ID; ability edits require verifiedAbilitySlot, not enabled |
| Nature | Mon.natureFromPid, PID % 25 | Display only; Phase18 rejects mutation |
| Egg state | Mon.egg | Do not expose editing for eggs |
| Bag existing item ID/quantity | h168::item() under active bank | Per-stack quantity view; no item insertion without separate pocket/ID verification |
| PC contents | No validated Phase18 PC parser | Availability::Unsupported, zero selectable real PC entries |
| Backup status | Separate Phase16 hardware evidence | Availability::Unverified until a separately verified inventory provider exists |
| Restoration | Separate authenticated restoration phase | UI blocks; no restoration calls or CMAC signing |

## Controls and unsupported operations

Up/Down selects entries, Left/Right changes proposal by ±10, Up/Down proposal ±1, X resets, A opens/reviews, B cancels, START exits, L/R cycles PC boxes or edit groups. PC boxes and bag stacks are paged six entries per view; L/R switches PC boxes or pages the bag. The native executable shows no real save contents.

IVs, EVs, unchanged/read-only move IDs, PP, money and existing bag quantity each show a synthetic proposal where supported. The final A confirmation **always reports BLOCKED** and leaves the snapshot unchanged. New move IDs, ability slot mutation, PID-derived nature mutation, PC editing, bag item insertion and all actual real-data edits remain unsupported. No-op proposals show an invalid message rather than succeeding.

## Integration and release gates

- Confirm the exact H168 ROM and validated read-only copy. The Phase18 callback must first run rogue18::validate and respect the active counter, Pokémon identity and checksums.
- Establish exact move/item metadata, PC layout and ability mapping before adding any write capability. Unknowns must report Unsupported.
- Any eventual edit request must capture the old party ID/PID/OT/checksum/old field values and counter, then pass an independent full candidate verification and backup/stale-source/recovery review.
- Phase16 backup/journal evidence does not enable restoration in Phase19. Authenticated GBA VC CMAC signing, rollback and physical gameplay/persistence proof remain separate approvals.
- Do not alter Phase18 or Phase14–17 branches, merge or expose a live writer.

## Test evidence accounting

Host tests use equivalent D-pad, A/B/X/L/R/START key events and mock/invalid provider cases. devkitPro 3DSX/ELF, experimental CIA and isolated Azahar smoke use the Phase19 workflow. A passing emulator smoke test does not prove a game-load or real handheld navigation. Independent Nintendo 3DS manual button testing is still outstanding.
