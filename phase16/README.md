**1 November 2026 — PHASE 16 REAL 3DS INITIAL JOURNAL REHEARSAL PASSED; RESTART CHECK PENDING.** User supplied three hardware photographs of the Phase 16 diagnostic running on their physical Nintendo 3DS. First screen: `Full backup: NOT PRESENT`, `Trial journal: NOT PRESENT`, `LIVE CMAC: BOTH SLOTS VERIFIED`. Second: explicit A→A confirmation for creating a private, immutable full SD backup and trial journal. Third: `Full backup: PRESENT: preserved`, `Trial journal: PRESENT: preserved`, `LIVE CMAC: BOTH SLOTS VERIFIED`, `RESULT: SD JOURNAL VERIFIED`, `Backup bytes: 263168`, `Journal bytes: 526400`, `Prior counter: 49`, `Live unchanged? YES`. The Phase 16 implementation requires successful full-length native SD write/flush/reopen/byte-for-byte comparison for both files and verifies the newly signed candidate in RAM before returning this result. This provides genuine physical hardware evidence for initial recovery-file creation and candidate CMAC checks. **NOT YET SHOWN:** exiting/relaunching and pressing Y for `RESTART VERIFICATION PASS`, independent GodMode9 inspection, cold power cycle durability, real game restore, rollback or gameplay resave. No installed game-write operation was enabled. Neither save content nor journal bytes are included in the public repository; images remain in the private conversation. Do not merge or enable direct restoration on this evidence alone.

# Phase 16 — full recovery-journal hardware rehearsal (no live writes)

**Experimental. Not a finished editor or release.** This phase builds on the user's physically verified Phase 15 full-container read/CMAC and SD scratch-file write/readback. It tests the full-size Phase 14 recovery record on SD **without modifying the installed Pokémon Emerald Rogue 2.1.2 save**.

## What the 3DS build does

1. **X:** Re-run the Phase 15 read-only live two-slot CMAC check.
2. **A then A (once only):** Read the installed GBA VC container twice independently via a privileged **read-only** handle, verify both existing slot CMACs and H168 checksums, generate a narrow money +1 candidate into the older slot **in memory only**, request a console MAC for that candidate, and independently validate the entire resulting two-slot image. Only after those checks pass, write:
   - `SD:/3ds/PKSM-Rogue/phase16/original_container.bin` — an **exact, untouched full GBA VC container** (expected 263,168 bytes), exclusively created and reread.
   - `SD:/3ds/PKSM-Rogue/phase16/rehearsal_only.rec` — an *authentic Phase 14 journal-format* before/after record + SHA-256 integrity digest (expected 526,400 bytes for the measured 263,168-byte container), exclusively created, flushed in 32KiB chunks and reread.
   - It then performs a new live game read and confirms the original is still **byte-for-byte unchanged**. It never writes to the installed game.
3. **Exit with START and relaunch**, then press **Y:** Independently reopen both preserved SD files, decode and rehash the full journal, verify the backup matches its before-image, check the candidate's full-container MAC and strict allowlisted money +1 edit again, reread the live container twice, and require it to still equal the original. Result: `RESTART VERIFICATION PASS` if all checks pass.
4. The test refuses a second attempt while either file exists, even if one is partial. It does **not** delete, rename, repair or overwrite any existing file. Existing Phase 14 production recovery path `/3ds/PKSM-Rogue/recovery/phase14.rec` is **never touched**.

The Phase 16 files are NOT directly playable `.sav` backups and should **never** be injected into GodMode9/Checkpoint. The journal contains a candidate that has been *validated by a console MAC service*, but this alone cannot prove game acceptance on restore or safe rollback. Do not rename `rehearsal_only.rec` to `phase14.rec`. Keep `LiveWritesEnabled=false` throughout this phase.

## Important privacy and safety notes

The two SD files contain complete **personal game save data** and authentication metadata. Keep them private on your SD card. Do not upload these files or console keys, SD CID, MAC values or ROM to the public GitHub repository. Only share photographs of the result screen with the developer. Keep independent GodMode9/Checkpoint backups unchanged before testing. Prefer the 3DSX Homebrew Launcher build first.

Power-loss durability is not proven by `FS_WRITE_FLUSH` and readback; the restart test checks persistence across an application restart, **not a sudden power cut**. Do not deliberately remove the SD card or interrupt a write. A premature power cut may leave partial files; these deliberately block further attempts. Do not delete or retry them without separate investigation.

## How to test it

1. Ensure an **independent recovery copy** of Emerald Rogue is preserved outside the Phase 16 trial directory. Exit Emerald Rogue before running the diagnostic.
2. Download the experimental Phase 16 3DSX build produced by GitHub Actions when CI is green. Copy the `.3dsx` to `SD:/3ds/` and launch via Homebrew Launcher. CIA installation uses the same **separate preview title ID** `000400000F73A400` as earlier diagnostics and may replace that experimental title, **not** Emerald Rogue `0004000007D7BA00`.
3. On launch press **X** and photograph the CMAC diagnostic; it should report `BOTH SLOTS VERIFIED`.
4. Press **A then A**. The program creates a large private backup and trial record **on the SD card only**. Photograph the exact `RESULT`. The expected journal can consume about 790 KiB *including the separate original copy*.
5. Exit normally using START; launch the **same** build again and press **Y**. Photograph the `RESULT`, file lengths and previous counter.
6. If successful, open GodMode9 (without changing the files) to confirm the two distinct files exist with exactly 263,168 and 526,400 bytes. Do not inject the files, delete them or restore the game.
7. Confirm that Emerald Rogue still launches normally, if desired. No in-game change is expected.

If either operation reports a failure, **stop and share the screenshot**. A failed candidate CMAC generation may indicate a real discrepancy in the console signing adapter; do not bypass verification. A partial or corrupt record is not automatically repaired. If the live save has legitimately changed since the rehearsal, the Y check will refuse `RESTART VERIFICATION PASS` but should not damage either file.

## Automated coverage

`bash phase16/run_tests.sh` generates only synthetic, nonplayable fixtures and uses a **fake MAC service**. Its disk-backed tests verify the 263,168-byte original plus 526,400-byte journal, exact reopens, full signed-plan consistency in the test double, file immutability/nonoverwrite, prewrite failures, corrupted/truncated journal detection, mismatched backup detection, and a changed live source. It also runs all Phase 15/14 and older inherited host regressions. GitHub Actions builds native 3DSX and CIA, runs Azahar launch tests and checks that game writes remain gated.

Neither host CI nor Azahar can prove physical live CMAC generation, actual SD persistence or hardware game restoration.

## Remaining Phase 17 prerequisites

- A successful **physical** Phase 16 restart test, plus independent GodMode9 file size verification and matching live save source.
- Review whether the Phase 14 native recovery journal has the necessary storage persistence/lifecycle and supplies actual recoverable original authentication, not just a narrow raw-save backup.
- Explicit controlled hardware test on **expendable** game progress: prove a modified slot is accepted by AGB_FIRM, gameplay launch and re-save, correct slot/counter selection, safe interruption behavior, and rollback restoring full original container.
- A *separately reviewed* change is required before enabling any live GBA VC write. Never enable direct writes merely because this SD-only rehearsal passed.
