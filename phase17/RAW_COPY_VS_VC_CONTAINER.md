# H168 128-KiB offline copies vs authenticated GBA VC 263168-byte containers

**Phase17 / Phase20 boundary. No installed-game write permission.**

## Byte-exact, non-interchangeable formats

| Item | Exact bytes | Composition | Purpose and allowed operations |
| --- | ---: | --- | --- |
| Phase20 imported raw copy `imports/00000001.sav` | **131072** | Raw H168 save data (inner dual bank plus unverified trailer) | Read copied-data structure; guarded source-SHA-256 offline proposals; do not open an installed game |
| Phase20 `backups/phase18_original_NNN.sav` | **131072** | Independent name on **the same SD**, byte-identical to one import | No-overwrite raw *import copy*; not an authenticated full-game recovery image |
| Phase20 `drafts/phase18_draft_NNN.sav` | **131072** | One byte-exact guarded edit to the imported raw source | Experimental offline candidate; no on-console gameplay compatibility claim or archive restore |
| GBA VC wrapper `phase16/original_container.bin` | **263168** | Two independent `0x200`-byte headers + two `0x20000`-byte raw slots | Prior owner-verified read-only full-container backup from installed title; contains header title, CID, counters and CMAC fields |
| Phase16 rehearsal-only journal `phase16/rehearsal_only.rec` | **526400** | 32-byte record header + **two 263168-byte containers** + 32-byte SHA-256 footer | Synthetic/RAM-only recovery-plan evidence; not a production durable on-console transaction log |

In integers:
`RawSize=131072`,
`HeaderSize=512`,
`SlotSize=131584=512+131072`,
`ContainerSize=263168=2*131584`,
`JournalSize=526400=64+2*263168`.

A wrapper carries TWO raw slots with additional authenticated metadata. It is
**never** safe to rename or directly copy a 128-KiB offline draft to a
263168-byte title archive, pad it with guessed headers, or treat its raw bank
counter as the full GBA VC wrapper counter. The raw H168 parser handles its
two inner bank counters; Phase17's wrapper selector independently requires
adjacent, non-wrapping `timesSaved` counters and **independently authenticated
CMACs** from a console-bound read-only handle. Both validation systems are
insufficient to prove the save will load in the game.

## Two different checksums and snapshots

1. Phase20 SHA-256 locks the **entire imported 131072-byte copy**, including
   bytes not covered by the sector checksum. Its offline backup/draft
   checksum equality and readback detect many ordinary SD corruption/stale
   import failures. Such byte verification does **not** validate a GBA VC CMAC.
2. Phase17 evidence hashes the **whole 263168-byte authenticated wrapper**,
   verifies its journal includes exact unmodified original/candidate images,
   performs full-container freshness comparisons and checks both slots with
   the console-bound authentication operation. Its exported V1 status is an
   **unsigned, forgeable, write-free envelope** even when values look valid.
3. Neither a successful raw import nor an offline draft is an independent,
   off-SD disaster-recovery backup of installed progress. Two separately
   held verified full-container backups and an actual recovery procedure
   remain future restoration pilot prerequisites.
4. The current Phase14 journal contains images for old tightly restricted
   `MoneyPlusOne`/`ItemQuantityPlusOne` rehearsal edits; it has no
   separately validated adapter for Phase20 IV/EV/PP edits, and the native
   terminal receipt is one byte without a signed transaction binding.
5. Provenance, title identity, ROM-build metadata, full-wrapper current bytes
   and an independently reviewed signed container transformation are all
   missing from a `phase18_draft_NNN.sav` file. Never infer those details
   merely from a raw file having valid sector checksums.

## Safe integration rule

Phase20 may display Phase17's read-only status and source fingerprint
*as information only*. Its editor may export separately named, expendable
raw SD copies. It must neither receive `phase14::Plan` or `NativeBackend`,
nor call `execute`, `recover`, `FSPXI_WriteFile`, CMAC regeneration, or
any installed game archive write from any button, success status or draft.
There is **no authorised path** from Phase20's 131072-byte edited copy to an
installed game restore.

For future read-only physical observation, see
`PHYSICAL_READONLY_VERIFICATION_CHECKLIST.md`.
For independent and still-open release criteria, see
`PHASE20_PR12_RECOVERY_REAUDIT.md` and `PHASE20_READONLY_HANDOFF.md`.
**Keep `LiveWritesEnabled=false`.**
