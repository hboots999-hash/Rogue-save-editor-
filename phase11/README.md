# Phase 11 — H168 installed GBA VC direct READ-ONLY bridge

**Status: first native direct-live-read prototype; no direct installed-save writes or restores.**

## Reason for the separate gate

The Phase 10 preview successfully launched and created a distinct offline money+1 draft on the user's physical 3DS. It does **not** automatically access Emerald Rogue's installed save. The next goal is to read the live GBA Virtual Console save and, after independent recovery validation, safely restore edited data. We are not skipping backup/rollback verification.

## Grounded design

- Installed target title **`0004000007D7BA00`** (GBA-P-H168; user CIA package label 2.1.2). No ROM or personal save bytes enter this public repo.
- Uses Luma3DS's `svc 0xB0` *steal client service* to obtain **`PxiFS0`**, as done by [Checkpoint](https://github.com/BernardoGiordano/Checkpoint/blob/master/3ds/source/archive.cpp) and PKSM. Builds Pxi archive `ARCHIVE_SAVEDATA_AND_CONTENT` for the target SD title (low, high, MEDIATYPE_SD, 1).
- Opens the fixed GBA save file path `{1,1,3,0,0}` **read-only** using `FSPXI_OpenFile(..., FS_OPEN_READ,...)`, as in Checkpoint's `fsstream.cpp`.
- Parses the `.SAV` slot headers with save offset 0x200 and expected 128 KiB, selects highest `timesSaved`, and **refuses** any newest slot not marked with the exact target title ID. Does not silently read a stale H168 slot after another GBA VC has been played.
- Copies the raw 128KiB save entirely into volatile RAM, closes privileged handles, then independently validates both 14-sector banks and encrypted party checksums using our proven H168 parser.
- **No `FSPXI_WriteFile`, `FSPXI_CalcSavegameMAC`, live save mount/write, or restore code** is present. The existing optional X+A+A offline +1 draft still operates **only on `/3ds/PKSM-Rogue/imports/00000001.sav`**. In live view X is blocked.
- Only an explicit **Y** press attempts PxiFS0 reading; it is never initialized automatically at startup. **B** returns to the offline copy. **START** exits.

## Files

- `phase11/live_header.hpp`: standalone in-memory .SAV header and title guard; host test `test_live_header.cpp`.
- `phase11/rogue_svc.s`: svc wrapper, modified from Checkpoint `csvc.s` (permissively licensed; attribution retained).
- `phase11/live_read.hpp`: Luma service acquisition, Pxi filesystem read-only code and explicit handle cleanup.
- `phase11/main.cpp`: native 3DS UI, Y to read installed title and show money/party/bag as read-only. Offline-copy mode retained.
- `.github/workflows/phase11-live-reader.yml`: native compile, host parser + synthetic regression, Azahar offline regression, CIA packaging and installed-CIA emulator smoke test. **Emulator tests do not execute the privileged live service path.**

## Test milestones

1. **Host:** wrong title ID, uninitialized slot, differing slot counters and save sizes rejected.
2. **Native:** CIA/3DSX compile with 3DS `PxiFS0` code and service wrapper.
3. **Emulator:** previous offline backup + draft path still works; independent CIA installs and can launch as a title.
4. **Physical 3DS:** install optional Phase 11 preview CIA only after independent recent GodMode9 and Checkpoint backups, open menu, press **Y**, read the result without any write, capture exact status/error code. This hardware live-access step is NOT yet verified.
5. **Later:** new live **backup** button with readback and unchanged-source check; recovery rehearsal; only after proven repeatability should any direct restore capability be offered, using current console's CMAC signing as in Checkpoint 5.3 (mere byte-copy overwrite is invalid).

Hardware caveat: Luma service stealing depends on CFW/hardware permissions. An emulator-only launch does not prove it is available; errors must fail closed. A malformed GBA VC archive must never cause writing or fallback to another installed game. A GBA VC save's CMAC is console/title-specific and must be refreshed during a *future* restore; do not treat our raw-copy writer as a valid live restore implementation.

Project upstream reference: Checkpoint GPLv3 `3ds/source/gbasave.cpp` and `archive.cpp`, PKSM GPLv3 `3ds/source/titles/loader.cpp`. This prototype does not embed the user's proprietary game ROM/save.
