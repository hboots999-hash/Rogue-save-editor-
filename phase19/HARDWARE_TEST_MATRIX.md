# Phase 19 experimental 3DS input and recovery test matrix

**Physical console status: NOT RUN.** Host key-equivalent tests and Azahar smoke are not substitutes. This checklist is for a separate user-controlled manual test of the synthetic-only app. No personal save or game ROM is needed; this app does not open one.

## Before hardware testing

- Verify independent, recent original Emerald Rogue GBA VC backups exist externally, and confirm the console launches normally.
- Prefer standalone Phase19 3DSX via Homebrew Launcher first. If choosing the experimental CIA after the 3DSX check, verify title ID 000400000F73A500; it is NOT the Emerald Rogue title 0004000007D7BA00 or the prior preview 000400000F73A400.
- Never inject a draft, replace a game save, format SD, or delete any original backup as part of this test.
- Press START to exit at any point. If HOME Menu instability occurs, stop and report the crash screen rather than retrying blindly.

## Manual button matrix (all results should be demo-only)

| ID | Starting view | 3DS buttons | Expected visible result | Observed |
| --- | --- | --- | --- | --- |
| 01 | Home | A | Party: three sample Pokémon, status READABLE | NOT RUN |
| 02 | Party | Down, A | Swampert detail: IVs, EVs, four moves, ability slot and nature | NOT RUN |
| 03 | Pokémon detail | A, A, Down, A, A, A | IV proposal reviewed twice, then BLOCKED; data unchanged | NOT RUN |
| 04 | Pokémon edit | R, Down, A, Right, X, B | EV proposal clamps against 510 limit; X reset, B cancels | NOT RUN |
| 05 | Pokémon edit | R, A, Up, A | Changed move ID produces UNVERIFIED warning; no write | NOT RUN |
| 06 | Pokémon edit | R (to PP), A, Up, A, B | Move PP proposal supported for demo; B cancels from confirmation to proposal | NOT RUN |
| 07 | Pokémon edit | R to ability and nature; A on each | Unsupported edit notices | NOT RUN |
| 08 | Home | Down, A, R, Down, A, A | PC box switch, sample Pokémon detail, PC edit UNSUPPORTED | NOT RUN |
| 09 | Home | Down twice, A, Down, A, Up, A, A, A | Item quantity proposal blocked after two reviews | NOT RUN |
| 10 | Home | Down three times, A, A, Up, A, A, A | Money proposal blocked; original amount still 42000 | NOT RUN |
| 11 | Home | Down four times, A, A, B | Backup confirmation cancels; no file created | NOT RUN |
| 12 | Backups | Down, A, B, Down, A | Unknown backup status then RESTORE disabled message | NOT RUN |
| 13 | Home | Down five times, A; select each row and A | Five short recovery explanations visible and readable | NOT RUN |
| 14 | Any view | START | Clean app exit; Emerald Rogue original title unaffected | NOT RUN |
| 15 | Relaunch | Check original mock money/party/items | Original fictional values unchanged, no writes performed | NOT RUN |

The test routes should start on the listed view; return via B between cases. Example sequences are illustrative and can depend on the currently selected category/row. An explicit screenshot or observation should be recorded for each case, including page overflow, D-pad focus and HOME/START exit stability.

## CI interpretation

phase19/test_ui.cpp drives the same logical Key enum (Up/Down/Left/Right/A/B/X/L/R/Start). phase19/test_adapter.cpp covers validation boundaries and unavailable data. The optional isolated Azahar workflow launches the 3DSX and CIA, captures screenshots and validates that a synthetic SD canary file was not changed. That smoke job does not prove each button was recognized by actual 3DS hardware, nor any game editing, restore or recovery service.

**Hardware PASS requires actual recorded observations.** Until then leave manual physical 3DS UI gate PENDING, even if all CI jobs are green.
