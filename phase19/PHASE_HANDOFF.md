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
