# Phase 20: isolated read-only integration preview (DRAFT)

**Date:** 1 November 2026. **Branch:** `feature/phase20-readonly-integration`. **Base:** `feature/integration-qa` (stacked on draft QA PR #10), **not merged**.
**Status:** Experimental synthetic-only combined build. **No installed GBA VC save writes and no restoration approvals.**

## Pinned component inputs

| Source | Pin | Why |
| --- | --- | --- |
| Phase17 PR #9 | `e3fd4971071de91285a4c42b3717b1d1f70999e9` | Public `readonly_status_v1.h` C ABI, fixed-size status, zero capability bits; producer remains separate |
| Phase18 PR #8 | `9569c2ac3f7693ee9d9bed5b30a84f79cb8c2d91` | Isolated `rogue18::raw_v1` parser namespace, `SaveView` model v1 and source SHA-256 |
| Phase19 PR #7 | `e0b19c562b9b2ddf7148e01ecb84a88f1d04c921` | L-free 3DS UI, guarded copied-save importer, no real-data edit states, Rosalina START+SELECT safe exit |
| Original independent QA | `8c5cc54fd96607a5706e8934c3ebbccf23c88f95` | Prior negative tests, release-gate register and documented Phase14 concerns |

**Re-pin when Phase17/18/19 fixes are finished; existing CI evidence is only for the exact SHA set tested.**

## What Phase20 has implemented

- `phase20/compat_phase19_v1.hpp`: temporary **read-only** namespace shim translating unqualified Phase19 `h168` into Phase18's scoped `rogue18::raw_v1`. This translation unit **must never include Phase14/Phase17 internal `h168` headers**. Phase17 public ABI has no such namespace.
- `phase20/read_only_v1.hpp`: verified copy-reader boundary, validates a raw exact **131072-byte** SD copy independently through Phase18 `SaveView` and Phase19 UI model, checks counter/money/party/items/digest agreement before publishing visible values. Invalid sizes/checksums leave the visible snapshot and source fingerprint unchanged. Uses static large UI staging, heap-backed exact SD read, `fopen(...,"rb")` only. Distinguishes the **263168-byte** authenticated VC container without conversion or auto-trimming. Source SHA includes normally unchecksummed tail bytes. Evidence flags `canModifyInstalledGame` and `canRestore` are always false.
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
- The native template still comes from a moving upstream URL pending hash/SHA pin; package CI passing alone would not satisfy reproducible release requirements.
- Old QA counter policy QA-02, authentic data/ROM semantics QA-04 and recovery durability QA-06 remain release blockers. Future Phase20 status producer integration needs an explicitly reviewed read-only native API and cannot be inferred from this mock NOT CHECKED envelope.

## Acceptance gate for following milestone

- [ ] Pinned exact-sha combined host strict and sanitizers pass; first run caught incompatible old Phase19 model, corrected SHA in subsequent CI.
- [ ] Native ARM11 compiled/link outputs and separate-title experimental CIA pass title-ID safety tests.
- [ ] Clear deterministic namespace boundary without alias hacks after Phase19 upstream adopts explicit versioned types.
- [ ] Phase17 independent recovery evidence compatible with host+native, without live-save write.
- [ ] ROM-build exact provenance and evidence established before any wider offline editor controls.
- [ ] Independent physical 3DS read-only copy viewer and exit/navigation test, when appropriate.
- [ ] **Restoration and installed save editing remain disabled regardless of above checks.**

Tests: GitHub Actions workflow `phase20-readonly.yml` (strict C++17 and ASan/UBSan + devkitARM + separate CIA). Do not merge draft PR as a release.
