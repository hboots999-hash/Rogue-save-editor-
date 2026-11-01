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
| Ability | Corrected 2-bit EX ability index at Misc+8 bits29..30; preserves Gigantamax flag | Requires verified matching-ROM mapping in `Evidence` | Not an arbitrary ability-ID setter; real v2.1.2 gameplay untested |
| Nature | Source-derived 5-bit header modifier; preserves PID/OT, language, encryption and Pokémon checksum | **Blocked by default**, enabled only with verified matching-ROM nature semantics | Real v2.1.2 gameplay/stat recalculation untested |
| Item insertion | Append only; source-backed 80→450 upgrade formula, 50 key-item slots and nine-pocket order | **Rejected by default** until exact H168 upgrade byte and every item's pocket are verified | No generic mid-pocket insertion; source version not ROM matched |
| Save delivery | Pure RAM candidate + optional copied-SD-only export | Immutable backup, no overwrite, full readback and stale checks | No live restore, no CMAC or actual hardware user test |

| PC storage | Source-derived 14×30×80 section-5–13 candidate with checksum checks | **Read-only research status only**, PC editing blocked | Unconfirmed exact 2.1.2 build/actual populated PC |\n| Phase 19 interface | Versioned `SaveView`, capability gates, `beginRequest` optimistic locks | Read-only snapshot; unsupported editing visibly disabled | Integration and hardware tests belong to Phase19 |\n\n## Source changes

- `phase18/offline_engine.hpp`: checked pure-memory engine and exact full-buffer candidate proof.
- `phase18/offline_export.hpp`: fixed import path, verified immutable backup and distinct draft with source rechecks.
- `phase18/test_offline_engine.cpp`: synthetic positive/negative/rollback suite.
- `phase18/test_offline_export.cpp`: synthetic filesystem staging/regression suite.\n- `phase18/bag_rules.hpp`: source-backed capacity and pocket-order verification.\n- `phase18/pc_readonly.hpp`: conservative PC candidate inspector.\n- `phase18/editor_model.hpp`: versioned Phase19 read-only snapshot and guarded editing request builder.\n- `phase18/test_pc_readonly.cpp`, `phase18/test_editor_model.cpp`: PC corruption and model contract tests.\n- `phase18/SOURCE_FINDINGS.md`: upstream source-vs-observed H168 byte-offset audit.
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

## 1 November 2026 continuation — material correction

The earlier Phase18 representation of the Gmax bit as an ability slot was **incorrect** for the public EX-v2.1 Pokémon layout and has been corrected. The original simple species-u16 interpretation was also unsafe when a held item occupies packed Growth bits. See `SOURCE_FINDINGS.md` for source links, the precise 11-bit species/11-bit split held item, 2-bit ability index, nature modifier, source-derived capacity and PC model.

**Critical version mismatch:** EX-v2.1 public source reports money at SaveBlock1 +0x490 and bag +0x560, while the independently observed private H168 v2.1.2 has money +0x4A8 and bag +0x578. Matching-ROM source verification is still a release blocker. Do not infer the target bag-upgrade byte offset by simply adding +0x18.

**Phase16 user hardware update:** GBA VC full container authentication, 263168-byte untouched backup, 526400-byte recovery journal and restart verification reportedly passed on physical hardware. This is valuable upstream recovery evidence but does NOT authorize Phase18 live writes or establish Pokémon/PC semantics.

**Integration:** Phase19 should include `phase18/editor_model.hpp`, call `inspectReadOnly(...)` on a copied offline raw save, construct requests with `beginRequest(view,kind,slot,request)`, fill only requested new values after user confirmation, and call `propose`/byte-exact `verifyExact`. The `SaveView.capabilities` booleans distinguish default on-copy numeric edits from operations still requiring independent evidence. The optional PC status cannot be treated as verified. Only Phase18's separate offline SD import exporter can make immutable backups and drafts. No live file handles or game archive access are involved.

**Final CI:** refer to the newest Phase18 workflow PASS for the final source commit. Older [run 41258801141](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41258801141) passed strict and ASan/UBSan for the earlier source correction, PC candidate and model tests before final bag-capacity hardening.
