# Phase 14 — authenticated restore core, live writes disabled

This is a tested restoration/recovery **prototype**, not a hardware-ready restore feature. Existing Phase 11–13 code and UI are untouched. No installable binary is produced by Phase 14 CI.

## Implementation

- `authenticated_restore.hpp`: complete data-only signed plan and backend-driven transaction engine, reusing Phase 13's exact money +1 / existing item +1 footprint validator. Verifies BOTH slots' raw checksums and MACs. Keeps all container bytes, including tail and historical slot, in the recovery record. No console keys or private saves enter this repository.
- Strict slot policy: both slots must be initialized 128 KiB H168 saves, same target title `0004000007D7BA00` and SD CID, distinct counters, latest below UINT32_MAX. Unknown sizes, foreign titles, corrupt/unauthenticated slots, ties and counter rollover fail closed. This is intentionally stricter than the Phase 11 read-only parser; a readable save can still be ineligible for restoration.
- The candidate is independently validated against the newest raw save, then placed in the older slot. Copy the active slot's header metadata, increment the wrapper counter, compute SHA-256 of bytes 0x30 through the raw payload end, obtain a stable file-bound console CMAC. No manual secret keys or extra CTR-SIGN hashing.
- Before mutation: reconstruct/validate the entire plan, fresh full-container equality check, exclusive durable recovery record of before AND after plus operation, journal readback, second full-container freshness check.
- Write sequence: target raw data; header prefix; header remainder excluding MAC; **CMAC last**. Preserve the previously active slot throughout. Every backend write must report exact count and flush success. Final verification compares the entire container and reauthenticates both slots.
- Any post-mutation failure attempts rollback of the touched slot only, then verifies the entire original container and authentication. Rollback failure or unrelated external modification returns `RecoveryRequired`, never success. It does not retry the candidate automatically.
- Restart recovery loads an immutable record, checks its digest, revalidates the authenticated plan and requires current bytes to be old/new bytes from that transaction. It never silently overwrites unrelated bytes or a different title. Checksums detect accidental journal damage; console MAC validation authenticates the saved images.

## Backends and boundaries

`native_pxi.hpp` implements fixed-target PxiFS0 archive/file acquisition, size-checked full-container reading, console MAC calls, RAII cleanup and the checked `FSPXI_WriteFile` source path. **It only opens FS_OPEN_READ, and `LiveWritesEnabled` is a constant false with no command-line override.** The write method immediately refuses; compiled probe must have no FSPXI_WriteFile reference. No native UI links it. Native full-container read/authentication itself is not yet exercised on hardware.

`journal_codec.hpp` implements the bounded, digested recovery format. `host_journal.hpp` implements an exclusive POSIX file store with file and parent-directory fsync, close checks and byte-exact readback. It is expressly prohibited in a 3DS build. Tests use this real disk journal with a simulated PXI device and deterministic MAC service double. The double is NOT AES-CMAC or evidence that Nintendo accepts a signature.

`native_journal.hpp` implements exclusive SDMC recovery-record creation, flushed exact writes, checked close and reopened byte-exact readback, recovery-record loading, and immutable completion receipts. A fixed `sdmc:/3ds/PKSM-Rogue/recovery/phase14.rec` record blocks any subsequent transaction, including after completion; there is deliberately no automatic deletion. Its parent directory must exist. Partial/corrupt records and IO errors block new writes. `NativeBackend` composes it with the permanently read-only PXI adapter. Each PXI snapshot closes/reopens the file/archive.

**Still required before enabling native writes:** verify native SDMC durability and startup UI integration for pending-transaction discovery; hardware verification of close/reopen reads; explicit restore/recovery confirmation UI; independent raw backup export alongside the container recovery record; read-only console CMAC verification; disposable on-device restore/relaunch/re-save/rollback and power-loss evidence. The existing Phase 12 raw backup alone cannot reconstruct a torn wrapper header/CMAC. Do not import a Phase 14 container journal as a raw `.sav`.

Power loss during metadata/CMAC writing is not claimed atomic. Keeping the prior slot intact reduces exposure, but actual AGB_FIRM fallback and SD controller durability remain unverified. Host tests simulate byte writes and restart recovery, not hardware storage semantics. A race between the final read and write cannot be eliminated without a proven device lifecycle/exclusivity protocol. Do not resume a game while a future transaction is active.

## Tests

Run `bash phase14/run_tests.sh` from the repository. Uses nonplayable generated fixtures only. Runs strict compiler warnings, normal and ASan/UBSan tests, and Phase 11–13 host regressions. On hosts where ptrace prevents LeakSanitizer, `ASAN_OPTIONS=detect_leaks=0 bash phase14/run_tests.sh` preserves ASan/UBSan checks; CI uses leak detection by default.

CI also compiles/relocatable-links the native probe with devkitPro/libctru, checks native write elimination, and rejects private game/save binaries. It does not boot a game, execute the console MAC service, or validate a CIA.

See [PHASE_HANDOFF.md](../PHASE_HANDOFF.md) and [REFERENCES.md](REFERENCES.md).
