# Phase 20: isolated read-only integration preview (DRAFT)

**Date:** 1 November 2026. **Branch:** `feature/phase20-readonly-integration`. **Base:** `feature/integration-qa` (stacked on draft QA PR #10), **not merged**.
**Status:** Experimental combined build. [Integration CI #41328064232](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41328064232) PASSED host strict/ASan/UBSan, native ELF/3DSX and separate-title CIA packaging. Azahar startup is a new independent optional check, pending latest CI. **No installed GBA VC save writes and no restoration approvals.**

## Pinned component inputs

| Source | Pin | Why |
| --- | --- | --- |
| Phase17 PR #9 | `9908eab15a79c5b64126335d746b140028d05af1` | Public `readonly_status_v1.h` C ABI, fixed-size status, zero capability bits; producer remains separate |
| Phase18 PR #8 | `59888cb7194e2a819602a58ddfa76829d060aa00` | Isolated `rogue18::raw_v1` parser namespace, `SaveView` model v1 and source SHA-256 |
| Phase19 PR #7 | `f9b7874ad70a04f71de1b9e2fc6a030dba369016` | L-free 3DS UI, guarded copied-save importer, no real-data edit states, Rosalina START+SELECT safe exit |
| Original independent QA | `8c5cc54fd96607a5706e8934c3ebbccf23c88f95` | Prior negative tests, release-gate register and documented Phase14 concerns |

**Re-pin when Phase17/18/19 fixes are finished; existing CI evidence is only for the exact SHA set tested.**

## What Phase20 has implemented

- `phase20/compat_phase19_v1.hpp`: temporary **read-only** namespace shim translating unqualified Phase19 `h168` into Phase18's scoped `rogue18::raw_v1`. This translation unit **must never include Phase14/Phase17 internal `h168` headers**. Phase17 public ABI has no such namespace.
- `phase20/read_only_v1.hpp`: verified copy-reader boundary, validates a raw exact **131072-byte** SD copy independently through Phase18's lightweight `rogue18::validate` / `partyAt` / `h168::item` plus source SHA-256, and checks it against the Phase19 UI model, checks counter/money/party/items/digest agreement before publishing visible values. Invalid sizes/checksums leave the visible snapshot and source fingerprint unchanged. Uses static large UI staging, heap-backed exact SD read, `fopen(...,"rb")` only. Distinguishes the **263168-byte** authenticated VC container without conversion or auto-trimming. Source SHA includes normally unchecksummed tail bytes. The native test rejects any >4096 byte stack frame and publishes its `.su` stack-usage report alongside ELF artifacts. Evidence flags `canModifyInstalledGame` and `canRestore` are always false.
- Phase17 `P17ReadOnlyStatusV1` is a separately compiled C ABI with zero write capabilities. Even when the producer reports RAM-only evidence, Phase20 labels it a **claim only**, never authenticated independently or usable to restore. Current combined native preview initializes this to **NOT CHECKED** rather than opening any live save or pretending hardware verification.
- `phase20/main.cpp`: Phase19 UI integrated with Phase20 guarded SD-copy reader; L-free navigation, existing demo/viewer screens and broken-L alternatives remain. A copied real-data import is entirely read-only. No native game archive is opened.
- `phase20/test_read_only_v1.cpp`: synthetic 28-sector corruption, malformed lengths/wrapped container, stale unchecksummed trailer digest changes, safe mock/real state transitions, immutable SD stream readback and Phase17 ABI tamper/status denial.
- `.github/workflows/phase20-readonly.yml`: strict compiler + ASan/UBSan combined host check, native ARM11 compilation, isolated 3DSX/ELF and distinct title **000400000F73A700** CIA packaging with title-ID scans and SHA-256 manifest. No physical device required.

## Safety and scope of this milestone

- **Imported data can be displayed, not edited.** Synthetic editing UI is a demonstration with final application blocked. `rogue18::propose`, SD offline exports, Phase14 authentication signer, and installed-title write/restore paths are not called.
- No derived data or private save bytes enter repository or CI. Synthetic fixtures are explicitly not playable game evidence.
- `P17ReadOnlyStatusV1` confirms data **shape** only; do not confuse a plausible envelope with fresh, authenticated container proof. The Phase20 UI currently reports NOT CHECKED because the native producer is not linked.
- All three upstream development PRs remain independent drafts. Phase20 changes are confined to this separate branch.
- This combined preview has NOT been validated on physical Nintendo 3DS, not demonstrated in gameplay, and does NOT attest to precise ROM v2.1.2 item/move/ability/nature layout or safe restore.
- The native template URL is now pinned to devkitPro/3ds-examples commit `be2001fee08cf8ec7d3366e095e83f6f75da8942` (source hash still to be recorded); package CI passing alone would not satisfy reproducible release requirements.
- Old QA counter policy QA-02, authentic data/ROM semantics QA-04 and recovery durability QA-06 remain release blockers. Future Phase20 status producer integration needs an explicitly reviewed read-only native API and cannot be inferred from this mock NOT CHECKED envelope.

## Acceptance gate for following milestone

- [ ] Current updated component pins: strict/sanitizer and independent host-only full SaveView parity; previous run 41328064232 passed older pins, not this revision.
- [ ] Current updated native ARM11 3DSX/ELF and separate-title CIA, including `-Werror=frame-larger-than=4096`, must pass again; previous run 41328064232 passed older pins.
- [ ] Remove temporary global `h168` compatibility alias after Phase19 adopts explicit versioned `rogue18::raw_v1` types.
- [ ] Phase17 independent recovery evidence compatible with host+native, without live-save write.
- [ ] ROM-build exact provenance and evidence established before any wider offline editor controls.
- [ ] Independent physical 3DS read-only copy viewer and exit/navigation test, when appropriate.
- [ ] **Restoration and installed save editing remain disabled regardless of above checks.**

Latest workflow also attempts *optional* Azahar 3DSX and installed-CIA boot smoke with an isolated SD canary, which is NOT physical 3DS evidence. Tests: GitHub Actions workflow `phase20-readonly.yml` (strict C++17 and ASan/UBSan + devkitARM + separate CIA). Do not merge draft PR as a release.

## 1 November 2026 — Offline copied-save editing integrated (still DRAFT)

Phase20 now contains `offline_edit_v1.hpp` and a native 3DS UI action bridge in `main.cpp`. Without changing Phase19's read-only session model, the new separate controller intercepts deliberate A-presses on imported Money, existing Bag stack and existing non-egg Party Pokémon. It offers **money, existing quantity, individual IV, individual EV (510 budget) and existing move PP only**, with two A confirmations and B cancellation. All unverified nature, ability, species, move replacement, insertion and PC editing remain blocked.

A successful final confirmation uses Phase18 **v2** `begin / beginMove / preview / verify`, with captured source SHA-256/counter and displayed-value consistency, followed by `rogue18::exportCopied` at the fixed SD import. This creates a new read-back-verified backup and distinct offline draft. The input is never overwritten; no installed title or authenticated VC write path is linked. Each exported candidate contains a **single edit** relative to the imported copy. Original UI values remain shown until a new import is explicitly requested.

New `test_offline_edit_v1.cpp` checks the five offline edit classes where fixture values differ, exact-byte draft verification, immutable original/backup, stale file and hash refusal, invalid fields/bounds, unsupported imported edit gates, and two-confirmation UI cancellation/success through the actual guarded exporter. Latest Phase20 workflow also links offline export into native ARM11 with 4096-byte compile-time per-frame limit, runs strict and ASan/UBSan host suites, produces a separately titled non-release CIA and checks optional emulator startup. See `OFFLINE_EDITOR_GUIDE.md`.

**Still outstanding:** independent physical 3DS stability and SD draft readback, private target-ROM compatibility, gameplay/stat recalculation, full save-reload persistence, and independent real recovery. Earlier Phase19 crash is not resolved merely by Phase20 CI. This remains experimental and NOT cleared for live game writing, injection or release. Keep PR #12 DRAFT.


## 1 November 2026 evening — Phase20 native v3 multiple-edit queue VERIFIED IN AZAHAR

- Phase18 v3 pinned to `3625e160a17f805821098cf35a47a0f96aba8e16` and native ARM11 includes `multi_edit_v3.hpp` plus `multi_export_v3.hpp`.
- Phase20 `offline_edit_v1.hpp` now owns a separately staged, source-digest-locked `BatchFlow`: native **Y** starts and reviews RAM-only queue; existing money, bag, IV, EV and PP A-based value screens stage their scalar changes. Queue review pages five edits, X removes last, B cancels, final A/A/A review sequence includes **two distinct export confirmations** before creating one offline SD backup and one combined draft. Source/identity, final EV budget, changed-byte footprint, combined proof and readback are delegated to the additive Phase18 v3 API; existing v2 single-edit screen remains intact.
- Queue-aware EV limits allow a staged decrease on one stat to free headroom for a later stat increase. Conflicting/duplicate/no-op/invalid/stale entries are blocked. Every new import resets the queue; real PC/species/ability/nature/new-item/new-move editing stays disabled.
- Host regression `test_offline_edit_v1.cpp` drives three staged menu actions (money, occupied bag, party IV), B cancellation of final review, two final confirmations, independent combined save proof, source tamper refusal and EV redistribution from a 508-point source.
- **PASS: [all four jobs at CI run 41349356154](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41349356154)**: strict and ASan/UBSan C++17, native ARM11 3DSX/ELF with 4KiB frame cap, experimental distinct-title CIA metadata and package, and **mandatory Azahar multi-edit native menu test**. Azahar first produced independent money 40001→40002 draft, then triggered native Y queue, staged money+1 and existing item quantity 10→11, double-confirmed one additional SD batch draft. `verify_emulator_draft.cpp` rebuilt/verified all 131072 bytes of the batch using Phase18 v3, and checked immutable original backups and SD canary. This job does not use private ROM/save material.
- Jules QA PR #13 had unrelated history in its diff when inspected and was retargeted as a **draft** to `feature/phase20-readonly-integration`. It must not be merged wholesale; only independently reviewed and rebased QA-only changes may be considered.

**Open gates:** physical Nintendo 3DS copy-import/backup/draft-export testing with expendable SD files and independent originals; exact game build/ROM semantic compatibility; game-load/resave effects and cached stats; live restoration durability. **No live game writer/restorer is enabled.** Keep PR #12 draft and do not install experimental CIA as a release.
