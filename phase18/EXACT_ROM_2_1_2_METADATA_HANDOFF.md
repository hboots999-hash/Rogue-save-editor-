# Installed-copy ROM evidence — Emerald Rogue 2.1.2 (local, metadata only)

**Status:** a user-supplied GodMode9 CIA was inspected **locally** on 2026-11-01. No CIA, 32 MiB GBA ROM, save bytes, certificate, game code binary, sprite assets, encryption keys, or extracted binary tables were uploaded to this repository. This document records only non-executable offsets, SHA-256 digests, counts and illustrative field values. The currently deployed production `move_v1::preview` gate stays **DISABLED**.

## Binary identity and structure

- Input CIA SHA-256: `9c75b3868c2428dce48bad3285f817a21fb0c006a59abb5a5820fd91b9b2ec42`.
- ExeFS `.code` begins with a complete **33,554,432-byte GBA ROM** at CIA absolute offset `0x4500`. It has a normal validated GBA header `POKEMON EMER`, game code `BPEE`. These header fields alone are **not** proof of the game build.
- **Actual extracted ROM SHA-256:** `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`. Every future reader must match the entire ROM digest before interpreting the offsets below; a title ID/game code or `2.1.2` filename is insufficient.

### Exact-ROM lookup geometry (all offsets relative to 32 MiB GBA ROM)

| Table | Start | Record size | Records | Raw byte-array SHA-256 |
|---|---|---:|---:|---|
| Move names, fixed GBA text | `0x004C283C` | 13 | 936 | `e91817b5658420469ccce73e6e2b59075339ac8a326edea3b6d2136e8fe768d0` |
| Battle data variant A | `0x00BD27D0` | 28 | 936 | `b68bb9f44e18a4461ba0e230724c0947294559ffe3d675e9618c6918b91cb8ca` |
| Battle data variant B | `0x00BD8E30` | 28 | 936 | `323c8d3dfa069066a5ddf121e2e6ba36ab70c0bbdecd2dcb9aa96419e5c5c071` |

Within each 28-byte record, the compiled **base PP** field is at **offset +5** (consistent with multiple known move IDs, normal power/type/accuracy and the public EX-v2.1 C struct). These positions are actual measurements from the user's supplied GBA ROM and do **not** assume binary equality with public EX-v2.1.

- Ordinary move IDs **1..848** have named entries; ID 0 = none, ID 165 = Struggle (not an ordinary user-editable move). Entries 849..883 are blank/reserved; 884..935 are special Dynamax/Gigantamax-like names, **never automatically selectable**.
- Of 848 ordinary entries, **40 base PP values disagree** between variants A/B. Example: ID 58, `Ice Beam`, PP **10 / 15**. These are not interchangeable: **do not choose one table by guessing**.
- 807 ordinary entries have matching positive PP in both tables and decodable names, including Struggle ID 165. Excluding Struggle yields **806 potential UI review entries**; this is still **not permission to teach them**.
- Sample independently located ROM data: ID 53 `Flamethrower` PP 15/15; ID 405 `Bug Buzz` PP 10/10; ID 483 `Quiver Dance` PP 20/20; ID 552 `Fiery Dance` PP 10/10. These are **this ROM's numeric IDs**, not guessed from the public source.
- The public EX-v2.1 PP-Up layout remains a hypothesis for this target build until the exact-ROM code path is validated: four 2-bit counters, 0–3 each, `maxPP=basePP+floor(0.2*basePP*ups)`. Original Phase18 research deliberately resets replaced slot PP Ups, but exact-ROM behaviour may differ.

## Safety and Phase20 integration

1. A separate **read-only local extractor** can generate a 936-row CSV and small JSON report from an exact SHA-matched CIA. These local outputs are **not committed**; no ROM image is copied into a public branch.
2. Phase20 may consume a reviewed, fingerprint-tagged read-only search index as **informational content only**. It must explicitly show which PP values are variant-dependent; **never** automatically fill maximum PP for those 40 IDs.
3. A real move-replacement operation still requires: exact binary routine inspection or emulator evidence for table selection, required PP-Up handling, move legality and any cached battle structures; independently validated SD-only backup/draft preservation; and an approved new **versioned** production API. No v3 `Kind::Move` PP-only edits may be reinterpreted as move ID changes.
4. The imported CIA is not proof that the *currently running* installed game has precisely this SHA unless the CIA was dumped directly from that title and/or the on-console read-only identity matches.
5. **Do not enable** installed-game save writes, VC restore, CMAC signing, PC edits or unverified move IDs. Keep PR draft/unmerged; no Phase20 edits.

This metadata-only handoff replaces “we lack any matching ROM data” with **verified indexed move-name and two-base-PP-table evidence**, while retaining the real safety blockers above.
