# Phase 21 — Real Emerald Rogue save-to-game safety validation
Updated: 21 November 2026

## Goal
Move from *structurally valid SD-only save drafts* to independently proven
gameplay-safe, recoverable editing on the **owner's Emerald Rogue 2.1.2
GBA-P-H168 ROM**. This phase is a release gate, not a request to bypass it.

**Do not commit the owner's original ROM, CIA, save, emulator screenshots
containing private trainer information, or modified save.** Keep these owner
files in the offline local lab.

## Evidence completed privately on owner's exact files
- The uploaded CIA contains an embedded 32MiB GBA ROM at CIA offset 17664,
  fingerprint SHA256 `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`.
- Original raw save is precisely 131072 bytes. Both banks independently pass
  all 14 sector footers, party checksums, and bag range/continuity validation;
  adjacent counters 156, 157 select active bank 1.
- Constructed a **private, exclusive, independent +1-money draft**:
  observed 572400 => 572401 in raw active bank. 128KiB structure verified,
  only **two actual bytes differ** (low byte of encrypted active money at
  `0x124A8` and sector1 checksum at `0x12FF6`); inactive bank and all
  other bytes match; exact original backup/reverse is verified. Corrupt-footer
  input fails validation. Original upload was never overwritten.
- Independently fetched official mGBA 0.10.5 Linux binary through public
  GitHub-only bootstrap workflow, never sending any ROM/save to GitHub.
- Ran the actual exact ROM and two separately staged save copies in locally
  isolated mGBA instances. **Both original and +1 draft** display the real
  Continue screen with the same trainer/quest information, and both **load
  into the snowy hub / post-run NPC story sequence without a fatal error**.
- No in-game Save was performed. SHA-256 of both staged save copies remained
  unchanged after the emulator sessions. This confirms **a successful
  read/load**, *not* a successful save/re-save or lasting money change.
- Original/edited game sessions entered the same NPC dialogue/cutscene.
  Automated menu attempts after the cutscene did not yet produce a reliable
  in-game money reading, Save menu or in-game re-save. The absence of a
  re-save must be treated as **NOT TESTED**, not as a pass.

## Additional major correctness risk
Emerald Rogue keeps *current adventure state* and *stashed hub state* in
different locations. Public documentation of the game's original source
(`RogueSave_SaveHubStates` and `RogueSave_LoadHubStates`) describes the
`sRunRestoreBlock` copy of party, bag, daycare, difficulty, money, time.
At run boundaries, SaveBlock1 may swap to the other state. Verify this on the
**exact H168 build**, not generic Gen III assumptions.

Therefore even an emulator screenshot proving the active SaveBlock1 money +1
does NOT alone establish lasting hub-money edits. A proper validation must
check which state is active, both copies if applicable, and the data after a
full in-game save/reload and run/hub transition.

Reference for independent research: third-party `ddxtanx/Emerald-Rogue-Save-Editor`
README and `docs/FORMAT.md` as of 2026-11-21. It is not yet a validated
dependency and its version-specific offsets must not be assumed to match H168.

## Hard release gates

| Step | Requirement | 2026-11-21 outcome |
|---|---|---|
| 1 | Exact ROM and raw save independently identified | PASS |
| 2 | Copy-only minimal money change with source digest/counter preflight, exact byte whitelist, independent structural validation and byte-perfect original backup | PASS (offline) |
| 3 | Boot original and modified save as separate .gba/.sav pairs in a real GBA emulator | PASS (mGBA loads Continue and hub in both) |
| 4 | Observe new money amount in the game state that matters (hub or adventure); distinguish stashed vs current state | **OPEN** |
| 5 | Use normal in-game Save after edit, quit mGBA, reopen with freshly written save, verify state and 28 checksums and entire intended game state | **OPEN** |
| 6 | Independently prove original save restores and loads after a failed edit; keep second immutable backup | **OPEN for true emulator restore**, offline byte-exact backup verified |
| 7 | Physical 3DS read-only backup and external export inspected, no data loss in rollback drill | **OPEN** |
| 8 | Optional reversible install-path write, if independently approved, confirmed for title H168/AGBSAVE/CMAC and restored originals, not enabled by default | **BLOCKED** |
| 9 | Expand to money/items/IV/EV/moves only on individual proven offsets/pockets/PP rulesets; ability and PC gates separately | **BLOCKED pending per-feature proof** |

The existing main Phase20 experimental app **must remain SD-only**, direct
installed title save writes disabled, PR #16 a draft. A successful CIA
compile/Azahar UI test cannot be substituted for these gates.

## Next executable work
1. Inspect the exact ROM/source path that selects current/adventure/stashed
   hub money and identify save-state flags. Validate a different minimal test
   case at a hub save state rather than blindly editing active run money.
2. Use official mGBA `emu.loadSaveFile` temporary source or separate copied
   ROM+save pairs; never alter the original owner-uploaded save.
3. Automate a reproducible game menu / Save / reload path. Before writing any
   in-game save, assert pre-run SHA fingerprints and isolate the emulator
   directory. Validate the newly generated emulator save independently.
4. Develop a *read-only* physical 3DS checklist and backup presence checks
   for the owner; no CIA enabled for live save restoration without their
   explicit approval after all prior tests pass.
