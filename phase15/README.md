**31 October 2026 — PHASE 15 PHYSICAL 3DS LIVE AUTHENTICATION VERIFICATION PASS (READ ONLY).** User supplied two photographs of the diagnostic running on an actual 3DS. Initial screen: Phase 15 H168 v2.1.2, no writes/restore, no Phase14 recovery record. After Y: `RESULT: BOTH SLOTS VERIFIED`, `Container bytes: 263168`, `Two identical reads: YES`, Slot 0 `HDR Y RAW Y CMAC Y` wrapper counter **49**, Slot 1 `HDR Y RAW Y CMAC Y` wrapper counter **48**, latest slot **0** counter **49**. This establishes physical ability to read a full GBA VC container twice and independently validate its **existing** file-bound console CMAC tags through the live 3DS service. It **does not** validate signing a *new* altered slot, true SD journal durability, installation/rollback or actual restored game playback. Live writes remain disabled. User then showed the `A → A` SD-only test confirmation and subsequent screen reporting `SD SCRATCH VERIFIED: probe_001.bin`. This is **on-hardware PASS for unique 256-byte native SD file creation, flushed write, new-handle readback, and byte equality as checked by the application**. The screenshot does not independently show GodMode9 file size or prove persistence across a power cycle, and it does not validate the much larger Phase 14 recovery journal or game restoration. Preserve experimental Phase 15 PR #5 as draft. No private save content, CID, CMAC or screenshots committed to repo.

# Phase 15 — H168 read-only on-device GBA VC CMAC diagnostic

**Status:** experimental source and CI hardware-probe build. **No installed-game saves are modified.** A successful host test is NOT evidence that the Nintendo 3DS accepts a generated CMAC or that restoration is safe.

## What this app actually does

- Uses the existing Phase 14 `NativePxi` adapter to open the specific installed Emerald Rogue 2.1.2 GBA VC title `0004000007D7BA00` via Luma PxiFS0 with **`FS_OPEN_READ` only**.
- On **Y**, opens the live GBA container twice independently, requiring byte-exact equality and an expected container size. It parses both 0x200-byte slot headers and the two 128-KiB raw payloads. It verifies target title, shared SD CID field (without displaying CID), differing counters, acceptable rollover range, and both payloads' H168 dual-bank checksums.
- Requests a stable file-bound `FSPXI_CalcSavegameMAC` for SHA-256 of each slot's bytes from offset 0x30 through its raw payload, and compares both CMAC values to the stored 16-byte tags. It only displays **pass/fail** and wrapper counters; it does **not** print MACs, hashes, save contents or SD CID.
- Checks for the presence of the Phase 14 immutable `/3ds/PKSM-Rogue/recovery/phase14.rec` file without reading or changing that file. It never considers a failed/missing probe to mean the archive is safe to restore.
- On **A then A**, optionally creates a **new, uniquely named 256-byte generic test file** in `sdmc:/3ds/PKSM-Rogue/phase15/probe_NNN.bin` using native SDMC exclusive creation, flushed write, close, independent open and byte-exact readback. Existing files are never overwritten or deleted. This is **NOT** a real Phase 14 recovery journal, and is deliberately not made in the recovery directory. A failed/partial file is left for inspection.
- **B** clears the displayed scan; **START** exits. `LiveWritesEnabled` stays hardcoded `false`. There is no native restore UI or privileged write request in the diagnostic.

A `PASS` means the *existing* two slots meet the conservative Phase 14 policy and the current console service returned matching MACs at that instant. It is NOT authority to edit or restore a game save. On real consoles, a good readable GBA save may still fail the stricter Phase 14 both-slot policy, and the result must be reported rather than bypassed.

## How to test carefully on physical 3DS

1. In GodMode9 or Checkpoint, **make an independent untouched recovery backup of the installed game**. Never move/delete original backup; do not inject Phase 13 drafts. Close the running Emerald Rogue game.
2. Prefer Homebrew Launcher: put `PKSMRoguePhase15RO.3dsx` under `SD:/3ds/` and open it. The isolated CIA may optionally be installed over the older **experimental PKSM Rogue preview** title `000400000F73A400`, never over Emerald Rogue `0004000007D7BA00`. Because a previous experimental CIA crashed HOME Menu on this console, trying 3DSX first is prudent.
3. Launch Phase 15; confirm the banner says **READ ONLY / NO LIVE WRITES**. Press **Y** only once to run the live read + CMAC validation.
4. Photograph the full result: RESULT, container byte count, both slot header/raw/CMAC statuses and native last return code (if error). A `CMAC SERVICE FAILED` result may mean Luma/Pxi permissions or service command parameters need reviewing. Do not assume corruption; do not attempt restoration.
5. Optional: press **A, then A** to create the harmless SD-only 256-byte probe file. Check `SD:/3ds/PKSM-Rogue/phase15/` for a new `probe_NNN.bin` with size 256 bytes. Do not delete it or use it as a save.
6. Exit with START. Confirm the installed game still opens normally if desired. **Do not restore any edited draft, format/reinstall the game or change Luma settings to force test success.**

## Automated tests and proof boundaries

- `bash phase15/run_tests.sh` uses a nonplayable synthetic save and deterministic **fake** MAC response to test: dual identical reads, file-size/header/title/CID/counter constraints, invalid raw save, mismatched CMAC, MAC service failure/unstable results, stale first MAC and refusal on all faults. It also reruns Phase 14's native-independent host tests and earlier regressions.
- GitHub Actions builds native ELF/3DSX using devkitPro, verifies no `FSPXI_WriteFile` reference in the Phase 15 main object, performs an Azahar **UI-boot only** test and packages a separate experimental CIA. The Luma-specific privileged read/MAC service cannot be validated in ordinary Azahar.
- SD scratch test is native-only and must be checked on physical hardware; there is **no claim** of power-loss persistence or a proven native Phase 14 journal transaction from this probe.
- No personal saves, ROMs, keys or private binaries should be committed to public GitHub.

## Next milestone — Phase 16 release gates

Read-only full-container checks on an expendable console/title are necessary but **not sufficient**. Phase 16 still needs correct native recovery-journal lifecycle, independently validated restore plan and console MAC agreement, checked PXI exclusive lifecycle, rollback after faults/power loss, AGB_FIRM fallback, real-game load/save/close/relaunch, and explicit user approval before a separately reviewed change can enable installed-title writes.

Source: `phase15/diagnostic.hpp`, `main.cpp`, `sd_probe.hpp`, `test_diagnostic.cpp`; Phase 14 authentication reference `phase14/README.md` and `PHASE_HANDOFF.md` on Phase 14 branch.
