# Phase 10 — private real-save byte-level round-trip laboratory

**Status: PASS for offline mechanics; NOT YET proven playable, restorable or hardware-safe.**

On 31 October 2026 the user supplied, privately, the exact GodMode9 standard CIA for their installed \`Pokémon Emerald Rogue 2.1.2\` (\`0004000007D7BA00\`) and a matching unwrapped 128-KiB GBA VC flash dump. The binary-identity and read-only structure fingerprints are recorded under \`phase9/profiles/\`.

## Reproducible, non-invasive source

- \`memory_only_roundtrip.py\`: has **no file I/O**. Its \`draft_copy(raw, expected_sha256, operation, value, item_id)\` takes a supplied byte array and requires a caller-pinned exact SHA256. It supports only in-memory money and **existing** item-stack quantity changes. It validates both bank sector maps, 0x08012025 signatures, checksums and save counters; decodes the correct section-0 XOR key; verifies all populated 104-byte party-member encrypted payload checksums; edits the latest valid bank, recomputes only that section's checksum, and checks that only intended field/checksum bytes changed. It never touches the historical bank. \`rollback_bytes\` demands a byte-identical return to the exact original input.
- \`test_memory_only_roundtrip.py\`: uses only the deterministic NON-playable synthetic Phase 7 fixture, and tests normal in-memory edits, pinned SHA, invalid ranges, no-op, missing items, corruption rejection, bank selection and byte-perfect rollback. CI: [41198328043](https://github.com/hboots999-hash/Rogue-save-editor-/actions/runs/41198328043), **PASS**.

## Privately completed experiments

Local-only validation against the *user-supplied original copied save* (no personal save in source control):

| Test | Expected result | Outcome |
| --- | --- | --- |
| Money in active bank | 556799 to 556800 | PASS, 4 changed bytes including checksum |
| Existing Rare Candy stack (ID 113) | 899 to 900 | PASS, 2 changed bytes including checksum |
| Independent alternate read-only decoder | 28/28 sector checksums, both banks, same party and bag size | PASS |
| Preserve prior bank and the current Pokémon | byte-identical inactive bank and decoded Pokémon | PASS |
| Reverse each edit | SHA256 matches exact original; original input file unmodified | PASS |
| Negative input tests | SHA mismatch, out-of-range money/item, unknown item | PASS |

**These results are NOT proof of playable saves.** They establish that small encrypted numeric edits can be computed, verified and reversed on an entirely separate copy. No altered save was installed, imported through GodMode9, written to live game archives or booted in mGBA.

## Private emulator test kit — kept OUTSIDE GitHub

A separate user-only archive was prepared in the conversation workspace, with an extracted matching GBA ROM and original + two experimental copies. **Do not commit or redistribute the user's ROM, CIA, personal save or their private save fingerprint.** The kit's \`START_HERE_EMULATOR_TEST_ONLY.md\` explains safe, isolated game boot and in-game re-save tests using mGBA or the RetroArch mGBA core.

Emulator acceptance tests must pass **in this order**:

1. The untouched original copied save loads correctly using the ROM extracted from the same CIA.
2. Exactly one test save loads, shows its one-unit change, and navigates the hub/run.
3. The emulator can save *in game*, close completely, relaunch and load the save; other progression is unaffected.
4. Repeat independently on the second test save. Keep the untouched original separate.
5. Only after these pass can we begin a **disposable** installed-title restoration and recovery test, never on the user's sole real save.

The native Phase 9.4 CIA remains a toy-fixture-only offline editor. **Do not weaken its exact-synthetic allowlist based on this local round-trip.** Real GBA VC archive access, reliable rollback, proper ROM-version recognition on device, native editable output, startup safety and full PKSM UX remain unimplemented/unverified.

## Next blocking milestone

Evidence of gameplay and roundtrip inside an isolated emulator for both test copies. A game boot test cannot be replaced by sector checksum validation or a file's integrity alone.
