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

| PC storage | Source-derived 14×30×80 section-5–13 candidate with checksum checks | **Read-only research status only**, PC editing blocked | Unconfirmed exact 2.1.2 build/actual populated PC |
| Phase 19 interface | Versioned `SaveView`, capability gates, `beginRequest` optimistic locks | Read-only snapshot; unsupported editing visibly disabled | Integration and hardware tests belong to Phase19 |

## Source changes

- `phase18/offline_engine.hpp`: checked pure-memory engine and exact full-buffer candidate proof.
- `phase18/offline_export.hpp`: fixed import path, verified immutable backup and distinct draft with source rechecks.
- `phase18/test_offline_engine.cpp`: synthetic positive/negative/rollback suite.
- `phase18/test_offline_export.cpp`: synthetic filesystem staging/regression suite.
- `phase18/bag_rules.hpp`: source-backed capacity and pocket-order verification.
- `phase18/pc_readonly.hpp`: conservative PC candidate inspector.
- `phase18/editor_model.hpp`: versioned Phase19 read-only snapshot and guarded editing request builder.
- `phase18/test_pc_readonly.cpp`, `phase18/test_editor_model.cpp`: PC corruption and model contract tests.
- `phase18/SOURCE_FINDINGS.md`: upstream source-vs-observed H168 byte-offset audit.
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

**Resolved stale source offset comments:** the EX-v2.1 source's 104-byte Pokemon stride means money is actually at `0x238 + 6*104 = 0x4A8`, and the 50 PC item slots shift the actual bag to `0x578`, matching the historical private H168 read-only observation. The older `0x490/0x560` comments were based on 100-byte party records. The candidate bag-upgrade byte is `0x4AD` by public-source geometry but is still **unverified on the exact private v2.1.2 ROM**; the insertion gate requires explicit verified evidence and a matching saved value.

**Phase16 user hardware update:** GBA VC full container authentication, 263168-byte untouched backup, 526400-byte recovery journal and restart verification reportedly passed on physical hardware. This is valuable upstream recovery evidence but does NOT authorize Phase18 live writes or establish Pokémon/PC semantics.

**Integration:** Phase19 should include `phase18/editor_model.hpp`, call `inspectReadOnly(...)` on a copied offline raw save, construct requests with `beginRequest(view,kind,slot,request)`, fill only requested new values after user confirmation, and call `propose`/byte-exact `verifyExact`. The `SaveView.capabilities` booleans distinguish default on-copy numeric edits from operations still requiring independent evidence. The optional PC status cannot be treated as verified. Only Phase18's separate offline SD import exporter can make immutable backups and drafts. No live file handles or game archive access are involved.

**Final expanded source CI PASS (1 November 2026):** [GitHub Actions run 41259163807](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41259163807) on `f86ca56ddd329802c36860e0c05983c1af691caf`: all **three** jobs passed — strict C++17 host, ASan/UBSan host, and **devkitPro native ARM11 C++ header integration compile**. Tests include 24 permutations, different bank keys, 28-sector corruption, ciphertext integrity, key failure, exact forward/reverse edits, source-derived 80→450 bag rules, reserved-pockets sorting/insertion guards, PC section crossing and checksum rejection, Phase19 snapshot/request API, stale inputs, separate immutable backup/draft and independent Python inspector. Native compile is a **header integration check only**, not a launched 3DSX/CIA or hardware gameplay proof.


## 1 Nov 2026 Phase18 additional hardening (for Phase20)

- Reused existing PR #8 and branch; no merge, no live writer/restorer changes.
- Added `layout_geometry.hpp` with checked party, money, bag and candidate
  upgrade offsets; resolved source-comment discrepancy from 6 × 104-byte records.
- Added `source_digest.hpp` portable SHA-256 with known-answer tests. Phase19
  read-only `SaveView.sourceSha256` is pinned to `Request.expectedDigest` to
  reject an edited input whose active counter and selected money/quantity/mon
  values remain unchanged.
- Fixed the 32-bit save counter rollover bug that inherited Phase10's
  numerically-greatest-bank assumption. Synthetic tests cover both 0xFFFFFFFF→0
  directions, invalid gaps, equal counters, and empty parties.
- Source-backed item insertion now checks the candidate saved upgrade count
  against explicit verified H168 offset evidence. Pocket map/ID mappings remain
  unverified: normal insertion stays blocked.
- Added `beginMoveRequest` to the Phase19 model, which safely preloads the
  selected move slot's expected ID and PP, rejecting absent/invalid slots;
  invalid three-slot ability values also fail validation.
- Added `test_integrity.cpp` (stale same-counter saves, SHA-256 known vectors,
  rollover, empty party) to the strict/sanitizer CI jobs. All data remains
  generated/non-playable synthetic input only.
- See `PHASE20_FEATURE_MATRIX.md` for the per-feature verified/source-derived/
  experimental/blocked status and exact remaining ROM/emulator gates.

**Phase16/17 progress is acknowledged but does not grant Phase18 permission
to write installed saves. All Phase18 outputs are offline drafts only.**


## Verification of hardened continuation — CI and Phase20 handoff

**PASS:** [GitHub Actions run 41325167130](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41325167130) for feature commit `0c92432922a97d45738c0420f6d72777826efb2a`, with all **three** jobs successful: strict C++17 host; ASan/UBSan host; devkitPro native ARM11 header compile. The host regression runner now includes SHA-256 known-answer vectors, same-counter stale input rejection, both UINT32_MAX/zero bank rollover directions, empty-party money edits, verified candidate bag-upgrade matching, four-slot move request guards, corrupt/oversized Pokémon and item requests, PC read-only candidate parsing, exact rollback, and zero live-save operations.

**Source discrepancy:** upstream `0x490/0x560` comments were stale after 104-byte `struct Pokemon`; `layout_geometry.hpp` formally derives observed `0x4A8/0x578`. The bag upgrade at `0x4AD` is still a **source-derived candidate**, not confirmed on the exact private v2.1.2 ROM. No byte-level private save/ROM evidence has been committed.

**Integration contract:** Phase19 should call `inspectReadOnly`, retain `SaveView.sourceSha256`, create requests through `beginRequest` or `beginMoveRequest` (for moves 0–3), and then call `propose` with that SHA-256 guard; `verifyExact` must validate the output before any offline export. Unsupported abilities, new move identifiers, nature, generic item insertion, and PC mutations remain blocked by default. This branch never interfaces with Phase16/17 restoration; PR #8 remains **draft**.

The complete per-feature decision matrix and missing proof are in [PHASE20_FEATURE_MATRIX.md](PHASE20_FEATURE_MATRIX.md). The three-job CI run is a **synthetic/compile pass, not a real-ROM/gameplay test**.
