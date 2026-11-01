# Phase19 → Phase20 safer editor: adoption and physical acceptance

**1 November 2026.** Phase19 owns UI-only helpers. Phase20 PR #12 remains separate, draft and unchanged. Reviewed Phase20 editor files at `2bbea3539e15cc30a03ed72a78a741bbd06cdf76`; user-reported Azahar 40001→40002 draft test is synthetic evidence, NOT real 3DS/gameplay approval.

## Shared UI files to adopt

- `phase19/offline_edit_presentation.hpp`: `emitOfflineFields`, `emitOfflineReview`, `emitOfflineResult`, `OfflineReviewSession`, `OfflineApprovalGate`.
- `phase19/offline_review_ui.hpp`: `ReviewTarget`, `reviewDisplayMatches`, `formatReviewTarget`, `safeDraftBasename`. Display-only checks; NOT authorisation to export.
- `phase19/screen_text.hpp`: allocation-free `forEachBoundedLine` and backward-compatible `forEachWrappedLineBounded` cap multi-KB errors without scrolling essential A/B controls off-screen.

## Exact Phase20 coordinator changes

1. **Field screen:** In `phase20/main.cpp::offlineTop` when `EditStage::Field`, replace generic Stat 1–6 with `emitOfflineFields(display, kind, partySlot, selectedField, outputLine)`. Map IV/EV/PP to `ReviewField`. It highlights exactly one row, identifies stats HP/Atk/Def/SpA/SpD/Spe, displays numeric PP move IDs, and rejects egg, empty party, missing field/slot. Do not show a fake write-ready option if it returns false.
2. **At first confirmation:** build a `ReviewTarget` using `OfflineFlow.choice`, actual imported `Demo`, original/proposed values, `CopyObservation.activeCounter` and `sourceSha256`. Set `itemId` for existing bag quantity and `moveId` for existing move PP only after bounds checks. Call `OfflineReviewSession::begin(display,target)`; on failure refuse without exporting. Use `emitOfflineReview(display,target,false|true,outputLine)` on both confirmations: source `imports/00000001.sav`, target identity, species ID, stat/move or bag ID, before/after, EV totals, installed game UNCHANGED.
3. **Distinct-key approval:** the current native Phase20 UI accepts repeated A, reaching SD export after confirmation 2. Update BOTH input handling and the rendered labels, not just the text. During confirmation stages consume each HID frame's held A, X, B and SELECT via `OfflineReviewSession::poll(display,aHeld,xHeld,bHeld,selectHeld)`. First fresh A yields `Next`; *release both A and X*, then a fresh X yields `Approved`, which is the ONLY UI event allowed to call Phase20's existing independently verified exporter. B cancels both stages. A held, overlapping A+X, SELECT, stale digest/values and rapid repeat must not approve. Do not also forward the consumed key to `takeOfflineKey`! Reset session on cancel, result, successful import, Import→X reset, source change or exit. START alone exits on release; START+SELECT still opens Rosalina in either press order.
4. **Result filename clarity:** After successful independently verified export use `OfflineResultNames{source,backupName,draftName,true}`; run `validatedResultNames` and `emitOfflineResult`. Display separate full backup/draft BASENAMES, fixed original source, **one independent edit per draft**, NOT game-tested, DO NOT RESTORE, and A/B return. On fail/exception, unknown path, truncated name or partial output show EXPORT UNVERIFIED/BLOCKED, preserving uncertainty. Do NOT concatenate names into `OfflineFlow.result[256]` where truncation can obscure provenance. Bound long errors on screen.
5. **Source/selection resets:** Phase20 currently resets shared State but not necessarily its separate `OfflineFlow`/`CopyObservation` on each Import→X and successful re-import. Explicitly reset all related edit state together. A failed re-import must preserve old visible snapshot and clearly say OLD COPY STILL DISPLAYED. Bounds-check `s.demo.bag[f.choice.slot]` in Phase20's **renderer**, not just on export; likewise party/PP field before rendering. Never treat UI review checks as edit authorisation.

## Phase20 additional negative tests required after adoption

- All five classes: B from category/field/value/ConfirmOne/ConfirmTwo; rapid A hold, X too early, A+X overlap, SELECT-held, all keys released before second approval, accidental double-submit, source disappears between stages.
- Empty party/bag, egg, absent occupied bag stack, zero move ID for PP, invalid selected index/field, EV total 510, invalid path/name, stale SHA/counter, repeated imports/exports and reset to synthetic demo.
- SD-full/partial writes, failed close/flush, out-of-memory export and no false success; source original remains unchanged, incomplete separate output needs manual review. Include strict, ASan/UBSan, native 4-KiB stack guard and Azahar snapshots.

## Memory and release risks

Earlier experimental CIA ARM11 crash stemmed from a massive stack frame before gfxInit; corrected Phase19 static Demo/compact State remain necessary. Reviewed Phase20 `.su` from run #41338410580: max frame **1064 bytes**, main **232 bytes**, below 4096-byte compiler limit. Frame statistics DO NOT bound several 128-KiB heap vectors or SDL SD durability; physical stress/export tests are pending. No installed GBA VC game I/O, injection, CMAC, restore or live writes are approved.

## Restricted physical 3DSX acceptance checklist (PENDING)

**Stage A: No import.** Keep independent GodMode9/Checkpoint backups. Launch only updated experimental 3DSX via Homebrew Launcher, NOT the experimental CIA. Confirm top/bottom displays, scrolling and caps, L-free R/Right, X/Left, A/B, START-alone release exit, START+SELECT Rosalina in either order. Open Import with no file; expect missing-file refusal, unchanged demo and no outputs.

**Stage B: READ ONLY expendable copy.** After Stage A, use only independently generated expendable exact 131072-byte H168 data in the fixed imports folder. Inspect party/bag/money; verify empty/unsupported PC notices, repeated imports, invalid-copy refusal, no unsafe editing/restore. Never put original gameplay save or truncate 263168-byte authenticated VC backup.

**Stage C: Expendable offline export only, separately authorised.** After Phase20 adopts new UI and passes its own tests, with independent explicit user approval and plenty of SD space, edit a disposable synthetic file, verify source immutable and distinct backup/draft names/bytes, never import the draft into installed game. Stop on any memory warning, SD error, corrupted display or ARM11 crash.

Emulator and host success are not physical-hardware or gameplay compatibility proof. PR #7 and PR #12 remain DRAFT and unmerged.