# Emerald Rogue: ONE move-and-ability NPC in the main outdoor hub

**Status:** source implementation / native GBA build verification in progress.
**This is NOT a ready-to-install 3DS CIA.** Do not install source outputs over
the user's existing Emerald Rogue 2.1.2 title or inject a save.

## What is actually implemented

One scientist NPC is added to `data/maps/Rogue_Hub/map.json` at **(31,19)**
(along the main hub path near the Pokémon Centre area), with script
`RogueHub_SpeakCustomEditor`, always visible (`flag=0`).

NPC flow:
1. Speak to NPC and select **CHANGE MOVE** or **CHANGE ABILITY**.
2. Choose one non-egg party Pokémon using existing `ChoosePartyMon` UI.
3. **Move**: pick any available compiled battle move with nonzero base PP from
   the game's names, choose one of four replace slots, confirm Yes.
   Duplicate moves/invalid or zero-PP IDs are rejected. The old PP-Up count
   for that slot is cleared before `SetMonMoveSlot` assigns the new PP.
4. **Ability**: choose an ability from the compiled Gen9 ability IDs, or
   **SPECIES DEFAULT** (clears the custom choice), then confirm Yes.
   A custom ability is stored per Pokémon in 10 previously unused Growth
   bits (`0`=default; `N`=ability ID `N-1`); this changes no struct
   length. The ability lookup used in the battle-move copy and Summary
   screen reads that custom value. Existing save ability slots are retained.
5. A normal in-game **Save** is required to persist the chosen edit.
   Edits are not written to the GBA VC save archive by any external tool.

All special hooks and entry code are in `src/rogue_npc_editors.c`; the
installer patches `pokemon.c`, `pokemon.h`, `pokemon_summary_screen.c`,
`data/specials.inc`, main outdoor hub map JSON and Pory script.

## START-menu access (new overlay)

The **START → EDIT MON** action is now available in the ordinary hub and
during a Rogue run when the original nine-row GBA menu has capacity. No L
shoulder button and no NPC visit are needed. It opens **CHANGE MOVE** and
**CHANGE ABILITY** using the same party picker, dynamic lists, validation,
encrypted per-mon ability override, and save path as the existing scientist.

Apply both overlays to a *clean* upstream EX-v2.1 source checkout, in order:

```sh
python3 rogue_npc_mod/apply_source_overlay.py ./clean-rogue-ex-v2.1
python3 rogue_npc_mod/add_start_menu_editor.py ./clean-rogue-ex-v2.1
python3 rogue_npc_mod/test_overlay.py ./clean-rogue-ex-v2.1
```

The START option is hidden in catching contests and when the original menu
already fills all nine rows; this prevents GBA window/tile overflow. Unlike
the scientist's script, the START callback uses `lockall` and does not
assume an NPC is facing the player.

**This remains source code, not a verified 2.1.2 CIA.** The uploaded
Recruiter/Tutors CIA has a different ROM fingerprint. Do not claim that
successful EX-v2.1 compilation patches that exact installed title or
automatically preserves its existing NPCs and save without further matching
and hardware/emulator tests.

## Reproduce the patch against an upstream tree

Use an independent **clean copy** of the public
`Pokabbie/pokeemerald-rogue` source at tag `EX-v2.1`:

```sh
python3 rogue_npc_mod/apply_source_overlay.py ./clean-rogue-ex-v2.1
python3 rogue_npc_mod/test_overlay.py ./clean-rogue-ex-v2.1
```

The overlay refuses missing/mismatched anchors, an existing NPC source file,
and duplicate patch applications. Source tests check both menu branches,
battle/summary override plumbing, new special registrations and position.

## Accuracy and release gates

- The public **EX-v2.1 source** is *not proven identical* to the user's exact
  2.1.2 3DS VC ROM SHA-256
  `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`.
  A successful EX-v2.1 compile would be a **modified upstream build**,
  **not an exact binary modification of the user's installed game**.
- A compiled GBA ROM is also **not** a Nintendo 3DS VC CIA. Repackaging into
  a separate Title ID and testing the original save compatibility are
  separate gates.
- The 10-bit encoding is compatible with the storage *layout*, but every
  PC/battle/cross-version code path and reserved bit use must be verified.
  Some dynamic, transformation or form-dependent abilities may not behave
  properly with custom selection; these must be tested individually.
- Physical 3DS testing of this custom NPC has not occurred. Keep two
  immutable copies of the owner's exact save before even considering
  hardware trials; never overwrite the installed title before
  independent recovery is proven.
- The source modifications do **not** alter all TM legality checks globally;
  the NPC specifically offers the general replacement list.
- No ROM, existing private save, or copyrighted assets are stored in this repo.

## Automated integration

[GitHub Actions workflow](../../.github/workflows/hub-npc-mod.yml) attempts
both source tests and an arm-none-eabi GBA compile. The output is **build logs
and hashes only**; it deliberately does not publish a ROM/CIA.

**Next actual milestone:** get a reproducible GBA build and show
the hub NPC opening both menus, with a saved/restored custom ability and
move surviving emulated game save/reload. Only then consider a separate
3DS VC install package.
