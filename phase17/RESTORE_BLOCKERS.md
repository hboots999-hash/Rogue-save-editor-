# Phase 17 review — blockers before permitting installed-game writes

**As of 1 November 2026:** The user has verified the Phase 16 **read-only** full GBA VC container, original backup, Phase 14-format SD rehearsal journal and restart verification on their actual 3DS. Phase 17 builds a RAM-only restore/rollback exercise on these verified inputs. **A successful Phase 17 RAM simulation must never enable game-save writes.**

| Blocker | Existing evidence | What must be verified separately before any real restore |
| --- | --- | --- |
| Native live file write handle | `phase14/native_pxi.hpp` is deliberately `FS_OPEN_READ`, `LiveWritesEnabled=false`, so no write permission was tested | Explicit privileged write permissions, close/reopen lifecycle, safe time with AGB_FIRM not running, and clean refusal when unavailable |
| Independent recoverability | Phase 16 SD folder contains an untouched full container and a distinct before/after journal | A production flow must make **independent untouched full-container backups** before attempting any installed-game write; protect against both copies being lost/corrupted |
| Recovery journal durability | Phase 16 created and verified a 526400-byte rehearsal record and reopened it after an **application restart** | Power-loss durability, filesystem flush guarantees and partial/truncated-record handling under physical interruption; the original `NativeJournal` does not have Phase 16's proven **32KiB chunked** SD writer |
| Journal recovery status | Phase 14 host tests exercise journal/receipts and restart recovery on a host file double | Native receipt lifecycle, crash re-entry, wrong/stale journal handling, metadata checks, and never overwriting old recovery files |
| Torn native write | Phase 14 host and Phase 17 RAM tests cover staged writes and simulated crashes | The actual FSPXI write path might have different atomicity, caching and interleaving; test with expendable progress and independent restoration route |
| Console-generated CMAC for proposed edit | Phase 16 on-device `SD JOURNAL VERIFIED` and `RESTART VERIFICATION PASS` require the console service to validate the after-image | True write acceptance by AGB_FIRM and game loading/re-saving, never inferred merely from an in-memory matching CMAC |
| Counter/slot compatibility | User's physical container passed both slots with wrapper counters 49/48; Phase 14 builds counter 50 in the older slot | Game selects the intended edited slot, can save afterward, and cleanly returns to untouched original on explicit rollback |
| Game data editors | Phase 18 has its own offline branch, and Phase 19 separate UI branch, both synthetic-safe | Verify each feature against exact H168 build, expected game mechanics and in-game persistence separately |
| Recovery refusal | Phase 17 simulates unrelated memory mutation and fails closed | Native corruption, unexpected saves between staging/writing and power cuts must require manual recovery and never auto-restore a stale image |

## 1 November 2026 — Separate read-only recovery evidence gate

`phase17/recovery_evidence_gate.hpp` now provides a **pure-memory, non-writable** classification for independently obtained full backups, exact journal bytes, current live save and the two terminal receipt-file presence checks. Unknown/unreadable storage state, missing or damaged records, mismatched full backup, stale/changed live data, terminal receipt files, invalid MAC/plan and unauthenticated candidate images all fail closed. Its sole success result is `ReadyForRamOnly`: it is **not** a permission to restore or a transaction plan. The gate has no FS service, no data output to write and no path to the native write adapter.

New host regression cases cover good record, unreadable/missing files, terminal receipts, unchecked receipt lookup, stale raw bytes/after-image, different backup, corrupt/truncated journal, independently invalid candidate despite a valid journal digest, and unusable console signer. The RAM backend also now simulates a failed terminal receipt after a completed mock restore and a failed receipt during mock rollback; each must return `RecoveryRequired`, preserve the journal and recover the source exactly via an independent retry. These are host simulations, **not real durable filesystem receipts**. The existing Phase 17 hardware diagnostic stays unchanged. **Future production integration would require** native read-only record enumeration, exclusive journal creation, robust signed/hash-bound terminal receipts and durable chunked writes; none is claimed implemented or enabled by this addition.

## Immediate safe next hardware test

Phase 17 standalone 3DSX/CIA reads the *existing* Phase 16 private SD files **read-only**, repeats current live slot CMAC, then simulates restore, torn writes, interrupted rollbacks and recoveries only on heap vectors. No SD file, live GBA VC or AGB_FIRM write function is called. Tests should stop and display an error if the live container changed since Phase 16.

After Phase 17 passes, perform an **independent GodMode9 size-only inspection** of the original and journal files and confirm the untouched original save still launches. Optional power-off/restart readback should be a separate non-destructive test; do **not** simulate a power cut during a real write.

## Proposed next reviewed development steps

1. Harden an isolated, never-enabled native restore backend: chunked authenticated and checked SD journal writer; independent full backups; explicit unresolved-journal detection; no silent overwrite/delete; independent read-only verification before every stage.
2. Add a safe, conspicuous, opt-in test mode using **only expendable game progress**, gated behind review and user approval. Preserve manual GodMode9 recovery instructions.
3. Test restoring one minimal money +1 change with real AGB_FIRM launch/re-save and an independently demonstrated rollback. Run fault-injection checks before allowing other editors to connect.
4. Integrate Phase 18/19 only after restoration and per-field save compatibility are independently proven.

**Do not change `LiveWritesEnabled` to true or remove `FS_OPEN_READ` merely because the Phase 17 simulator reports PASS.**
