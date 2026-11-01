# Phase 19 — PKSM Rogue Edition, experimental 3DS read-only viewer

**Current status (1 November 2026):** updated standalone 3DSX builds a fictional editing demonstration and an **opt-in read-only viewer for a separately prepared 131072-byte H168 SD save copy**. No live Nintendo 3DS game archive is opened. No save, backup, draft, item, Pokémon, money or PC value is ever written or restored. The new copied-save-integrated version **has not** been tested on the user's physical handheld; earlier real-console success was for the previous corrected synthetic-only 3DSX. **PR #7 stays DRAFT.**

## Features

- Two-screen libctru console UI: Home, Party, PC, Pokémon detail with numeric move IDs **and PP**, fictional IV/EV/Move/PP previews, read-only Ability/Nature, Bag, Money, Backups, Recovery and copied-SD Import.
- The native executable **starts in fictional DEMO mode**. Fictional proposals can be adjusted and double-confirmed, but finish **BLOCKED** and do not modify even the sample snapshot.
- Seventh Home menu entry: **Inspect SD copy (read-only)**. Press A to inspect only `sdmc:/3ds/PKSM-Rogue/imports/00000001.sav`. The file must be an **independently prepared, unwrapped exact 131072-byte H168 copy**; missing, corrupt, wrong-sized and 263168-byte authenticated GBA VC wrappers are refused. Failed reads leave the visible data unchanged. Do not truncate or overwrite original saves.
- Imported party, money and existing item quantities are numerical and **READ ONLY**. Party nickname/level, exact ROM move/item names and actual PC layout are unverified: show Species N, Lv? and Unsupported PC, not fake values. Structure validation is not proof of exact ROM identity or game-playability.
- Native copied-save imports disable proposals using both snapshot origin and a **separate session-edit gate**. Import->X explicitly resets to a full fictional snapshot, clearing all previously imported bytes in inactive slots. No real-data editor or Phase18 draft/export API is linked.

## Controls — **L shoulder NOT REQUIRED**

| Location | Controls |
| --- | --- |
| Home and lists | D-pad Up/Down select; A open; B back |
| PC boxes / Edit category | **R or D-pad Right** next; **X or D-pad Left** previous |
| Bag pages (6 entries per page) | **R or D-pad Right** next; **X or D-pad Left** previous |
| Fictional proposal only | Up/Down ±1, Left/Right ±10, X reset, A advance to BLOCKED, B cancel |
| Exit | **START alone, released** |
| Rosalina shortcut | **START + SELECT**, either order, is reserved and must not exit the app |

L continues to function as an optional previous control but is not needed for any essential flow. On the top screen, all status/recovery guidance wraps at word boundaries; the lower screen shows context-oriented controls.

## Build, verified checks and limits

Workflow: `.github/workflows/phase19-editor-ui.yml` builds experimental 3DSX/ELF using devkitPro and packages a separate demonstration CIA Title ID **000400000F73A500**. Host strict-warning and sanitizer suites cover all menu categories, confirmation cancellation, unsupported states, fake-data isolation, L-free navigation, 450-item pagination, Rosalina chord and safe memory handling. Phase18 generated dual-bank H168 fixtures test empty party/bag, stored values, corrupt sectors and read-only stream canary. Native linked-ELF symbol audit excludes Phase18 proposal/export interfaces. Azahar screenshots of boot, selected menus and exit attempts are best-effort **emulator** evidence, never proof of physical 3DS compatibility.

A previous experimental CIA caused an ARM11 stack crash; the corrected demo 3DSX later passed actual hardware tests. The **new updated 3DSX** must be separately validated when the user's console is available. Do **not** install the previously crashing CIA as a shortcut.

### When 3DS is available again

1. Preserve verified backups and confirm Emerald Rogue still starts.
2. Download **the latest Phase19 3DSX** from the newest passing GitHub Actions run; back up the previous working 3DSX and replace it through the Homebrew Launcher path. **Do not install the experimental CIA.**
3. First launch with **no copied save in the import folder**. Verify demo menus, R/X navigation, and the seventh Import option showing missing-file status, then B back and standalone START release to exit.
4. Only if an independently verified, unwrapped 131072-byte H168 **copy** is available, place that copy at the fixed SD path and inspect the numeric party, bag and money read-only. **Never** modify installed Emerald Rogue or trim its 263168-byte VC backup.

Further details: [READ_ONLY_IMPORT.md](READ_ONLY_IMPORT.md), [INTEGRATION_CONTRACT.md](INTEGRATION_CONTRACT.md), [PHASE20_INTEGRATION_CONTRACT.md](PHASE20_INTEGRATION_CONTRACT.md), [PHASE_HANDOFF.md](PHASE_HANDOFF.md), [HARDWARE_TEST_MATRIX.md](HARDWARE_TEST_MATRIX.md).

---

## Historical engineering notes (some statements superseded by the current overview)

## 1 November 2026 — Phase 19 adapter/UI integration iteration

This continues draft PR #7. The UI now consumes the bounded, read-only snapshot model defined in phase19/adapter.hpp. It still instantiates ONLY mockSnapshot() in the native executable. Future read-only provider callbacks may supply Phase 18-validated data but cannot enable any edit. No live game save, SD export or restoration API is present.

New features: explicit Ready/Empty/Unsupported/Invalid/Unverified states; sample PC browsing up to 30 slots per box, displayed six rows per page; Pokémon move PP display and mock proposals; clearer 3DS-facing ability-slot/nature/new-move-ID restrictions; item quantity 1–999; numeric edit clamping, no-op rejection, cancellation to a prior view, and two blocking confirmations. Unsupported PC edits, ability changes, nature mutation, unknown move IDs and bag item insertion are intentionally unavailable.

Backup status says NOT CHECKED IN APP: Phase 16 hardware evidence does not mean this isolated preview can locate verified backups. Recovery has concise on-screen messages for missing saves, unknown layout, checksum failures, backup absence and disabled restoration. All actual-save snapshots are non-editable. Invalid input and missing provider data never fall back to fictional values.

Host tests include every control route, cancel, invalid/no-op changes, an example read-only provider, unsupported status and PC pagination. CI uses strict C++17 and ASan/UBSan, native devkitPro, CIA packaging and optional Azahar smoke. These are not physical Nintendo 3DS hardware tests; those remain outstanding. Details: phase19/INTEGRATION_CONTRACT.md.

## 1 November 2026 — Phase18 READ-ONLY copied-save integration

Phase19 can now display validated *user-supplied unwrapped copied* H168 save data, without opening the installed GBA VC game. The seventh main-menu entry, **Inspect SD copy (read-only)**, triggers a deliberate one-time read from the fixed offline SD import path. The native app **still starts with the same fictional Pokémon** and never scans for game saves automatically. The UI can show active bank money/counter, numeric party species, IVs/EVs, numeric moves/PP, effective nature index, two-bit ability slot and existing bag ID/quantities. Unverified real Pokémon name/level shows Species ID/Lv?. Real PC storage remains unsupported; all actual-data edits are blocked.

The parser is compiled from the **pinned Phase18 PR #8 source commit 596743d8eaa073759a736b5a8107db43f3a8c17b**, checked out separately by CI, without merging or changing Phase18. Invalid, wrapped 263168-byte or wrong-sized file attempts do not replace the displayed data. The source folder is SD-only read-binary; this app cannot write live saves, drafts, backups or restore. Full details and safe hardware testing instructions are in READ_ONLY_IMPORT.md.

**No real imported save has yet been tested on hardware.** The previous physical console passes apply to the older synthetic UI build, not this new import-integrated executable. Revalidate the updated 3DSX safely before any copied-save experiment. The draft PR must stay unmerged.
