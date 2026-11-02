# Phase 19 QA PR #10 remediation — 1 November 2026

**Branch:** feature/phase19-editor-ui | **PR:** #7 (DRAFT) | **Installed-game writes:** DISABLED | **Physical console:** unavailable.

## Resolved findings

| Issue | Resolution |
| --- | --- |
| QA-01 stale Phase18 pin | Reviewed and repinned both CI checkouts to immutable Phase18 ea7303e3d04e9c409febd44ee3e1f834de02cdc6 instead of 596743d8. Current decoder supports counter rollover, layout geometry, stricter Pokémon validation and SHA-256. |
| Native stack hazard | Enforcing -Werror=frame-larger-than=4096 exposed Phase18's PC research inspector with a **37,104-byte** ARM frame. Removed editor_model.hpp / pc_readonly.hpp from Phase19's native translation unit. Native uses only audited low-level Phase18 read-only decoder, geometry and SHA; full versioned SaveView is cross-tested on host only. |
| Main stack crash recurrence | Large Demo snapshots use static storage; input save copy is a 128 KiB heap allocation. Native State is <256 bytes, not an embedded snapshot; no UI recursion. The resulting STACK_USAGE.txt reports maximum **840 bytes** for inspectCopiedBuffer, **232 bytes** for main. Compiler rejects frames over 4096 bytes. |
| Phase18 API version and data | Native snapshot includes read-only sourceSha256 and sourceModelApi=1; host asserts Phase18 ModelApiVersion=1 and compares all party IV/EV/move/PP, two-bit ability, effective nature, species, bag, money, counter, SHA and unsupported capability flags against versioned SaveView. Unknown ROM IDs and PC are never guessed. |
| Invalid/short SD import | Exact 131072-byte unwrapped H168 path opened only with rb; wrapped 263168-byte containers rejected. Host tests include missing/null, wrong lengths, EIO and early EOF after a lying file-size report (GNU fopencookie), plus preserving all bytes in an input-file canary and previous displayed snapshot. |
| Corruption and stale counters | Generated non-playable fixture checks all 28 historical sectors, intentionally checksum-valid but semantically wrong ability slot, EV and PP, adjacent rollover 0xFFFFFFFF to 0, nonadjacent counter gaps, empty party/bag and source SHA differences. |
| Accessibility | R/Right next, X/Left previous; A/B and Up/Down; no L required. START exits only on standalone release, and START+SELECT is reserved for Rosalina in either press order. Six-row pagination, word-wrapped messages, move PP labels and explicit unsupported PC/backup/restore status. |
| QA-07 build template | devkitPro Makefile download pinned to exact upstream be2001fee08cf8ec7d3366e095e83f6f75da8942. Toolchain container tag is still moving; full bitwise reproducibility is not claimed. |
| Imported-data isolation | State.demoPreviewEnabled is an independent synthetic-preview latch, cleared with every successful copy import. Imported-data screens cannot enter mock edit proposals even if a snapshot flag is accidentally changed. Native ELF symbol audit excludes Phase18 proposal/export/writer methods. |

## Verification matrix

- Strict C++17 host tests and ASan/UBSan: UI, adapter, full synthetic H168 importer, cancellation, all seven main screens, 450-item pages, ROM-unsupported restrictions and repeated source transitions.
- devkitPro 3DSX/ELF build: native 4KiB stack-frame guard and STACK_USAGE report; project editor/restore/export symbol audit.
- Separate non-release experimental CIA packaging: isolated title ID only. **Do not install this CIA.**
- Optional Azahar smoke: startup, demo navigation, missing-save error, exit and SD canary checks. Screenshots are emulator evidence only.
- Latest CI evidence is linked from draft PR #7.

## Current Phase 20 read-only boundary

Native APIs: rogue18::validate, rogue18::partyAt, h168::item, rogue18::sourceDigest; the host suite cross-checks audited Phase18 SaveView API v1 without linking research PC code into the 3DS build. Data fields are strictly read-only; copied source SHA and bank counter are **not** permission for editing or restoration. PC boxes remain unsupported for imported saves. The GBA VC authenticated container is not opened or modified.

## Unresolved external blockers

- QA-08 duplicate h168 definitions across Phase17/18; Phase20 must isolate them in separate translation units or consolidate shared declarations. Phase19 does not include Phase17 headers.
- QA-02 Phase14 authenticated-container counter policy and any production live restore/CMAC bridge are outside Phase19, remain unapproved.
- Actual matching-ROM mapping, full-container authenticability, populated PC layout, physical 3DS testing of this new copied-save viewer and real game compatibility remain unverified.
- Phase16/17 RAM-only and synthetic emulator passes do NOT authorize live writes.

Keep PR #7 DRAFT and unmerged. Do not access the installed-game archive, write the save, inject or restore.
