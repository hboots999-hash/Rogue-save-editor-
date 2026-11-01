# Phase 20 — Read-only integration contract and gated release plan

**Status (1 November 2026): DRAFT PLANNING ONLY. No runtime integration, no game write, no branch merge, no new installable build.**

This document is owned independently of the Phase 17, Phase 18, Phase 19, and integration-QA feature branches. It records the technical contract and evidence requirements for combining the components. Re-check the heads and CI status of all four contributing branches at implementation time; never silently use an outdated ABI or claim that passing tests on one commit certify a different commit.

## Source ownership and merge separation

| Workstream | Existing branch / PR | Allowed role in Phase 20 | Not yet proven |
| --- | --- | --- | --- |
| Recovery | `feature/phase17-rollback-rehearsal` / PR #9 | Read-only GBA VC container authentication, Phase16 provenance and RAM-only rollback evidence | Installed-game write/restore, journal durability through power loss, AGB_FIRM re-save |
| Editing engine | `feature/phase18-offline-editors` / PR #8 | Validated **copied** H168 unwrapped save parser and guarded in-memory edit proposals | Exact ROM mapping for all fields, gameplay validation of output, installed-game use |
| UI | `feature/phase19-editor-ui` / PR #7 | 3DS input/menu controller, explicit opt-in copied-save viewer, permanently blocked real editing | Full physical test of latest import-enabled build, save export, live application |
| Independent QA | `feature/integration-qa` | Tests/review reports; independently exercised synthetic compatibility | Does not constitute hardware certification or approval of live writes |

Phase19 currently pins Phase18 **`596743d8eaa073759a736b5a8107db43f3a8c17b`**, while Phase18 continues changing. The pinned source is deliberately not equivalent to Phase18's newest branch HEAD. Any update requires explicit API diff, repin, reproducible CI and supported-provenance checks. Phase19 also has a previously confirmed stack-crash in an older CIA; never reuse that artifact.

## Independent QA PR #10 — finding disposition (1 November 2026)

The independent QA team identified these **open integration issues**. Severity reflects the missing release evidence, not a proven problem with the user's current game save.

| QA ID | Finding | Phase20 action / owner | Gate |
| --- | --- | --- | --- |
| QA-01 (HIGH) | Phase19 pinned an older Phase18 commit than QA's newer edited source | Phase19 owner must review the Phase18 API changes, pin an exact audited commit, rebuild native UI and re-run ASan/UBSan/3DSX/CIA smoke | Block bundled interface |
| QA-02 (HIGH) | Phase14 wrapper slot selection allowed skipped counters, while Phase18 checks internal raw bank adjacency | Phase17 branch now fail-closes on **nonadjacent non-wrapping wrapper counters**; synthetic tests added for 49/48, skipped/equal and near-rollover pairs. Re-pin QA to the verified commit and evaluate the independent raw bank domain separately | Block live restore; code correction pending its exact CI confirmation |
| QA-03 (HIGH) | No trusted 131072-byte raw edit to 263168-byte authenticated GBA VC container restoration boundary | No bridge permitted in Stage A. Require fully separate reviewed container patching/CMAC and rollback verification much later | Block installed writes |
| QA-04 (HIGH) | ROM ID and mapping evidence is caller-provided and not proof of exact game version | Phase18/QA must independently verify ROM identity and mapping. Unsupported edits remain disabled | Block new move/ability/nature/PC/item insertion |
| QA-05 (MEDIUM) | Raw save checksums do not cover all trailing bytes; format validation is not ROM identity proof | Preserve full **source SHA-256 optimistic lock**, reject stale view/copy, explicitly label version identity unverified | Block unverified writes |
| QA-06 (HIGH) | Production journal durability and complete receipt lifecycle untested | Preserve Phase17 RAM-only status; independent durable recovery design and expendable test before any live pilot | Block live restore |
| QA-07 (MEDIUM) | Phase19 build template comes from a moving upstream revision | Pin devkitPro build inputs to reviewed immutable versions, verify title ID and artifact hashes | Block release packaging |
| QA-08 (MEDIUM) | Unified Phase17/18/19 headers may not compile together | Require QA unified-header probe logs. Resolve type/namespace conflicts by read-only independent module boundary, never by weakening validation | Block Phase20 combined build |

**Interpretation:** Independent QA is a useful defect-finding gate; its pinned test results do not automatically apply to moving feature branch heads. All current PRs remain draft. A repaired issue is not closed until the exact source revision passes appropriate regressions, and hardware-dependent gates cannot close through host/emulator CI.

## Phase20 Stage A: safe read-only composition (first release candidate)

1. Start from a standalone Phase19 **3DSX**, not the older crashing CIA. Do not install any extra CIA to test Stage A.
2. Start in unmistakable fictional/demo mode. Nothing reads the installed Emerald Rogue title automatically.
3. On deliberate user action, read **only** the fixed separate offline copied raw file `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`, requiring exactly **131072 bytes**. A 263168-byte wrapped GBA VC container is **not** a raw import: refuse, never truncate or auto-convert it.
4. Parse via the pinned Phase18 **read-only** H168 path. Reject corrupt/checksum-inconsistent banks, stale/ambiguous counters, wrong size, unsupported layout and malformed identifiers; never silently fall back to demo values after an invalid import. Keep data provenance (`synthetic`, `copied-validated`, `unsupported`, `invalid`) explicit on every view.
5. Show validated numeric party, bag and money fields. Do not invent ROM-specific Pokémon/item/move names, levels or PC boxes; mark them unavailable unless independently proven. PC layout remains unverified.
6. The copied-save view has **no path to any edit, export, backup, restore, game archive or privileged FS write API**. All real editing buttons must refuse. UI previews must never mutate the source snapshot.
7. All essential UI routes must work without the **L shoulder button** (broken on target hardware). Verify D-pad/R/A/B/X paths; HOME/START exit behaviour must be unambiguous. The owner's Rosalina combo is START+SELECT.
8. Check stack and heap headroom, text wrapping, pagination, empty/huge input cases and clean return to HOME. The fix for the prior >63 KiB stack frame is a regression requirement.
9. Synthetic host tests and Azahar startup are required, but neither proves copied-save reading works on the user's real console. Mark Stage A hardware gate **PENDING** until the user voluntarily tests after protecting backups.

Stage A may be composed on an isolated Phase20 branch after Phase18/19 APIs settle and the QA team reviews the dependency diff. Do not merge working feature branches just to assemble a binary. Prefer explicit commit-pinned read-only dependency builds.

## Stage B: offline-only edit/export (separate, not enabled by Stage A)

- Phase18 must independently validate target H168 save variant, active dual-bank counters, Pokémon encryption/checksums, money/item layout and the exact IDs being edited.
- **Known mismatch:** publicly available EX-v2.1 SaveBlock1 offsets (+0x490 money, +0x560 bag) differ from observed target H168 v2.1.2 (+0x4A8 money, +0x578 bag). Do not derive unknown field positions by adding 0x18, and do not treat the public code as a verified match.
- Proposed modifications belong to a *different* immutable copied file, never the source or installed-title save. Require full-buffer before/after byte-diff proof, verified checksum reparse, independent output name, no overwrite, and explicit stale-input refusal.
- Existing numeric editing may be investigated with synthetic fixture only. New move IDs, arbitrary ability IDs, nature semantics, item insertion/upgrade-count, and PC editing remain blocked until matching-ROM, per-feature evidence exists.
- Phase20 UI must show per-field `available/unsupported/unverified` gates from the validated backend, not from a synthetic UI range alone.

## Stage C: installed-game restoration (separate physical safety gate)

**NOT AUTHORIZED.** Phase16 passed physical authentication, full-container immutable backup/journal and application-restart verification. Phase17 passed physical RAM-only rehearsal and separate new host tests. Those **do not** demonstrate real native GBA VC writes or power-loss resilience.

The existing native `FS_OPEN_READ` and `phase14::LiveWritesEnabled=false` must remain unchanged in Stage A/B. Do not directly link any live write interface into Phase20. Only consider an independent reviewed restoration pilot after all the following:

- Two *independent* untouched, readable full-container backups stored on separate media; backup/restore procedure reviewed and successfully rehearsed without touching valued progress.
- Signed provenance for exact installed title ID `0004000007D7BA00`, file/service identity, original wrapper length, both slots/counters, exact-source bytes, and console-bound CMAC; fresh revalidation immediately before any transaction.
- Durable, exclusive, never-overwriting journal creation with bounded writes, exact-length flush/readback, immutable receipt semantics and recovery handling for torn/partial/failed writes; independent verification of restart and power-loss scenarios on **expendable** test progress.
- Explicit refusal of foreign changes, stale journals, unverifiable records and unreadable storage; no silent replay, no automatic overwrite and no guessed recovery.
- Separate actual on-console game load, subsequent in-game re-save, controlled rollback and independent verification. A single RAM or emulator PASS does not close this gate.
- User review/approval for any first deliberate physical write. No unattended live mutation.

## Phase20 contract test inventory (implement before integration)

| Risk | Synthetic regression / static check | Required outcome |
| --- | --- | --- |
| Wrong file length | 0, 1, 131071, 131073, 263168, very large input | Reject with explicit error, no write or fallback |
| Unverified source | Wrong profile/version/title, corrupted bank, invalid counter/sector/Pokémon checksum | Invalid/unsupported, not editable |
| Changed source | File changed between capture and proposal; double-select | Refuse stale request |
| Unsupported field | PC, item insertion, nature, arbitrary ability, unverified new move | Controls blocked end-to-end |
| Preview safety | Repeated confirm/cancel/navigation, edit retries, switching from demo to imported copy | Actual imported data unchanged |
| Broken L hardware | Navigate party, bag, PC and every category without KEY_L | No essential action inaccessible |
| Buffer/stack | Maximum 6 party, 450 bag stacks, 14 boxes x 30 demo slots; empty data; oversize | No overflow, graceful pagination |
| Journal/race | Missing/corrupt/truncated/stale journal, receipt IO failure, foreign bytes, interrupted mock rollback | Fail closed, retain immutable original |
| Forbidden I/O | Review link map and source for FS_OPEN_WRITE to installed title, live writer toggles | No Phase20 live write path |
| Privileged service | Read-only host/emulator without console's real GBA service | Explicit unsupported, never fake authenticated |

## Execution order / release rules

1. Wait for stable, documented Phase18/Phase19 contract heads and QA compatibility report.
2. Build and test isolated **Stage A only**. Record exact source revisions and successful CI run IDs.
3. Request separate opt-in on-console read-only 3DSX test only when hardware is accessible and the complete SD backup is independently secured. The most recent physical console had a 1–2-minute delayed HOME start; treat any recurrence as a reason to stop testing.
4. Keep Stage B and Stage C behind their own pull requests and evidence gates. Never label Phase20 complete while any required real-world save safety step remains unverified.

**Absolute restrictions:** Do not publish private saves or copyrighted ROM data. Do not merge PR #7/#8/#9 or change the independent QA branch as part of drafting this document. Do not change live-write defaults. No Phase20 planning approval implies permission to write to the installed game.
