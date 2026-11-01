# Phase 19 handoff — isolated 3DS UI, synthetic read-only preview

**Development branch:** feature/phase19-editor-ui (based on feature/phase7-native-ci)
**Status:** Standalone UI integration candidate only. Experimental 3DSX/CIA, not an approved release. No restoration implementation and no live save writer.

## Architecture

phase19/ui.hpp: platform-independent state machine and immutable sample dataset; party navigation, PC box switching, Pokémon IV/EV/move/ability/nature selectors, money and bag proposals, range limits, cancellation, two confirmations, blocked final result, backup and recovery status. No IO interface is available.

phase19/main.cpp: libctru entrypoint with separate top/bottom console views; HID down-edge input handling, VBlank and clean START exit. No FSUSER, FSPXI, AGBSAVE or SDMC logic.

phase19/test_ui.cpp: host regressions for party/PC screens, all editing groups, lower/upper bounds, 510 EV budget, money and item caps, B cancellation, backup and restore refusal, and unchanged model after all workflows.

phase19/rsf: isolated experimental unique ID 0xF73A5 (title 000400000F73A500), UseExtSaveData=false.

## Evidence and outstanding gates

GitHub Actions workflow phase19-editor-ui.yml: host tests and forbidden-I/O scan; native 3DSX/ELF; experimental CIA packaging and title ID checks; best-effort emulator startup test. Source checks and results must be read from the actual GitHub run, not inferred from committed workflow text. Native compilation, emulator launch and physical installed CIA launch each require separate recorded evidence.

Integration with real save parsers, ability/nature/move name mapping, PC/storage layout, confirmed backup enumerator, Phase 18 edit engine, Phase 14–17 CMAC restore/rollback and real hardware roundtrips are NOT part of this branch. Do not enable a live writer on the basis of this demo. Keep Phase 18 and restoration work on their own branches; no merges.

## Interface adapter contract for future phases

Use read-only snapshots with explicit provenance and target/profile validation, or display unavailable/unsupported state. Source party and PC counts from independently validated parser results, not from the synthetic demo. Keep all real editing actions behind per-field allow lists and fail closed if checksum/layout is uncertain. A final stage/export or live restore transition must require separate, audited backend capability checks and a human-confirmed recovery plan; this build exposes no such hooks.

**Hardware caution:** If trying the experimental preview on a 3DS, first ensure independent verified GodMode9/Checkpoint backups and verify the install target is 000400000F73A500, never the game ID. A successful text screen does not prove a safe restore.

## 1 November 2026 — integration continuation

This PR was developed on the EXISTING feature/phase19-editor-ui branch. Added adapter.hpp, test_adapter.cpp, INTEGRATION_CONTRACT.md, improved ui.hpp/main.cpp/test_ui.cpp, and extended its existing GitHub Actions workflow. The Phase 18 draft PR #8 API was reviewed but not copied, linked, changed or merged. Phase 14–17 branches and restore sources were not modified.

Key adapter contract: ReadOnlyProvider -> capture() -> immutable bounded snapshot. It can expose only independently validated party, bag and money data, or explicit unsupported/error states. PC remains unsupported on real data pending a verified PC map. Ability is a slot bit, not an arbitrary ability ID; nature is PID-derived and immutable; changed move IDs require an independent ROM allowlist; item insertion is not supported. Real view snapshots cannot activate proposals, while mock proposals are always blocked after two confirmations. The native app still only loads fictional data.

See INTEGRATION_CONTRACT.md for Phase18 rogue18::validate, partyAt, Mon, Request, Evidence, and h168::item compatibility mapping. Verification of the actual installed title and full save edit/export, CMAC and rollback, game-load/re-save persistence and manual physical-console UI testing remain separate blockers. Keep the PR draft/unmerged.


## 1 November 2026 — real Nintendo 3DS hardware crash report (OPEN RETEST)

The user supplied a photo of an ARM11 core 0 **data abort** screen showing:
- `Current process: PKSM Rog (000400000F73A500)` — the isolated experimental Phase 19 CIA process, **not** Emerald Rogue.
- Fault `Translation - Section`, access type `Write`. Fault address on screen near `0x07FF85D8`, in the region normally used for the ARM11 application stack.
- **Hardware outcome of that experimental build: FAIL.** Emulator launch successes before this photograph are not a substitute. No evidence of changed game save bytes; the demo has no save writer, but live-game integrity still must be checked independently.

Potential root cause identified on source inspection: expanded adapter `Demo` embeds 14 x 30 Pokémon records plus 450 bag stacks (significantly over 32 KiB). Previously, `State` owned `const Demo demo` and main created a local `State state`, risking oversized thread-stack allocation. The 3DS executable now uses a **small State holding a const reference**, a single **static in-place populated** fixture, and a static native `State`. Compile-time and host test guards require `sizeof(State) < 256` and demonstrate the fixture remains large enough for the regression to matter. No actual root-cause symbolication or corrected physical-console test has yet been recorded; stack exhaustion is a strong hypothesis, **not proof**.

**Retest gate:** check HOME Menu and Emerald Rogue independently first; retain original saves/backups. Do not relaunch the crashing CIA. After replacement build CI passes, prefer testing corrected standalone 3DSX only. Stop on another exception and record the screen; keep CIA retest separate. A successful replacement emulator job does **not** close this hardware incident.

The Phase19 PR must remain **DRAFT/unmerged** and must not claim physical hardware pass until manually retested.


### Confirmed ARM11 exception symbolication of original pre-fix ELF

Downloaded and inspected the exact previous run's `PKSMRoguePhase19.elf` artifact from GitHub Actions run **41259169949** (prior hardware-tested CIA generation). The user photograph's registers read:
- `PC = 0x00104780`, `SP = 0x07FF85E0`, `FAR = 0x07FF85D8`, write access.
- `addr2line`: `0x00104780` = `gfxInit()` entry, `libctru/source/gfx.c:126`.
- ARM disassembly at `0x00104780`: `push {r4,r5,r6,lr}`. This writes below `SP`, including the faulting address `0x07FF85D8`.
- At the beginning of `main`, original executable has `sub sp, sp, #63744` followed by `sub sp, sp, #252` **before calling `gfxInitDefault()`**. This is 63,996 bytes of stack reservation in main, due to the oversized local `State` owning `Demo`.

Thus **the startup failure mechanism is confirmed as an out-of-bounds/unmapped stack write during gfxInit's prologue, following an oversized main stack frame**. This replaces the earlier qualified source-level hypothesis. The static snapshot/reference fix eliminates the oversized frame by design, subject to replacement build/emulator and a separate physical 3DS retest. It does NOT prove safety of unrelated code, actual backup integrity, or real device success.

Never recommend the pre-fix CIA/3DSX artifact. Distinguish old and new GitHub Actions run IDs.

## 1 November 2026 — Phase18 pinned copied-SD READ-ONLY bridge

Added phase19/read_only_import.hpp and generated-fixture test_readonly.cpp; updated adapter.hpp (stack-safe captureInto, true EX-v2.1 2-bit ability slots), native main/UI (explicit seventh home-menu import option and permanent real-data edit gate), Phase19 workflow (pinned isolated checkout of Phase18), and READ_ONLY_IMPORT.md.

The new entry accepts only exact 131072-byte H168 unwrapped copied input at sdmc:/3ds/PKSM-Rogue/imports/00000001.sav and rejects 263168-byte VC containers without truncation/unwrap. The Phase18 dual-bank parser validates both banks, counters, bag, party checksums before providing numeric species/moves/PP/IVs/EVs, accurate ability slot/effective nature, money and existing item values. Nicknames and level are intentionally unknown; no guessing. Real PC layout is unsupported; every real-data edit path returns READ ONLY. Existing synthetic UI mode remains intact.

**Source dependency is PINNED** at Phase18 commit 596743d8eaa073759a736b5a8107db43f3a8c17b (read-only use from separate checkout; other branches unchanged). The PR is draft and isolated. CI and emulator startup are not evidence of successful on-device imported-save reading, and previous hardware validation covered the prior demo-only build. No live game archive, save writes, draft exporting, CMAC, restoration or authenticated container parsing is enabled. See READ_ONLY_IMPORT.md.


## 1 Nov 2026 — host-only development hardening (no new hardware tests)

- **Legitimately empty party/bag now accepted**: a structurally valid copied H168 save with no party Pokémon and/or no bag stacks returns EMPTY instead of being rejected. Generated fixtures modify and re-sign both historical banks, then verify empty-state UI behavior.
- Extracted a testable FILE*-based exact read-only stream inspector. Fixed SD path remains the only native path, opened using fopen with "rb"; separate errors for file not found, inaccessible files, seeking/size failure, incomplete reads and wrapped 263168-byte container. A failed inspection preserves the previous screen data.
- Host integration test now uses disposable generated temporary files, compares every original byte before/after reading, refuses corrupted and wrapped files, and flips a byte in each of the 28 historical bank sectors to verify rejection (the trailing 4 sectors are not claimed to be independently authenticated).
- Pokémon detail screen now displays move IDs **with PP**, distinguishes mock party from copied save, and labels real PC storage UNSUPPORTED without misleading "Box 1/0". Native notices wrap at word boundaries so missing-file and recovery guidance is readable. R cycles forward without needing a functioning L shoulder button.
- The native build workflow audits the linked ELF for presence of the Phase18 read-only validator and absence of project-specific Phase18 mutation/export functions. Generic library FS service symbols may still exist because libctru supplies them; their presence is not evidence of this app calling them.
- "H168 structure checked" **does not mean ROM identity authenticated**. A structural decoder result never permits game-save writes, draft exporting or restore.

Phase18 PR #8 has subsequently added an optional source_digest.hpp on its branch; the Phase19 dependency remains intentionally pinned at 596743d8eaa073759a736b5a8107db43f3a8c17b until separately reviewed. Do not automatically merge or repin.

**The updated 3DSX is still awaiting its own physical-device check.** Earlier UI hardware passes applied to the preceding build. Keep Phase19 as an unmerged draft; do not test the CIA or the actual game save.


## 1 November 2026 — Phase 19 accessibility, Rosalina and Phase 20 contract

These changes were made without a physical 3DS:

- **No working L needed:** In PC boxes, IV/EV/move/PP/ability/nature groups, and bag paging, use **R or D-pad Right for next** and **X or D-pad Left for previous**. D-pad Up/Down selects; A enters; B cancels. Inside a proposal, X instead means reset and Left/Right adjust the number, so the controls remain context-sensitive.
- **START+SELECT is reserved for Rosalina:** A pure input gate recognises either button press order. It now exits only after **releasing standalone START**, and suppresses exit for any START press during which SELECT was detected. No SELECT-alone binding. Host tests cover simultaneous and staggered chords, standalone exit and recovery after a chord.
- **Imported real-copy safety latch:** `State.demoPreviewEnabled` is separate from `Demo.synthetic`; `switchView(state,false)` after a successful import clears all pending demo navigation/proposals and disables every edit. Explicit `Import -> X` repopulates the fictional fixture and re-enables mock previews. Failed import never sets the latch or loses current data.
- **Navigation coverage:** `test_accessibility.cpp` covers all seven home entries, PC box selection, L-free group selection, 19-stack and 450-stack bag paging, cancel/back operations, and import-to-demo transitions. Host and adapter suites continue to exercise IV, EV, move ID refusal, PP, ability/nature refusal, money, recovery and backup confirmations.
- **Rendering:** Native console uses an allocation-free word wrap helper from `screen_text.hpp`; statuses identify the source (demo vs copied save), numeric move IDs display with PP, unsupported real PC shows no fictitious box number. Additional emulator smoke captures opt-in import, missing-copy and exit-attempt screenshots where emulator keyboard mapping permits; screenshots do not prove button semantics or real hardware compatibility.
- **Source/build:** The read-only Phase18 decoder remains pinned to `596743d8eaa073759a736b5a8107db43f3a8c17b`. Native CI checks linked ELF for absence of project edit/export symbols and bounds State to <256 bytes. The previous stack-crash defect must not recur.

**Phase 20:** See [PHASE20_INTEGRATION_CONTRACT.md](PHASE20_INTEGRATION_CONTRACT.md) for current read-only APIs, unsupported real PC and identifier-name assumptions, required future edit intents, independent source fingerprints, hardware/recovery release gates and strict nonwriting policy.

**Safe later testing:** Run only the new 3DSX through Homebrew Launcher. Without placing any actual file, open **Inspect SD copy (read-only)** and press A; expect a missing-import warning, B return, standalone START release exit. If accessing Rosalina, hold START+SELECT together; it should not exit the app. Do **not** test the previously crashing CIA, install new CIA, rewrite your Emerald Rogue save or truncate the 263168-byte authenticated VC container. Independent originals remain untouched. All new build tests are host/emulator only, **not** real-console passes; PR #7 stays DRAFT/unmerged.


## 1 November 2026 — Phase19 accessibility/layout/stress completion

This branch now additionally includes:
- `screen_text.hpp` allocation-free, host-tested 31-column word wrapping, long-word splitting, blank/embedded hard-newline handling; `test_screen_text.cpp` covers page bounds including the final 450-stack page.
- `test_accessibility.cpp` exercises every home entry using **no L shoulder**: R/Right advances, X/Left reverses for PC/Editing/Bag; cancel/back paths and both session transitions are tested. The extreme 450-stack bag case is included.
- `test_state_stress.cpp` runs **40,000 deterministic no-L button events** over fictional and imported-data screen models under strict host builds and ASan/UBSan. It asserts unchanged source objects, valid selection/field/page bounds and that read-only imports never enter Edit/Proposal, including deliberate corruption of the `synthetic` display flag without lifting the independent session gate.
- `populateMock` now wipes the complete previous snapshot **in-place** before repopulating fictional content, preventing stale imported records from persisting in unused slots without reintroducing a giant ARM11 stack object.
- Current `INTEGRATION_CONTRACT.md` and README are updated to match the **live read-only parser integration** instead of the outdated pre-integration mock-only descriptions. The separate `PHASE20_INTEGRATION_CONTRACT.md` remains the canonical future interface contract.
- Azahar CI collects best-effort screenshots for import-menu navigation, missing-copy error and exit attempts, **not hardware proof**. Native exit is standalone START **release**, with START+SELECT reserved for Rosalina; emulator key mappings are not guaranteed.

No code has enabled actual save editing, installed-game archive writes, draft exporting, authentication, restoration or injection. Phase18 and Phase17 branch commits were not changed. The updated 3DSX **must be retested on hardware when available**; the previous successful hardware screenshots apply only to an earlier synthetic version. Keep PR #7 DRAFT and unmerged.
