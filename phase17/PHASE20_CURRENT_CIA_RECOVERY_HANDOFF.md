# Phase17 → Phase20 independent recovery and latest-CIA safety handoff

**2 November 2026, Phase17 PR #9 — draft, unmerged.** All source tests use synthetic files. No actual 3DS access or privileged installed-save writes occurred during this review. `phase14::LiveWritesEnabled=false` remains unchanged.

## Exact reviewed integration revisions (do not silently repin)

- Phase20 combined experimental editor: `5a79bcbc522ad4e8d37f14bebfa9e7ac12143de2`, [all four jobs PASS](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41450825410). The Azahar emulator exercises a copied synthetic H168 import, **read-only** move browser/search and pre-existing Phase18 multi-edit SD export.
- Phase18 v3 offline exporter: `3625e160a17f805821098cf35a47a0f96aba8e16`.
- Phase18 move metadata/research: `96f976232931e42ebcd474888314ad3c9875258e`. `move_v2::ApplyApproved=false`. The external catalogue is exactly **14040 bytes** and SHA-256 checked; **installed ROM identity and move semantics are NOT verified**.
- Phase17 public read-only ABI in Phase20 is pinned to older verified `9908eab15a79c5b64126335d746b140028d05af1`. It contains **no native write permission**. The Phase17 producer is never included in Phase20.

## What was audited

The combined native Phase20 UI paths `main.cpp`, `read_only_v1.hpp`, `offline_edit_v1.hpp`, and `move_catalog_bridge_v1.hpp` have **no located installed-title archive, FSPXI native writer, CMAC regeneration or Phase14 execute/recover call** in reviewed sources. Phase18's pinned SD writer explicitly opens **`ARCHIVE_SDMC`** for app-owned `sdmc:/3ds/PKSM-Rogue` backups/drafts. The additional move catalogue is opened through `std::fopen(path,"rb")`, is externally checksum-locked and has a static false Apply gate. A Phase17 GitHub Actions job independently checks all of these *exact pinned revisions*; Phase20's own ARM11 build/symbol scan supplies complementary evidence.

**Important limitation:** a passing source scan does NOT mathematically prove a compiled program can never write to an installed game under every unusual fault or future dependency. No actual 3DS privileged archive/open or destructive power-loss test was performed. Keep this guarantee phrased **"no identified installed-title write path in the reviewed, pinned experimental editor"** rather than production hardware certification.

## Immutable provenance and format separation

| Record | Length | Use and protection |
| --- | ---: | --- |
| `imports/00000001.sav` | **131072** | Fixed SD **raw copied** H168 input; re-read and SHA-256-locked before export |
| `backups/phase18_original_NNN.sav` | **131072** | New no-overwrite SD import copy **on same SD**, NOT off-device disaster recovery |
| `drafts/phase18_draft_NNN.sav` | **131072** | New no-overwrite **raw draft**; never directly installed or injected |
| `phase16/original_container.bin` | **263168** | Previously verified full GBA VC container, two 512-byte headers plus two 131072-byte raw slots, with native wrapper/authentication metadata |
| `phase16/rehearsal_only.rec` | **526400** | Prior backup+future image RAM journal for Phase17 rehearsal; **NOT** a durable, production transaction log |

A `phase18_original_NNN.sav` and `phase18_draft_NNN.sav` pair is not the authenticated full-container backup. A successful copy/raw checker cannot create the missing authentic wrapper, title/CID-bound CMAC, validated power-loss journal, or permission to restore.

## New independent acceptance tests on Phase17

1. `offline_sd_recovery_audit.hpp` remains pure C++/memory-only and refuses missing/unchecked/IO error files, corrupted/truncated/oversized raw content, stale import, non-identical backup, wrong expected draft, unsupported reused names and no-op edits; **every code path refuses installed-game writes**.
2. New `verify_offline_sd_hashes.py` is a **read-only PC SHA-256 checker**, requiring protected original source SHA-256 **and separately calculated Phase18 expected draft SHA-256**. It rejects missing or corrupt raw files, symlink/hardlink aliases, wrong paths/file sizes, same-name/reused counters, stale source SHA, partial writes, or same original/draft digest. It can ALSO require both untouched Phase16 files and their independently protected pre-test SHA-256 values (recommended for console acceptance).
3. `test_verify_offline_sd_hashes.py` generates **only synthetic 131072-, 263168- and 526400-byte** fixtures, tests missing/unavailable files, truncated/overlong records, interrupted export-like files, unchecksummed-tail changes, aliased paths and independent restart rechecks. Does **not** perform real SD removal or power loss. It executes as part of strict and ASan/UBSan CI (the Python tests are unsanitized Python; the C++ tests are sanitizer-instrumented).
4. New Phase17 `phase20-offline-scope` job checks exact Phase20, Phase18 v3 and move metadata pin and refuses installed-title archive APIs or re-enabled move Apply. Future Phase20 updates need **fresh repinning and a fresh passing run**. It is static/symbol-based evidence, not a formal proof.

## Minimal physical acceptance — L-free

**First choose a verified 3DSX**, not an installed CIA. Preserve full SD separately with the **263168-byte** authenticated container and **526400-byte** Phase16 rehearsal journal unchanged. If `imports/00000001.sav` exists and is valuable, **STOP** instead of overwriting it. Use a **nonplayable synthetic 131072-byte** file in a disposable test setup, or initially just verify that missing import is refused. Do not load your personal save into a new experiment unless explicitly reviewed later.

Use D-pad/A/B/X/R/Y (L broken). `START+SELECT` stays **Rosalina**; standalone START exits on release. Check import, cancel and safe exit first. In Pokémon detail, **Y** opens read-only moves when no batch is active; moves may be searchable but **cannot be replaced**. Elsewhere **Y** can open/review a small two-edit batch. Limit first SD output to a harmless two-field synthetic batch and two explicit confirmations. Stop if HOME is unstable or import/output errors occur.

After exiting, power down before SD removal. Independently hash the unchanged synthetic source, new 131072-byte original copy and new draft; compare against a separately reconstructed Phase18 v3 expected candidate. Also compare preserved Phase16 container + journal to **off-device protected SHA-256 values**. The independent command and options are documented in [PHYSICAL_READONLY_VERIFICATION_CHECKLIST.md](PHYSICAL_READONLY_VERIFICATION_CHECKLIST.md). GodMode9 can inspect **SD-only files** read-only via A → Calculate SHA-256, with D-pad/A/B and no L; use a PC SD reader when possible. Do NOT select NAND/game archive write actions, editor, injection or recovery.

**CIA is separate and optional, never the default:** experimental Phase20 title `000400000F73A700` differs from game `0004000007D7BA00`. Verify actual package and existing installed titles and never install over any existing title. If game or HOME state is uncertain, SKIP CIA. An emulator PASS does not prove real CIA stability.

On any freeze, SD disconnection, incomplete draft, absent saved pre-test hash, failed independent byte verification or suspicious recovery file change, **STOP; preserve all evidence; no cleanup, repeat export, restore or journal replay**.

## Gameplay test on isolated *disposable* progress — still blocked

The current Phase20 draft is **not playable by direct import**. Before authorizing even an *expendable* future edited-save gameplay trial, an independent reviewed workstream must supply:
- Two protected, independently verified **whole 263168-byte containers stored off active SD**, plus a demonstrated off-device recovery/rollback method.
- Definitive target build/ROM signature and field-by-field H168 2.1.2 layout; real move PP table mode, PP-Ups, legal move replacement and in-game recalculation evidence. No guesses from metadata alone.
- Reviewed authentic container binding, title/CID/counter state, console-native CMAC semantics and verified mapping from original raw save to wrapper slots. Phase18's 131072-byte draft checksum does not supply this.
- Durable exclusive, bounded chunked journaling with authenticated terminal receipts, FS close/flush/re-read rules, allocation failures, torn/partial write and restart/power-loss tests on **disposable data only**. Existing Phase14 one-byte receipt is insufficient.
- Separate peer-reviewed native privileged writer and manual rescue route, real hardware test with no valuable progress, successful game load and save, independent readback and rollback evidence, and separate **explicit user approval**. No Phase20 source-code status bit may unlock this.

**Allowed Phase20 consumption:** Phase17 v1 read-only status and denial codes, copied SD byte SHA observation, raw exported-file stop conditions, and RAM-only simulation results. **Not allowed:** Phase14 execute/recover, NativeBackend, game save archive or CMAC writer, injection, implicit restoration approval or installing drafts. No merges; PR #9 stays draft. `LiveWritesEnabled=false`.
