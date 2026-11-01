# Phase 19 → Phase 20 UI and restricted 3DSX hardware handoff
Date: 1 November 2026

Scope: **review only** of draft integration PR #12 at Phase20 source commit `5734735c93f74de3ac2f313488989c28c39a49e1` (same `phase20/offline_edit_v1.hpp` / `phase20/main.cpp` at later 2bbea353). Shared Phase19 changes are on branch `feature/phase19-editor-ui` and PR #7. Neither integration PR #12 nor Phase18/17 was modified by this handoff.

## A. End-to-end offline edit journey — source review

Phase20 imports only a separately copied, unwrapped, exact **131072-byte** H168 file from `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`. Its offline editor has a separate `OfflineFlow`; only a `CopyObservation` matching imported `Demo`, source SHA-256, counter, version, count, and disabled installed-write/restore flags enables entry. The UI re-reads the source and verifies a distinct immutable backup and draft on export. **No live-game archive, CMAC signing, title write, save injection or restoration is enabled.** This is a synthetic CI and source review, *not* verified gameplay or 3DS disk durability.

| Journey | Phase20 path and confirmed host test | Usability / accidental-input recommendation |
| --- | --- | --- |
| Money | Imported Money, A → Value (0–999999), Up/Down ±1, R/Right ±10, X reset, A/A/A through two confirmations to export. Host exports copied money and rechecks raw bytes. | Show **money before → after**, source filename and distinct draft intent before final approval. Long changes from 0 to large numbers are cumbersome; only add faster increments with explicit display and separate bounds tests. |
| Occupied bag stack | Imported Bag/Items, six-row page, A → Value (1–999), double-confirm. Host verifies exact existing item and quantity. New item IDs blocked. | Show **item ID + occupied bag position + before/after qty** on BOTH confirmation screens; distinguish absent/empty inventory from missing data and guard direct bag dereferences in renderer. |
| Party IV | Imported party detail → A → Category IV → Field (six stats) → Value (0–31), confirm twice. Egg edits blocked. Host exports IV draft; source species/checksum/IV expectations rechecked. | Display **party slot, species ID, named stat HP/Atk/Def/SpA/SpD/Spe**, original/target value on field, value and both review screens. Names and level from exact ROM are unverified. |
| Party EV | As IV, EV category; per-stat 0–252 and total ≤510. Host creates and verifies EV draft. | Display **EV total before → after**, including room left, alongside stat name; clarify a value capped by 510 limit. Never silently wrap when going beyond a limit. |
| Existing move PP | Imported party → Category PP → one of four slots; empty move slots blocked; 0–64. Host exports existing-PP draft, preserving move ID. | Show **slot number, numeric original move ID, old/new PP** on confirmation. Do not imply the numeric move name is verified. |

All Phase20 editing state is synthetic/SD-copy-only: `takeOfflineKey()` intercepts only while `offlineVisible()` is true. **L is not necessary**: R or Right advances categories/pages; X or Left backs up; Up/Down selects or adjusts; A proceeds; B cancels or returns. R also increases quantity by 10 on numeric Value screens. The native loop recognises standalone START on *release* and reserves either-order START+SELECT for Rosalina.

## B. Specific integration changes requested of Phase20 owner (do not cherry-pick blindly)

**Priority 1 — avoid misleading or unsafe confirmations.** Current `phase20/main.cpp::offlineTop()` prints only edit type and a number pair on ConfirmOne/Two; it does **not** reliably identify the party slot, species, item ID, stat or move ID. Pass a fully resolved identity summary into the two confirmation screens. Use a second distinct confirm control (e.g. X on ConfirmTwo) to reduce accidental repeated A; if retaining A, add a visibly separate second-stage prompt, require fresh key-down edges and test rapid A/B/START sequences. Do not write on navigation input alone.

**Priority 1 — bound result text and show exact filenames.** Current Phase20 result uses a single `OfflineFlow.result[256]` formatted with backup and draft paths; `snprintf` can truncate part of a filename. `phase20/main.cpp::wrappedNotice()` uses *unbounded* `forEachWrappedLine(...,31,...)`, which can scroll **A/B return guidance** off the console. Phase19 now exports `forEachBoundedLine(text,31,11,emit)`, reserving visible prompts and showing a truncation indicator. Integrate that helper in Phase20; store/display backup and draft **basenames separately** with per-line clipping and a clear success/failure indicator. A failed or interrupted export must never claim a verified draft, and may have **partial separate files requiring inspection**.

**Priority 1 — guard stale rendering and source transitions.** In `phase20/main.cpp::offlineTop()` the bag value branch directly indexes `s.demo.bag[f.choice.slot]` without a local bounds check. Guard **every render path**, not only `requestForCopy()`, because a state/source transition can invalidate the displayed index. On Import→X synthetic reset and Import→A successful re-import, explicitly clear `CopyObservation` and `OfflineFlow` **before** switching source; restore the new observation only after successful independent validation. Keep the previous visible snapshot after a failed import and clearly say that the previous copy is still displayed. Also clamp party/PC/bag field indices after each source change.

**Priority 2 — specific navigation and language.** Current Field view calls six IV/EV rows `Stat 1` etc. Use HP, Atk, Def, SpA, SpD, Spe; make move slot and numeric move ID visually distinct. An empty party or bag should render `EMPTY — nothing to edit`; unsupported PC, species/ability/nature changes, added items and restoration should be disabled with reasons rather than showing an actionable edit CTA. Display fixed source path on Import, source basename on confirmation, and backup/draft path **only when the exporter actually supplies verified names**. Clarify that each exported draft is **one independent edit from the original copied import**, not a cumulative change to the last draft.

**Priority 2 — resource and recovery limits.** Phase20 uses several independent 128-KiB raw buffers/vectors for SHA-locked readback and offline export. It catches `bad_alloc` / standard exceptions in export and main import, but actual 3DS heap pressure, partial file effects, failed SD writes, close/flush errors and repeat-export numbering require expendable-data hardware evidence. Add an explicit `busy/export in progress` guard; disallow re-entry until the complete exporter returns. Preserve unmodified import and immutable backup even on failure. Do not enable live title I/O.

**Priority 3 — test negative controls.** Phase20's host suite verifies all five draft classes and synthetic source-byte preservation, one money cancellation at both confirmation stages, some invalid indices and simulated allocation/exception. It does **not** fully exercise independent stage-by-stage cancellation for **all five** choices, rapid double taps, empty party/bag UI, malformed future flow state, missing move slots on the actual UI, or multiple repeated imports. Add those to Phase20's own `test_offline_edit_v1.cpp` and integrate native ARM11 stack-report and sanitizer regressions. The Phase19 shared UI boundary now clamps out-of-range states separately and has bounded-error rendering tests.

## C. Phase19-owned shared component changes

- Added allocation-free `forEachBoundedLine(raw,width,maxLines,emit)` for fixed 3DS screen budgets. Explicit `[MORE TEXT OMITTED]` marker on overflow; no recursion, heap allocation or unbounded screen scrolling. Phase19 native notices use 11 lines maximum. Phase20 must **opt in** to this new API; its current main.cpp still uses the legacy unbounded helper.
- Saturated `lastVisible()` against `UINT_MAX` wraparound, and extended 12,000-character error, newline, empty/null and pagination regression tests.
- Phase19 selector guards (now on the branch): party, PC and bag counts clamp to array sizes, invalid selected Pokémon return an inert placeholder, and original/maximum/proposal checks deny stale indices. They reduce crash risk but must not replace separate Phase20 render and exporter guards.
- The 3DS L button remains optional; all important controls are reachable by R/Right, X/Left, A, B, Up/Down. START+SELECT reserved for Rosalina.

## D. ARM11 memory and crash review

An **earlier installed experimental CIA** crashed with ARM11 data abort; binary symbolication showed an oversized main-thread stack frame preceding `gfxInit`. Phase19 subsequently moved the large `Demo` to static storage and made `State` a small non-owning view. Its native GCC stack guard is `-Werror=frame-larger-than=4096 -fstack-usage`; a historical build caught Phase18 research-PC's 37,104-byte stack frame, and the native UI excludes that header. The last audited Phase19 native report had a largest function frame of ~840 bytes and main ~232 bytes. Phase20's **separate** CI uses a native stack bound, but its actual heap peak, libctru SD flush/close durability and full copied-export failure paths are still unmeasured on a physical 3DS. Neither Azahar success nor a non-release CIA package proves hardware stability.

## E. Restricted physical 3DSX acceptance checklist — ONLY when available

**Do not install the experimental CIA or use the installed-game save. Do not enable save injection or restoration.** Keep GodMode9/Checkpoint independent original backups safe. The current owner has no physical console access, so the following are PENDING rather than PASS.

1. Launch **new Phase19 3DSX via Homebrew Launcher**, with **no** import file. Confirm clean startup, both screens readable, B return, L-free R/Right and X/Left, and standalone START release exits.
2. Relaunch and hold **START+SELECT** in either order. Confirm Rosalina opens without Phase19 exiting, then close Rosalina normally. The L shoulder button must not be required.
3. On the fixed Import screen press A **without** `/3ds/PKSM-Rogue/imports/00000001.sav`; expect missing-copy notice, no crash, no file creation, unchanged demo data. Confirm B returns.
4. For a *separate independently validated expendable* 131072-byte unwrapped synthetic save copy **only**, check party/bag/money read-only display, correct EMPTY and invalid-copy refusal, a repeated import, L-free navigation, and that no real-source edit proposal is accessible in Phase19. Do not put a 263168-byte authenticated VC container in this path or truncate it.
5. Phase20 **offline export** requires separate explicit approval and disposable media/test assets. Before undertaking it, verify sufficient SD space, untouched input, immutable backups, exact distinct draft filenames/readback, and a documented recovery path. Do not use the owner's personal gameplay copy or live GBA VC title.
6. Restart the Homebrew Launcher/game separately and confirm the installed Emerald Rogue game and its save still launch normally. Stop immediately for any ARM11 crash, HOME Menu instability, SD errors or mismatched metadata. Preserve crash dump; do not repeatedly launch a suspect CIA.

## F. Integration/release boundaries

Phase19 PR #7 and Phase20 PR #12 are still separate drafts. No PR merge, installed-title writer, CMAC, raw-to-authenticated-container conversion, PC editing, save injection, restoration authority or private save uploads. Phase20's own stack/sanitizer/emulator results do **not** waive the physical 3DS gate. Remain read-only at the integration boundary until exact matching-ROM metadata, save durability and independent recovery are proved.

See `phase19/PHASE20_INTEGRATION_CONTRACT.md`, `phase19/QA10_REMEDIATION.md` and the PR #12 test/CI report for the versioned read-only contracts.
