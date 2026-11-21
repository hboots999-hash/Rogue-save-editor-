# Phase20 experimental single-move offline draft — first integration

Status: **DRAFT / NOT A GAME-PLAYABLE OR LIVE-RESTORE RELEASE**. This is a new
optional flow on the Phase20 experimental console-style UI, not the original
PKSM graphical editor requested in Issue #15.

## Implemented

- Uses the **new Phase18 move-v3** single replacement contract from PR #14,
  pinned by workflow commit SHA. It does not invoke the legacy PP-only v3
  scalar operation as a replacement.
- Imports an exact, **131,072-byte unwrapped H168 raw copy** from
  `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`.
- Requires an independently generated, owner-local 14,040-byte metadata file
  at `sdmc:/3ds/PKSM-Rogue/catalogs/exact_rom_move_metadata.bin`.
  The entire binary is SHA-256 authenticated against the known user's 2.1.2
  ROM fingerprint. The file is never committed or bundled in this CIA.
- In the existing move browser's **Review** state, press **Y** to preview an
  *experimental OFFLINE SD draft*. Only one occupied move slot, only one of
  the 43 exact-ROM agreed-PP IDs in Phase18 v3. A confirms the first warning,
  A confirms the final warning; B cancels.
- The imported copy is re-opened at final confirmation, checked against the
  active on-screen SHA-256, counter, Pokémon identity and original move.
  The catalog is re-opened and re-authenticated. The Phase18 move-v4 adapter
  reconstructs and independently checks the 128KiB candidate.
- The Phase18 exporter creates *new files* with O_EXCL, fsync and readback in
  `sdmc:/3ds/PKSM-Rogue/drafts/`: 
  `move_trial_NNN_backup.sav` and `move_trial_NNN_draft.sav`.
  The /drafts/ directory must already exist. The fixed source import is
  never modified.
- Existing money/item/IV/EV/PP single and scalar-batch edits remain unchanged.
  Never merge an experimental move replacement with scalar edits yet.

## Safety

**No installed game writes, title archive access, CMAC, restore or direct
gameplay injection are wired into Phase20.** The original Phase19 move
browser's normal A-based Apply path is still deliberately blocked.

The 43 IDs are *not* certified learnable by each species. The experimental
rule clears only the replaced slot's PP-Up counter. Mode-dependent battle PP,
Rogue-specific learn/forget behaviour, real-game loading, persistence and
subsequent re-saving are **not independently proven**. Do not inject this
draft into the installed game or replace live save data.

The software identifies the *claimed* ROM SHA and catalog file, but that
does not prove the installed active game on the console is byte-identical.
Save backups from GodMode9/Checkpoint remain separate and must be preserved.

## QA needed before physical 3DS testing

1. Strict, ASan/UBSan and native ARM11 compiler jobs pass at the *exact* PR
   commit. 3DSX, ELF and experimental distinct-title CIA package generated.
2. Existing Azahar synthetic-money/item batch flow remains green. Additional
   on-console testing of the new Y flow is required.
3. First physical test: open the menu on the experimental title with **no**
   private imports, verify no crash and verify START+SELECT Rosalina remains
   functional despite broken L. The UI must refuse experimental draft from
   synthetic mode.
4. Then on a copied, disposable source + authentic catalog, preview a
   replacement, cancel, verify no file, and separately export a draft.
   Compare full source/backup/draft SHA-256 independently before any attempt
   to load the draft in a separate emulator save environment.
5. No installed-game save restore without independent real gameplay,
   save/reload, recovery and CMAC verification.

## Source

- Original PR #12: prior Phase20 editor, intact.
- PR #14: Phase18 `move_replacement_v3.hpp`,
  `offline_move_export_v3.hpp` and `move_queue_v4.hpp`.
- This integration PR: `phase20/move_draft_v1.hpp`, native input view and
  host fail-closed regression. All code GPL-3.0-or-later.
