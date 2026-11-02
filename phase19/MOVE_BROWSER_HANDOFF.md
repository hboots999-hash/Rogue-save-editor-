# Phase 19 — READ-ONLY searchable move browser (v1)

## Status and provenance

Phase19 PR #7, independent of Phase20 PR #12 and Phase18 move research PR #14. This is a **working 3DSX interface prototype** using 16 deliberately fictional records with IDs **9001–9016**. It does not bundle Emerald Rogue ROM bytes, a copyrighted move table, a CSV, or a trusted real-ROM name/ID mapping. `Fiery Dance` in the synthetic fixture has **fake ID 9001**, **not** its installed-game ID.

PR #14 documents **936 indexed ROM records**, of which **807 ordinary entries** had agreeing PP in the two observed compiled tables and **40 ordinary entries** had conflicting PP. These numbers are reported research results, not a reviewed production catalogue in Phase19. The browser supports up to 936 records and tests a synthetic set containing exactly 807 candidates, 40 conflicting-PP entries and 89 special/dummy/unknown entries; **only the future authenticated provider** can supply authentic IDs and names. The full actual 936 records are not currently bundled.

**Move replacement stays DISABLED for every record.** Even a matching-PP item marked `PP MATCH - REVIEW ONLY` is not approved for export, and a provider stating `ExactRomReviewed` does not grant application/edit authority. The production move-replacement API in Phase18 PR #14 is still blocked. No live save access, SD export, CMAC or restore has been added to Phase19.

## Versioned provider interface

`phase19/move_browser.hpp`: `MoveCatalogApi=1`, `MoveCatalog{version,origin,count,records,romSha256}` and `MoveRecord{id,basePP,evidence,name[40]}`. `records` is a **borrowed const pointer** to storage owned by the provider for the entire browsing session. The UI never parses a runtime CSV, reads a ROM, calls Phase18's editor, or allocates large per-frame result arrays. Maximum 936 records; each ID must be nonzero and unique; each name must be nonempty, ASCII and NUL-terminated within 40 bytes; PP <=64, candidate base PP 1–64. Duplicate/bad/oversized datasets fail closed. UTF-8 glyph rendering beyond ASCII requires a separately reviewed font/encoding layer. The version must be incremented if semantics change.

`MoveEvidence`: `Candidate` (two PP tables agree but still **review only**), `ConflictingPP` (UNVERIFIED), `Special`, `Dummy`, `Unknown` (all UNAVAILABLE). Never infer the special entries' PP or turn them into candidates. Display `basePP` only on `Candidate`; for the others display `Base PP: NOT VERIFIED` to avoid trusting conflicting/unknown values.

The browser sorts 936 `uint16_t` indices (about 1.9 KiB) alphabetically with a case-insensitive ASCII comparator and stable ID tie-break. It searches via bounded, case-insensitive substrings, derives results on demand without a large heap allocation, and paginates six rows at a time. Browser state is **static**, not a large ARM11 main-thread stack local. All raw metadata remains in immutable provider-owned storage.

### Event/control map — no L button required

| Context | Controls |
| --- | --- |
| Party Pokémon details | **Y** opens synthetic move browser for that party Pokémon; PC editing remains unavailable |
| Existing moves | Up/Down chooses slot 1–4; **A** opens catalogue; **B** exits |
| Alphabetical catalogue | Up/Down one row; **R or Right** next page; **X or Left** previous page; **Y** opens search keyboard; **A** opens read-only review; **B** returns to slots |
| On-screen search keyboard | D-pad moves 6×5 alphabet grid; **A** types; `<` deletes, `!` applies; **X** backspace; **Y** applies; **B** cancels without changing previous search |
| Review screen | Existing slot/old move ID/old PP vs candidate name, fake candidate ID, research PP, evidence status; **A** shows BLOCKED notice, **B** returns |
| Blocked notice | **A/B** returns to catalogue, no mutation |
| Global | Standalone START *release* exits; **START+SELECT** remains Rosalina in either press order |

Long move names use a shortened alphabetical-row label but are shown fully on the separate review page with bounded word wrapping (at most three 30-column lines). Entries are distinguishable by numeric IDs and a `!` unverified indicator. The lower screen says `SYNTHETIC IDS, NOT ROM DATA` and `NO MOVE REPLACEMENT ACTIVE`. When browsing an imported real Pokémon, the party data is real read-only **but the move list is still synthetic**; this must not be mistaken for ROM proof.

## Phase20 coordinator adoption requirements

1. Phase18/PR14 must supply a reviewed **immutable versioned ROM catalogue**, checked for exact-ROM SHA identity, table provenance and every move's original IDs/names/base PP evidence. Do not paste ROM bytes into the repo; produce clean metadata through approved tooling with source fingerprint and traceable extraction methodology. PR14's research-only `move_v1::preview()` is blocked and **must stay blocked** until independent production evidence/review.
2. A future Phase20 integration may provide a `const MoveCatalog` and a verified party snapshot but must not use selection as write authority. `MoveBrowser` returns a **read-only candidate**; a separate independent exact-byte, source-sha, party identity, original move slot, PP/PP-Ups and game compatibility validator is required **before** any offline draft. Do not enable Apply, export, injection or installed title access as part of merging this UI.
3. Existing Phase20 money, bag, IV, EV, existing PP and multi-edit queue menus must remain unchanged. Move browser uses **Y only on Phase19 Pokémon detail** and a separate UI overlay/state; Phase20 also uses Y to start a multi-edit queue on some screens, so the coordinator must choose a context-specific shortcut or new dedicated menu option, explicitly test priority, and never silently intercept the existing Y queue flow.
4. For the requested eventual Volcarona→existing move slot→Fiery Dance workflow: open party Volcarona, select existing move, search `FIERY DANCE`, review original move ID and PP versus authenticated candidate ID and base PP. **At present final step says BLOCKED and writes nothing.** User approval plus research and safety gates would still be required before separate offline draft export.

## Automated and physical tests

`phase19/test_move_browser.cpp` checks max 936 entries, 807/40/89 synthetic classifications, alphabetical order, six-row page bounds, case-insensitive matching, full D-pad keyboard input of `FIERY DANCE`, backspace/search cancellation, long names, wrong catalogue versions/IDs/PP/duplicates and missing party, and permanent read-only refusal. Strict C++17, ASan/UBSan, native `-Werror=frame-larger-than=4096`, separate experimental CIA packaging (NOT for installation) and optional Azahar startup/browser screenshot evidence are exercised by Phase19 Actions.

**Physical 3DSX test PENDING**. The earlier synthetic-only 3DSX passed real hardware tests, but this new move-browser executable has not. When the user's console returns, launch **3DSX** with no imported file; check Party→Pokémon→Y→slot→catalogue→search→Fiery Dance→review→BLOCKED, B return, R/X paging, START exit and START+SELECT Rosalina. Preserve installed Emerald Rogue and independent backups, never install the experimental CIA or use the gameplay save to test changes.