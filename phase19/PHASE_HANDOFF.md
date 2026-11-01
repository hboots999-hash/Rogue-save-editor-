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
