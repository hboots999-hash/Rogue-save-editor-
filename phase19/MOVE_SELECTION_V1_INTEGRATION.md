# Phase 19 move-selection UI v1 → Phase 20 integration (2 November 2026)

## Exact boundaries and status

UI source: `phase19/move_browser.hpp`, `phase19/main.cpp`; strict tests in `phase19/test_move_browser.cpp` and `phase19/test_move_replacement_ui.cpp`. Phase18 PR #14 head `ab77a81527ad1aa90c78c671c861c93cd223269c` still sets `move_v2::ApplyApproved=false` and `move_v1::capability().mayPreviewReplacement=false`. Phase20 PR #12 remains separate. These new Phase19 menus **never apply, preview, export, inject, restore or write a game save**. Screens are explicitly READ-ONLY/BLOCKED, including matching-PP catalogue entries.

## UI path

1. In standalone Phase19, **Y on Party** opens a bounded six-Pokémon picker. Up/Down selects Pokémon, A opens four existing slots, B exits. Empty/egg entries do not advance.
2. **Y on Pokémon detail** opens its existing four slots directly. Up/Down + A chooses an occupied slot; B cancels. The original Pokémon, move ID, known catalogue name (or `Old name: unverified ID`) and current PP are shown.
3. In alphabetical move catalogue, Up/Down selects, R/Right and X/Left page, Y opens a 6×5 D-pad keyboard, A enters read-only comparison. Rows have numeric move ID, metadata base PP only for ordinary matching-PP entries, `!` for conflicts/unavailable entries. Names longer than 15 characters display `...` and their full name appears on review.
4. Comparison → release buttons and **A** → `ConfirmOne` → release and **A** → `ConfirmTwo` → release and **X** → **BLOCKED**. B cancels on comparison or either confirmation. Holding A or X never skips stages, and the only final action is an explanatory refusal—no Apply API or export can be reached.
5. START alone exits on release; START+SELECT is reserved in either press order for Rosalina; L is not required. D-pad/R held navigation repeats; A/B/X/Y never auto-repeat.

## Versioned read-only integration object

`MoveSelectionIntentV1` has `version=1`, party index [0,5], move index [0,3], species, original numeric move ID and PP, proposed numeric ID and metadata base PP, source save counter and SHA256, and ROM-catalogue *claimed* SHA256. `describeMoveSelection(browser,snapshot,out)` populates this object only for an occupied original slot and a distinct, ordinary matching-PP candidate; it also rejects duplicates in other move slots. **A valid descriptor is informational, never permission.** `MoveReplacementGateV1::mayApply()` is hardcoded false; all `productionPreview`, `offlineExport`, and `installedWrite` constants are false. Metadata checksum agreement does not authenticate the installed ROM.

Phase18 move-v2 requires more evidence than this UI DTO can provide: immutable original raw-source fingerprint, bank counter, PID/OT/species/checksum, original encrypted move and PP, PP-Up bonus byte, exact ROM metadata, PP mode and compatibility evidence. Any future coordinator adapter must recreate Phase18's independently checked `move_v2::Request` with `move_v2::begin(snapshot,raw,partySlot,moveSlot,req)`, and `move_v2::choose(authenticatedCatalog,req,newId)` before invoking a **future audited production capability**. **Current `move_v2::preview` always blocks; `move_v2::verify` always returns false. Do not bypass these checks or use synthetic fixtures as an allowlist.**

## Phase20 adoption instructions

- Pin/import the reviewed Phase19 `move_browser.hpp` (and native UI rendering changes), then update Phase20 `moveTop` to render `MovePage::Party`, `Review`, `ConfirmOne`, and `ConfirmTwo` rather than routing unknown states to the generic Notice branch. Include original current move name if present in verified metadata, otherwise show numeric ID and an explicit unknown-name warning. For PP show **metadata base PP** only when both tables agreed; 40 conflicts and dummy/special/unknown entries must remain unavailable.
- In `phase20/main.cpp` preserve its already-fixed context routing: **Y on idle Pokémon detail** opens browser, **Y elsewhere** preserves the existing multi-edit queue, and browser Y edits search. Do not hijack **Y on Party** in Phase20 because the batch queue already uses it: instead expose the six-Pokémon picker through an explicit `Browse party moves` menu choice or separate nonconflicting shortcut. Standalone Phase19 uses Y on Party solely to exercise the picker.
- Update the native browser input to call `moveView.releaseApproval()` only on a frame when neither A nor X is held, and ensure `MoveNavRepeat` never repeats A/B/X/Y. Only fresh key presses can progress the comparisons; START+SELECT remains reserved. Clear and reset the browser on import/source change, leaving existing money, bag, IV, EV, PP and batch editor pathways intact.
- Phase20 `LocalMoveProvider::read()` owns optional, exact 14,040-byte SD metadata validation. Phase19 only receives `const MoveCatalog`; never bundle ROM bytes/CSV or fall back to synthetic IDs when a real imported file lacks authenticated metadata. The catalogue can have 935 records and our UI supports up to 936, without new allocations each frame.
- CI after adoption: strict host/ASan/UBSan for six slots × four move positions, malformed and missing metadata, 935/936-record sorted search, old/new names and PP, stale SHA/counter, repeated openings, B at both confirmation stages, A held/repeated, final X held, START/SELECT both orders, and all five pre-existing offline/batch editor operations. Run native ARM11 stack-frame report, Azahar screenshots and read-only output canaries; physical hardware is a separate release gate.

## Physical acceptance (PENDING)

Only test new **3DSX**, not experimental CIA, and start with synthetic data and no imported save. Inspect Party→Y→party slot→existing move→search Fiery Dance→comparison→two confirmations→BLOCKED; verify B cancellation at each screen and START+SELECT. Do not install or restore a draft, edit private saves, or assume emulator evidence proves real 3DS compatibility.

**This is a complete selection-and-review UI prototype, not a working replacement engine.** The Phase18 approval gate is an unresolved, independently audited dependency.
