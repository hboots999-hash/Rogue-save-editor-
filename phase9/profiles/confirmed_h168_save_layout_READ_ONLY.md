# Read-only observed save layout — H168 / Emerald Rogue package 2.1.2

Status: **PRIVATE REAL-SAVE READ VALIDATION COMPLETE; REAL-SAVE WRITES AND RESTORES DISABLED**.

Based on a privately provided GodMode9 \`0004000007D7BA00.gbavc.sav\` matching the title identified in [observed_h168_v212_READ_ONLY.json](observed_h168_v212_READ_ONLY.json). No personal save, ROM, nickname, money, trainer fields, save hash or game binary has been committed to the public repository.

Read-only, independent structural verification established:

- Size is exactly 131072 bytes, no outer wrapper.
- 2 physical banks of 14 sectors each, sector size 4096 bytes, with trailing unused space.
- Sector footer: section ID at 0xFF4 (u16), checksum at 0xFF6 (u16), signature 0x08012025 at 0xFF8 (u32), save counter at 0xFFC (u32).
- Both banks contain sections 0–13 exactly once, all **28/28 sector checksums** validate. Sections physically rotate; **never assume section 0 begins at physical sector 0**.
- Full-sector checksum sums little-endian u32 words through offset 0xFF4 and folds high/low halves to u16. Candidate latest bank is selected by a valid, greater save counter.
- Section 0 at offset 0x4C: u32 XOR encryption key, distinct per bank.
- Section 1 at offset 0x234: u32 party count (up to 6).
- Section 1 at offset 0x238: party array with **104-byte per-member stride**. Both a current and a historical party member validated with this stride; 100-byte stride fails for the historical second member. **Do not use vanilla Emerald's 100-byte party stride.**
- Party structure starts with personality u32, OT u32; encrypted 48-byte payload at +0x20, u16 checksum at +0x1C. Decrypt u32 words with personality XOR OT, then unshuffle four 12-byte substructures using personality mod 24. Checksums verified on all populated party records in both banks.
- Section 1 at offset 0x4A8: u32 money XOR section-0 key.
- Section 1 at offset 0x578: four-byte bag entries: u16 item ID + u16 quantity XOR lower 16 bits of section-0 key. All populated items in both real banks are contiguous and have plausible decrypted quantities.
- A private GBA VC save copy was read *only*; neither user's original file nor ROM was edited.

**Do not generalise semantic IDs:** Rogue v2.1.2 uses expanded 16-bit species identifiers. A species ID beyond the vanilla Pokédex limit is not proof of corruption. Move IDs, held items, ball-pocket locations, PC and hub flags must be verified against the exact embedded ROM binary before being labelled or edited.

**Safety:** This is substantial evidence supporting existing **read-only** party/money/bag parsing for the user's precise installation. It does *not* prove a modified save will boot, preserve hub progress, roundtrip via the installed VC title, or recover after a failure. Native Phase 9.4 continues to require the full byte-for-byte synthetic test fixture before exporting any edited draft. Never weaken that allowlist based on matching title ID, ROM SHA256, save sector checksums or one valid example.

Test source: \`phase9/tools/inspect_gbavc_save.py\`. Only synthetic data and corrupted synthetic copies enter CI. Latest host probe CI: https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41197540259 .
