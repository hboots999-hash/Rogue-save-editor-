# Phase 18 handoff — offline editors, no live writes

**Branch:** `feature/phase18-offline-editors`  
**Base (unmodified):** `feature/phase7-native-ci`, commit `64cb5f0d4c393e23c4b2da70a4643c5b81f68d3a`  
**Target game:** observed package Pokémon Emerald Rogue v2.1.2, GBA-P-H168, 3DS title `0004000007D7BA00`.  
**Merge status:** **DRAFT / NOT RELEASE-READY**.

## Implementation progress

| Editor feature | Implementation | Default gate | Important gap |
| --- | --- | --- | --- |
| Real H168 section/party/bag layout | Dual rotated banks, 104-byte party stride, PID/OT XOR, all 24 substructure orders, section + Pokémon checksums | Strict 128KiB structural/counter/quantity validation | Not independently rerun here on a private live-copy save |
| Money | Encrypted active section-1 field, limits 0–999999 | Counter and old-value match | No game-side re-save test |
| Existing stack quantity | Encrypted, 1–999, guarded slot and item ID | Default enabled only for strict copied save | Pocket game behavior unverified |
| Pokémon IVs | Six 5-bit IVs, preserves egg/ability bits | PID, OT, species, Pokémon checksum guards | No game-side test |
| Pokémon EVs | Six 0–252 EVs, total ≤510 | Same guards | No game-side test |
| Moves and PP | Four move slots, ID and PP, checksum repair | PP-only for unchanged ID; new ID needs verified ROM allowlist | Rogue 2.1.2 move-ID table unresolved |
| Ability | Bit-31 binary ability slot in encrypted Misc block | Requires independently verified mapping in `Evidence` | Not an arbitrary ability-ID editor |
| Nature | PID %25 display/read information | **Always rejects mutation** | PID/gender/shiny/custom mechanics need proven safe procedure |
| Item insertion | Contiguous bag append and encrypted quantity | **Reject by default**; verified pocket/ID policy required | Pocket capacities/order unresolved |
| Save delivery | Pure RAM candidate + optional copied-SD-only export | Immutable backup, no overwrite, full readback and stale checks | No live restore, no CMAC or actual hardware user test |

## Source changes

- `phase18/offline_engine.hpp`: checked pure-memory engine and exact full-buffer candidate proof.
- `phase18/offline_export.hpp`: fixed import path, verified immutable backup and distinct draft with source rechecks.
- `phase18/test_offline_engine.cpp`: synthetic positive/negative/rollback suite.
- `phase18/test_offline_export.cpp`: synthetic filesystem staging/regression suite.
- `phase18/README.md`: safety model and integration instructions.
- `.github/workflows/phase18-offline-editors.yml`: strict + sanitizer host suites and independent Python reader.

## Test/evidence accounting

- **Historical relevant evidence:** `phase9/profiles/confirmed_h168_save_layout_READ_ONLY.md` documents a prior private real-save read-only inspection and 28/28 sector checksums. It does NOT authorize all feature writes.
- **Phase18 host CI PASS (1 Nov 2026):** [run 41257531814](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41257531814) on source commit `d319753579c0e592873274eb14c3f594525b0fb7`: **both** strict C++17 and ASan/UBSan jobs passed deterministic rotated-bank fixture with two 104-byte Pokémon, **all 24 PID block permutations**, expanded species, differing XOR keys, exact rollback, guarded edits/stale/corrupt cases, 28 independent sector checks, phase10 immutable SD-only backup/draft roundtrips and Phase9 Python cross-decoder.
- **Not tested by this phase:** actual private H168 save, exact ROM move/ability/pocket semantics, emulator gameplay, device installation/SD flush, save persistence, Luma privileged reads/writes, CMAC signing, live restore or rollback.

## Blocking issues and follow-up

1. Independently extract/verify the exact H168 embedded ROM's item pocket map, species/move/ability metadata and nature mechanics, without publishing ROM bytes.
2. Run proposed feature edits on a **fresh private copy** in an isolated test environment with independent parser/ROM compatibility and gameplay/save-reload evidence. Do not enable real-save live mutation on this basis.
3. Integrate new editor controls through a separate PR: select the active party Pokémon / bag slot, capture expected current fields, present a confirmation screen, pass a reviewed `Request`, and use the fixed SD-only exporter.
4. Create reproducible power-cycle and independent recovery evidence before any separate restoration review. Phase18 cannot restore or authenticate the GBA VC container.
5. Keep `feature/phase14-authenticated-restore`, Phase 15/16 and all other branches untouched. This Phase18 PR targets `feature/phase7-native-ci` only.

**User safety:** no personal saves or ROMs were committed. Do not install or inject Phase18 outputs into the actual game. Treat every Phase18 .sav output as an offline experimental draft until the separate release gates are satisfied.
