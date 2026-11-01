# Integration QA — Phase 17/18/19 readiness and release gates

**Date:** 2026-11-01. **Status:** DRAFT / NO LIVE WRITE / NO MERGE APPROVAL.
**Independent QA branch:** feature/integration-qa, based on feature/phase7-native-ci.
**Audited, immutable QA inputs:**
- Phase17: c78b26ef51ba1511792c4362883b1f77cd7ed861 (PR #9).
- Phase18: ea7303e3d04e9c409febd44ee3e1f834de02cdc6 (PR #8).
- Phase19: 4e4d2a8dabe7d40ce2f78dc5b1c4dcf46a3b7c8e (PR #7).
New commits on those concurrently developed branches are OUTSIDE these pinned test results.

## Verified evidence (do not broaden its scope)

- Physical device, as reported by owner: Phase16 full 263168-byte container read/authentication and immutable backup/journal after application restart; Phase17 RAM-only restore/rollback rehearsal. Neither test wrote an installed save. A real native restore has **never** been proved.
- Existing Phase17 CI run 41324125202: SUCCESS at c78b26e (host fault/receipt/corrupt journal tests, native and emulator jobs). Emulator boot is not evidence of privileged native IO.
- Existing Phase18 CI has successful earlier strict and ASan/UBSan host runs. The selected ea7303e3 input was queued when this report was drafted; do not substitute older green runs for this commit.
- Phase19 source contains an opt-in exact 128KiB SD-copy reader using fopen("rb") and an explicit real-data edit denial in ui.hpp. This is a **structural read-only** viewer, not a game-validity or safety certification.
- **QA final recorded run 41326242101 PASSED** both jobs at commit `cc2d3c5b` (same pinned inputs): Phase11–17 inherited tests, Phase18 strict and ASan/UBSan tests, synthetic Phase18->Phase19 read-only viewer, negative read-only recovery, invalid copied-save rejection and ARM11 compile. The Phase14 synthetic 48/60 counter anomaly is confirmed in run 41326089410. The unified-header probe **intermittently fails** even when the workflow jobs are green, including run 41326236749. The report explicitly keeps combined compilation BLOCKED pending explanation, and this conditional diagnostic must not be taken as release success.
- **Separate Phase19 run 41325800066 PASSED** at the pinned UI snapshot, including host, devkitPro native, experimental CIA packaging and Azahar 3DSX/installed CIA startup smoke; **Phase17 run 41324125202 PASSED** at its pinned recovery snapshot, including analogous native/package/Azahar smoke tests. Those are standalone builds, not proof that the combined Phase20 application or privileged real-device restore works.

## Findings / defect and risk register

| ID | Severity | Finding | Resolution / disposition |
| --- | --- | --- | --- |
| QA-01 | HIGH | Phase19 CI checks out Phase18 commit 596743d8... rather than current ea7303e3... . Its green build can therefore miss newer parser and SHA-256 source-lock changes. | QA workflow pins the newer Phase18 and Phase19 snapshots and cross-tests them without altering either feature branch. Before integration, Phase19 owner must review/re-pin and re-run its own native packaging tests. |
| QA-02 | HIGH | Phase14 layout(), inherited by Phase17, selects numeric greatest GBA VC wrapper counter and only rejects equal counters. It does not establish adjacency of the two full-container counters. The Phase18 raw parser, by contrast, explicitly enforces modular adjacency/rollover. | Keep live restoration disabled. Define/test one authoritative wrapper policy, including stale/corrupt/nonadjacent/rollover cases, before integrating recovery with editing. Not fixed upstream. |
| QA-03 | HIGH | Phase17 operates on a 263168-byte authenticated container, while Phase18/19 operate on 131072-byte raw H168 copies. No reviewed raw-to-container patching, CMAC, durability or restart/gameplay contract spans all three. | Separate pure-memory read-only boundaries. No native write or restored-draft path. Block Phase20 write integration. |
| QA-04 | HIGH | Phase18's evidence allowlists/booleans are caller-provided and do not themselves attest to the exact installed ROM's species, moves, abilities, pockets or nature mechanics. The default unsupported capability flags are false, but future callers could set evidence without ROM proof. | Require independently recorded exact-build data provenance and mapping review, plus input refusal on unknown IDs; do not add write access via UI. |
| QA-05 | MEDIUM | Phase19 "validated copy" is checksum/geometry-valid, NOT independently ROM-version authenticated; unknown level/nickname and PC views remain unsupported. The final four 4096-byte sectors are outside the 28 checked H168 bank sectors. | Continue labeling read-only, unverified ROM identity. Require full source SHA-256 pin against stale/different copies, not only sector checksums. |
| QA-06 | HIGH | Existing recovery journal and RAM fault injection are not a production write-ahead recovery facility; receipt durability, native partial writes, filesystem power-loss semantics and independent backup recovery remain untested. | Block live writes. Require separately approved expendable-data 3DS testing and external recovery path in a future release. |
| QA-07 | MEDIUM | The Phase19 native build downloads an upstream devkitPro Makefile at build time from a moving branch, reducing reproducibility. Its CIA tests establish only isolated emulator/package behaviour. | Pin/review the build template, produce full SHA-256 artifact manifest, parse CIA metadata independently, and do not infer real-hardware success. |
| QA-08 | HIGH — CONFIRMED, INCONSISTENT CI | The pinned independent unified-header probe failed on QA runs 41325990864 and 41326236749 with duplicate `h168::Size`, `Bank`, `Info`, `Result`, and `r16` definitions from separate Phase17/18 `offline.hpp` paths; another identical-input run 41326089410 unexpectedly passed. This is **not** evidence of stable ABI compatibility. | Consolidate common declarations or isolate implementations behind versioned translation-unit boundaries, then require repeatable strict native/host unified compilation. Keep Phase20 direct-header integration blocked pending deterministic resolution. |
| QA-09 | HIGH — CONFIRMED | Phase13/14 authenticated restoration preflight permits only `MoneyPlusOne` and `ItemQuantityPlusOne`; it rejects any other operation and disallows broader value changes. Phase18 supports wider RAM-only money, item, IV/EV, moves and other requests. | **No generic Phase18-to-Phase17 restore adapter exists.** A separate reviewed, field-specific, byte-exact and authenticated transaction policy is required before any write integration. |
| QA-10 | MEDIUM — RESOLVED UPSTREAM | Phase19 snapshot `dad28767` failed `-Werror=reorder` in its `State` constructor initializers, preventing strict host and ARM11 compilation. | The independent Phase19 branch corrected initializer order at `4e4d2a8d`; QA pinned that fixed snapshot and verified its ARM11 reader compilation without modifying upstream code. |

**Fixes in this PR are deliberately isolated test coverage and deterministic revision pinning, not modifications to production code.** No Phase17, 18 or 19 source defect is claimed resolved on its own branch.

## Automated QA matrix

- [x] Dedicated reversible branch created, no upstream branch modified.
- [x] Add static source audit: false live-write flag, fixed SD-import rb, disabled unsupported capabilities, no tracked saves/ROMs/binary CIA.
- [x] Add Phase17 negative missing/unchecked/damaged/terminal-receipt evidence regressions.
- [x] Add Phase18/Phase19 exact-copy, 28-sector damage, malformed length, stale SHA-256, failed edit, checked offline candidate integration.
- [x] Include inherited Phase17 RAM fault suite and Phase18 strict/sanitized tests.
- [x] Add native ARM11 **compile-only** check for Phase19 viewer using Phase18 audited snapshot (not installation).
- [x] Add unified-header compile diagnostic and preserve logs if incompatible.
- [x] Record CI success at QA report commit `cc2d3c5b` (run 41326242101) and wrapper diagnostic commit `450a1157` (run 41326089410). Jobs passed; this is **not** release-gate approval.
- [ ] Resolve non-deterministic unified-header probe and require deterministic successful integration compile.
- [ ] Demonstrate a reproducible full 3DSX/ELF/CIA bundle against reconciled Phase18/19 code, verify title metadata and package hashes.
- [ ] Independently cross-check package and imported copied-save parser on emulator **without real data**, and verify canary/side-effect absence.
- [ ] Obtain exact matching H168 v2.1.2 ROM-derived, legally usable species/item/move/pocket/ability/nature metadata.
- [ ] Approve a versioned read-only Phase18/Phase19 interface with explicit field support flags and stable error semantics.
- [ ] Design and review immutable full-container backup/journal/receipt durability and power-loss recovery before any restoration evaluation.
- [ ] Obtain physical 3DS test evidence for the **integrated build** separately from Phase16/17 RAM-only PASS.

## Release-gate decisions

**G0 — privacy and separation:** No private save/ROM or live writer committed; keep all draft branches unmerged. **Required.**

**G1 — CI + memory safety:** Standalone host/sanitizer and ARM11 reader compilation **passed on QA commit `1858d7df`**. Unified-header inclusion **failed**; full current-source 3DSX/CIA packaging and emulator integration remain outstanding. **NOT PASSED AS A RELEASE GATE.**

**G2 — domain correctness:** Confirm H168 2.1.2 exact title/ROM/field semantics and safety of Pokémon recalculation, bank selection, item capacity and ignored sectors. **BLOCKED.**

**G3 — recovery correctness:** Verify independent full backups, write-ahead durability, signed receipts, stale journal refusal, interruption/rollback, 3DS hardware title handle privileges and after-image CMAC. **BLOCKED.**

**G4 — game behaviour:** Only after independent risk review, use disposable progress to check real 3DS launch, game load/re-save, persistence and rollback. No destructive power-loss test on the owner's existing save. **NOT STARTED.**

**G5 — release:** Read-only UI preview may be tested separately; restoration and real-save editing must remain disabled. **NO RELEASE APPROVAL.**

## Repeatable commands

The workflow .github/workflows/integration-qa.yml checks out exact pinned commits under dependencies/ and runs:

    bash qa/run.sh

No copyrighted ROM, real saved progress or attached SD card is needed. The Python source gate is:

    python3 qa/check_safety.py

The output from emulator tests, compiled objects and synthetic fixtures is never evidence of physical 3DS write compatibility. Re-pin all three references and re-run whenever upstream branches move. Keep the PR **draft**.
