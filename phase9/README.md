# Phase 9 — independent, extdata-free 3DS runtime (NOT A RELEASE)

This is a separate diagnostic 3DSX built from the **same** candidate save bridge as the experimental PKSM fork. It deliberately avoids official PKSM's extdata startup path, installed title access, backup/restore APIs, and all output/save-writing functions. Do **not** install a GitHub CI CIA on physical hardware.

## Existing native evidence

- The original 3DSX built and ran in Azahar 2126.2, reading a wholly synthetic 128-KiB SD-copy fixture and showing money 40000, party 1, bag 3, species ID 25, HP IV 10: [run 41158813605](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41158813605).
- Phase 9.1 adds read-only details for selected party Pokémon (numeric species, held item, nature, ability slot, IVs/EVs, four moves) and bag items, with an isolated ui_state.hpp input model. Native 3DS compilation, ASan/UBSan host contract tests, and emulator launch workflow passed: [run 41159666627](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41159666627).
- The follow-up workflow [41159780175](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41159780175) succeeded: it sent additional guest A/B/X keyboard events, captured detail/confirmation/blocked-labelled screenshots, verified the synthetic imported fixture SHA-256 was unchanged, and verified no backup or draft directory was created. The screenshot **contents still require human review** before we can claim individual guest modal controls were observed. These checks demonstrate no-write safety only.

## Controls

- L/R: pages (summary / party / bag); D-pad up/down: select a party or bag entry; A: numeric detail screen; B: return/cancel; START: exit.
- X: rehearse an edit request; A first and second confirmation; after both confirmations display **EDIT BLOCKED - NO SAVE WRITTEN**; B returns to viewer.
- This is not a functioning money, bag, IV/EV, or move editor on 3DS. Phase 7/8 already contain offline candidate editors and the distinct verified SD-import backup gate, but they are **not integrated into this independent Phase 9 shell**. No installed game save is accessible here.

The optional input is a COPY at sdmc:/3ds/PKSM-Rogue/imports/00000001.sav, exactly 131072 bytes. The simulator's synthetic fixture is not playable. Unsupported/missing input is rejected; the UI should still allow page navigation and safe exit.

## What remains before MVP and any hardware use

1. Integrate the already-tested Phase 7/8 candidate editing and byte-verified import-backup/draft mechanisms with a stable emulator-native UI, without weakening readback, O_EXCL, or stale-import rejection.
2. Add a ROM-build-specific compatibility allowlist, validated against an exact disposable patched Emerald Rogue ROM and save. Fail closed for unknown versions, unsupported items/pockets or fields.
3. Run guest emulator input sequences and negative I/O tests on entirely synthetic data; collect and inspect screenshots and on-disk before/after evidence.
4. Validate disposable game save extraction/restore and recovery with the correct game build. Only thereafter consider a separate hardware-safe release candidate with distinct title ID/SD paths, hashes and rollback instructions.

A successful host test, 3DS compiler job, or Azahar launch is **not proof** that a draft plays correctly in the user's Emerald Rogue build or that the CIA is safe to install.
