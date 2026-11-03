# Issue #15 — Phase 17 → original PKSM read-only H168 save interface

**Status: independent experimental code, PR #9 draft. No installed-game writes or restoration.**

The original PKSM Pokémon editor is a **UI only** for this milestone. Neither its
stock generation-three save writer nor its automatic `TitleLoader::save` or
PKX save methods are compatible with Emerald Rogue 2.1.2 until separately
verified. Do **not** connect either writer to the API below. ROM-specific
species, nature, move, ability and PP semantics still require independent
H168 2.1.2 proof. User's broken **L button** is irrelevant to the API; the
PKSM frontend must provide L-free navigation, and must reserve **START+SELECT**
for Rosalina without taking over that combo.

## Two independent, versioned read-only contracts

1. **Save-only installed view** (NEW for Issue #15; does not need a Phase16
   journal): `installed_save_probe_v1.h` +
   `installed_save_probe_v1.cpp`. `P17SaveProbeReportV1` is a fixed
   **128-byte C POD** (ABI version 1) containing title ID, exact size,
   active slot index, two GBA VC *wrapper* counters, full-container SHA-256,
   active raw SHA-256, denial code, and **capability_bits = 0**. It contains
   no pointer, file handle, write flag, restore plan or private H168 parser.
   Even a positive-looking report is a **forgeable observation**, not proof
   of a console authentication event by itself.
2. **Recovery-journal evidence view** (EXISTING, unchanged ABI):
   `readonly_status_v1.h` / `P17ReadOnlyStatusV1`, a fixed **80-byte
   C POD**, only reports Phase16 full-container backup / RAM-only recovery
   evidence. It does **not** support installed restoration and does not
   need to be called to inspect a save-only PKSM view.

Keep the private `installed_save_probe_private.hpp` and
`installed_save_probe_native_v1.cpp` in a **Phase17-owned translation
unit**, separate from all Phase18 H168 parser and stock PKSM Gen3 types,
avoiding the duplicate-`h168` collisions found by QA-08.

## Native adapter (not yet linked into any PKSM/Phase20 UI)

The optional C entrypoint is:
```c
#include "installed_save_probe_native_v1.h"
int p17_native_inspect_copy_v1(P17SaveProbeReportV1 *report,
                               uint8_t *raw_out, uint32_t capacity);
```
- `raw_out=NULL, capacity=0`: **read-only metadata probe**.
- `raw_out` points to a **caller-owned heap buffer of exactly 131072
  bytes**, `capacity=131072`: **read and copy** only the independently
  authenticated active raw save bytes. Never allocate this on the 3DS
  main-thread stack. An error clears the buffer and returns 0.
- A successful return 1 means only that the native Phase17 read-only
  probe completed. It does **not** grant an installed-game writer or prove
  that a stock PKSM Pokémon editor understands ROM-specific IDs.

Private production path `inspectInstalledSaveCopyV1(device,report,copy)`
requires a device whose `writesAllowed()==false`; it rejects any
write-capable device even if its read data looks valid. `NativePxi`
opens the installed title with **FS_OPEN_READ**, makes **three independently
fresh full-container reads**, rejects any size or byte mismatch, verifies
both raw slots, title/CID, nonwrapping adjacent wrapper counters and
**console-bound CMAC** against the final read-only file session, then
returns one heap-backed 131072-byte active-slot copy. It closes the native
handle before the C adapter releases a successful report; a close failure
blocks. No fallback from a corrupt slot, header, counter, MAC or raw bank.
The implementation contains **no SD export, restore, native writer,
CMAC regeneration, journal replay or game-archive mutation operation**.

The exact expected installed game title is
`0004000007D7BA00`, and the observed sizes are:

| Source | Bytes | Interpretation |
| --- | ---: | --- |
| Active copied H168 raw save | **131072** | Input for *separately* validated Phase18 copied-save UI, NOT an authenticated game container |
| Authenticated GBA VC two-slot container | **263168** | Two (512-byte wrapper header + 131072-byte raw) slots with CMAC and wrapper counters |
| Phase16 RAM-only rehearsal journal | **526400** | 32-byte header + two full containers + 32-byte digest; NOT a production restore transaction |

The native read-only path remains **compile-verified only**. A host fake
MAC signer and Azahar emulator cannot establish that the service is
available to a new PKSM frontend on the physical console, nor that
the game recognises the copied bytes. The high-heap, multi-read path
must be checked on **expendable, nonprivate progress** before any
production claim.

## How the original PKSM frontend should consume it

1. The PKSM frontend shows an explicit **Rogue H168 (read-only)** game
   selection; it MUST NOT automatically select generic PKSM Gen3 save
   writer/PKX semantics or assume that every GBA save is H168.
2. On deliberate user action, call the Phase17 native C adapter from a
   separate compiled unit; check return 1, `p17_save_probe_report_shape_v1`,
   `P17_SAVE_READ_ONLY_VERIFIED`, fixed title, size and capability bits.
   A copied 131072-byte raw buffer can be handed to Phase18's isolated
   H168 read-only parser via a separate independently reviewed bridge.
3. Display raw numeric Pokémon, existing items, IVs, EVs and PP only if
   **Phase18's own independent copied-save validation** passes. Unknown
   ROM IDs are labelled unsupported, never interpreted as stock Gen3
   names. Displaying raw bytes or IDs does NOT enable editing.
4. User-directed editing (where separately proven) is limited to
   **new no-overwrite SD backup/draft files** using Phase18's *existing*
   guarded exporter and independently reverified whole-file/source
   footprint. Phase17's own `verify_offline_sd_hashes.py` and
   `verify_raw_draft_diff_v1.py` are external PC **read-only** checks
   requiring protected original hashes and independently generated
   expected draft bytes/manifests. A manifest reverse-engineered from
   the output draft is circular, NOT independent validation.
5. Do **not** invoke stock PKSM `TitleLoader::save`, Phase14
   `execute/recover`, native `FSPXI_WriteFile`, any generic save
   injection, CMAC rewrite or game restore from a verified report.
   The UI must keep `canRestore=false`, `canModifyInstalledGame=false`.
6. L-free controls: D-pad/A/B/X/Y/R as appropriate and deliberate
   cancel/exit. Reserve START+SELECT Rosalina access; no L-required
   confirmations or recovery controls. Read-only detection is not
   coupled to any button in the Phase17 API.

## Existing authenticated backups preserved

Preserve the already physically checked Phase15–17 work and Phase16 SD
records **unchanged**:
`sdmc:/3ds/PKSM-Rogue/phase16/original_container.bin` **263168 bytes**,
`sdmc:/3ds/PKSM-Rogue/phase16/rehearsal_only.rec` **526400 bytes**.
Keep a full SD external copy (preferably two distinct offline copies),
verify original hashes before/after testing, **never rename/truncate/clean
or overwrite existing files**. A Phase20
`backups/phase18_original_NNN.sav` is only an independent **131072-byte
copied SD import** on the same card, not a separately authenticated
full-container disaster-recovery archive. Phase16's immutable journal
does not turn that raw draft into an installable container.

## Tests and unresolved release gates

- `test_installed_save_probe_v1.cpp`: synthetic active-slot orientation,
  triple-read source stability, missing/short reads, write-capable device
  refusal, MAC failures, malformed header/CID/raw, equal/nonadjacent/
  rollover wrapper counters, forged status, no stale copied output.
- `test_installed_save_probe_abi.c`: C11 struct layout/version/capability
  checks, intentionally demonstrating a forged plausible status is a
  **shape claim only**.
- Strict C++17, ASan/UBSan, separate Phase18 H168 consumer link, ARM11
  standalone native module cross-compile + undefined-symbol write scan,
  inherited Phase14–17 rollback and read-only backup verifiers remain
  mandatory CI. **These are not real-console installed-save tests.**

**Still blocked:** physical read-only PKSM-native title service access;
stock PKSM editor compatibility; ROM-specific move/ability/nature and
cached-stat semantics; gameplay save load/re-save; native memory/heap
stress and actual hardware crash response; a true independently verified
off-device restore path; CMAC authoring; durable atomic journal and
authenticated transaction-bound receipts; power-loss and interrupted
native writes; reliable restore/rollback using expendable progress,
independent QA review and explicit user approval.

**Release decision:** `phase14::LiveWritesEnabled=false` in every build.
Issue #15 does not grant permission for any installed-game modification.
No merges and no private saves/ROMs committed.
