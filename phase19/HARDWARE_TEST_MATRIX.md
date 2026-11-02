# Phase 19 experimental 3DS input and recovery test matrix

**Physical console status: PARTIAL PASS on corrected Phase 19 3DSX.** Host key-equivalent tests and Azahar smoke are not substitutes. This checklist is for a separate user-controlled manual test of the synthetic-only app. No personal save or game ROM is needed; this app does not open one.

## Before hardware testing

- Verify independent, recent original Emerald Rogue GBA VC backups exist externally, and confirm the console launches normally.
- Prefer standalone Phase19 3DSX via Homebrew Launcher first. If choosing the experimental CIA after the 3DSX check, verify title ID 000400000F73A500; it is NOT the Emerald Rogue title 0004000007D7BA00 or the prior preview 000400000F73A400.
- Never inject a draft, replace a game save, format SD, or delete any original backup as part of this test.
- Press START to exit at any point. If HOME Menu instability occurs, stop and report the crash screen rather than retrying blindly.

## Manual button matrix (all results should be demo-only)

| ID | Starting view | 3DS buttons | Expected visible result | Observed |
| --- | --- | --- | --- | --- |
| 01 | Home | A | Party: three sample Pokémon, status READABLE | PASS — navigated into party selection (photo of Volcarona subsequently selected) |
| 02 | Party | A on Volcarona | Volcarona detail: IVs, EVs, four moves, ability slot and nature | PASS — hardware photo shows accurate synthetic Volcarona data |
| 03 | Pokémon detail | A; select an IV; adjust; A three times | IV proposal reviewed twice, then BLOCKED; no save or backup changes in demo | PASS — proposal and final BLOCKED notice photographed; user confirmed return via B |
| 04 | Pokémon edit | R, Down, A, Right, X, B | EV proposals, 510 cap, reset and cancel | PARTIAL PASS (user-confirmed): Volcarona HP EV +1 proposal reached final BLOCKED notice on physical 3DS. 510-cap boundary, X reset and B cancel not individually tested. |
| 05 | Pokémon edit | R, A, Up, A | Changed move ID produces UNVERIFIED warning; no write | PASS (user-confirmed): Volcarona first move ID +1 proposal refused as INVALID/UNVERIFIED on real 3DS. |
| 06 | Pokémon edit | R (to PP), A, Up, A, B | Move PP proposal supported for demo; B cancels from confirmation to proposal | PASS (user-confirmed): increased synthetic move PP, opened confirmation and pressed B to return to proposal; no write. |
| 07 | Pokémon edit | R to ability and nature; A on each | Unsupported edit notices | PASS (user-confirmed): both Ability and Nature editing correctly showed UNSUPPORTED on physical 3DS. |
| 08 | Home | Down, A, R, Down, A, A | PC box switch, sample Pokémon detail, PC edit UNSUPPORTED | PASS (user-confirmed): R switched to Box 2 showing Garchomp, Milotic and Breloom; selecting Garchomp then A showed PC edit UNSUPPORTED. Prior Box 1 and Gengar read-only detail were photographed. |
| 09 | Home | Down twice, A, Down, A, Up, A, A, A | Item quantity proposal blocked after two reviews | PASS (user-confirmed): Rare Candy proposal and two confirmations ended with BLOCKED; prior bag inventory screen photo captured. |
| 10 | Home | Down three times, A, A, Up, A, A, A | Money proposal blocked after two reviews; sample amount 42000 | PASS (user-confirmed): displayed 42000, raised proposal by 1 and final confirmation showed BLOCKED; no separate photo for this step. |
| 11 | Home | Down four times, A, A, B | Backup confirmation cancels; no file created | NOT RUN |
| 12 | Backups | Down, A, B, Down, A | Backup status not checked in this app; restore disabled | PARTIAL PASS (user-confirmed): backup menu reports NOT CHECKED IN APP and restore disabled; deeper status/restore notice button paths not separately established. |
| 13 | Home | Down five times, A; select each row and A | Five short recovery explanations visible and readable | PARTIAL PASS (user-confirmed): missing-save, checksum-failure and restore-disabled warnings displayed; remaining recovery cases not individually verified. |
| 14 | Any view | START | Clean app exit; no crash on return | PASS — user confirms normal exit following B return; real-save byte comparison not performed |
| 15 | Relaunch | Check original mock money/party/items | Original fictional values unchanged, no writes performed | NOT RUN |

The test routes should start on the listed view; return via B between cases. Example sequences are illustrative and can depend on the currently selected category/row. An explicit screenshot or observation should be recorded for each case, including page overflow, D-pad focus and HOME/START exit stability.

## CI interpretation

phase19/test_ui.cpp drives the same logical Key enum (Up/Down/Left/Right/A/B/X/L/R/Start). phase19/test_adapter.cpp covers validation boundaries and unavailable data. The optional isolated Azahar workflow launches the 3DSX and CIA, captures screenshots and validates that a synthetic SD canary file was not changed. That smoke job does not prove each button was recognized by actual 3DS hardware, nor any game editing, restore or recovery service.

**Additional hardware photos (1 Nov 2026):** PC Box 1/2 shows Gengar Lv60, Lucario Lv62 and Metagross Lv64; Gengar details display correctly with explicit read-only label. Bag displays Rare Candy x25, Poke Ball x50, Ultra Ball x30, Potion x18 and Repel x12, matching the synthetic fixture. PC box R-switch and PC edit refusal are now user-confirmed PASS; item adjustment and double-confirmation remain unverified on this handheld.

**Hardware observations (1 Nov 2026):** Original Phase 19 CIA crashed at startup; replacement corrected 3DSX boots, displays both screens, navigates into sample Pokémon details, presents the IV numeric proposal and reaches the final BLOCKED notice. The user then confirmed B return and normal START exit. These specific rows are PASS, but whole-app manual hardware validation is **PARTIAL/PENDING**, with untested PC, bag, money, backup, recovery, moves/PP and ability/nature paths. The final BLOCKED message wraps poorly on the top screen (minor UI defect). This does not independently prove byte-for-byte save integrity. Never reuse the earlier crashing CIA.

**Hardware PASS for all screens requires individual recorded observations.** Keep full release and real-save integration gates pending.

**Additional user-confirmed hardware results (1 Nov 2026):** Synthetic Money screen displayed 42,000 and the +1 proposal terminated with BLOCKED after confirmations. Backup menu showed NOT CHECKED IN APP and restoration disabled. Backup creation/cancellation controls, item quantity proposals, recovery notices and other edit categories are still pending physical testing. No actual game save or backup was opened by this build.

**1 November 2026 physical testing continuation:** User reports Rare Candy item-quantity proposal reached the BLOCKED notice after confirmations. User also reports missing-save, checksum and disabled-restore recovery warnings displayed successfully. These are UI-level synthetic tests only: no real game data was opened, no authenticated restoration invoked. Other untested field-specific menus remain pending.

**1 November 2026 user confirmation:** Volcarona HP EV +1 in the synthetic Effort Values menu advanced through confirmations and ended BLOCKED. This demonstrates EV proposal and safe refusal on physical hardware; cap boundary and reset/cancel controls remain pending.

**1 November 2026 user confirmation:** Volcarona's first synthetic Move ID was incremented in the proposal screen; A returned INVALID/UNVERIFIED as expected. This verifies the unsupported-move ID guard on hardware, not any save-writing capability.

**1 November 2026 PP hardware check:** User confirms the move PP +1 proposal reached the first confirmation, and pressing B returned to the proposal screen without applying a change. The Phase 19 preview remains synthetic-only.

**1 November 2026 ability/nature hardware test:** User reports that opening the Ability and Nature categories and pressing A showed UNSUPPORTED for both. This completes physical-console confirmation of the six Pokémon editor category behaviors (IV, EV, Move ID, PP, Ability, Nature), although some edge conditions such as EV 510 limit and X reset remain untested. The separate CIA that previously crashed has not been re-tested; only corrected standalone 3DSX is hardware-verified. All real game-save operations stay disabled.
