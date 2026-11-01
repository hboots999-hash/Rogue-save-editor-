# Phase 17 → Phase 20 read-only authentication/status handoff

**Scope:** recovery evidence reporting only; no installed-game write, restore,
draft export, journal creation or privileged mutation. **PR #9 stays draft.**

## Findings from independent QA PR #10

- **QA-02: corrected, pending exact-HEAD independent QA repin.** The Phase14
  wrapper selector had accepted counters 48 and 60 because it compared only
  numeric order and equality. It now requires **non-wrapping adjacent
  GBA VC wrapper counters** and refuses equal, stale/skipped, unsupported
  rollover (0/UINT32_MAX), and staging near UINT32_MAX. 48/49 works in
  either physical slot order. This conservative wrapper rule is deliberately
  different from Phase18's modular **inner H168 raw-bank** counters. Never
  use one selector to infer the other. The first corrected implementation
  passed all five Phase17 jobs at run **41326378695**. New regressions are
  compiled and sanitized in subsequent CI runs; record their exact results.
- **QA-08: interface isolation implemented and compiler-tested.** Both
  Phase17's Phase10 vendored `h168` header and Phase18's modified Phase10
  `h168` header declare conflicting `Size`, `Bank`, `Info`, `Result`,
  and helper functions in the **same namespace**. Direct inclusion in a single
  translation unit is therefore unsupported and should fail strict builds.
  Phase17 now exports a **plain-C, versioned POD read-only status envelope**
  with no h168 types, templates, vector, path, file, service handle, raw
  container or restoration entrypoints. Source internals are included
  **only in a separately compiled Phase17 producer translation unit**.
  A pinned Phase18 consumer compiles and links exclusively to the C status
  ABI in `.github/workflows/phase17-rollback-rehearsal.yml`. This is an
  interface-compatible route, not evidence that all headers can be combined.
- **QA-03 / QA-09: NOT resolved.** Phase17 does not authorize a Phase18
  128KiB edited raw save to become a signed 263168-byte GBA VC container.
  Phase14's staged edits remain restricted to a narrow checked subset.
- **QA-06: NOT resolved.** Native journaling, receipts, external independent
  full backups, durability through interruption, native FS write permission,
  CMAC handling after real write and AGB_FIRM load/save/rollback are not proven.

## Public ABI: exact composition policy

Consumers include **only** `phase17/readonly_status_v1.h` and link
`phase17/readonly_status_v1.cpp`; those two files do not include
`h168` or `phase14`. In a **different translation unit**, a Phase17-only
producer may include `phase17/readonly_status_producer.hpp` and use
`inspectToStatusV1(evidence, signer, report)` *after* reading independent
full-container, journal and live bytes read-only with a genuine file-bound
signer. The producer must not be included in a Phase18 source file.

The 80-byte `P17ReadOnlyStatusV1` has fixed version/size, explicit failure
codes, expected title, wrapper counter, full-container bytes, journal bytes,
SHA-256 of validated **full-container backup**, and reserved fields; status
payloads have **no raw data, internal types, pointers, file handles or write
capabilities**. Always check `abi_version`, `struct_size` and
`p17_readonly_status_shape_valid_v1`; reject unknown/future versions.
`capability_bits` must be **zero**. The only positive code
`P17_RO_RAM_REHEARSAL_READABLE` means: suitable to **ask Phase17 to perform
a RAM-only exercise**, not a release certificate. A C caller can forge a
struct, and the shape validator is NOT cryptographic authentication or proof
that a current console-side source really matches. For provenance the data
must originate in a trusted Phase17 producer with fresh, console-bound MAC
verification on the device; never treat a payload from a file or user input
as authenticated. No positive flag allows edit/export/restore/rollback to an
installed title.

The hash covers the complete full-container source. It is NOT a replacement
for Phase18's separate SHA-256 optimistic lock over the **unwrapped** 131072
byte copy. Do not compare those hashes as though they covered the same format.

**Versioning rule:** v1 fields and C enum values are frozen. Adding a field,
changing a flag's meaning, or replacing data semantics requires a new ABI
version/header/function symbol and explicit compile+runtime negotiation;
never reinterpret existing reserved bits as write permissions.

## Synthetic test matrix and safety interpretation

- Recovery wrapper counters: 48/49 both directions, 0/1, equal, skipped,
  out-of-order, 48/60, 0/UINT32_MAX and near-overflow refusals.
- Corruption: both wrapper header magics, titles, raw-size fields, untrusted
  unsigned counters; journal SHA tampering in 32 distributed locations,
  truncation at 1, 16, 32, 64 and 4096 bytes, mismatched full backup, stale
  live read, signed-plan discrepancy and file-bound signer denial.
- Faults: forward short writes, simulated crash at each of four sub-steps,
  rollback short-write retries, recovery interruption/restart, failed restore
  and rollback completion receipts, and **foreign active-slot mutation during
  a staged shadow write**. All simulated targets are heap-owned vectors.
- Boundary: malformed pointers and oversized writes are rejected by the RAM
  backend. Separate C11 consumer and C++ Phase18-header consumer exercise
  status ABI version/size, reserved bytes and forbidden capability bits.
- CI: strict C++17 plus ASan/UBSan, inherited Phase14/15/16 tests, native
  3DSX/ELF, Azahar startup and experimental separate-title packaging; inspect
  the **exact current head** before claiming all complete.

## Nintendo 3DS resource and failure-path review

- `phase17/main.cpp` holds only a small `Screen` value on the stack.
  `phase17::Shadow` contains a few vector descriptors; its full copied
  GBA VC images, journal and crash/rollback simulations allocate on the
  **heap**, not on the per-thread ARM11 stack. This avoids the large local
  state that previously crashed a different Phase19 CIA.
- **Residual memory risk:** 263168-byte full container and 526400-byte
  encoded journal may coexist with several copies of before/after images
  and recovered plans. Peak allocations are not proven under worst-case
  low-memory 3DS conditions. `std::bad_alloc`, C++ exception availability
  in every linked build, heap fragmentation and cleanup after allocation
  failure need explicit device-independent stress testing before release.
  Future implementations should enforce a strict memory budget, reuse
  bounded heap buffers, avoid hidden copies, and abort safely when memory
  allocation fails. Native builds alone do not prove this.
- `phase14/NativePxi::read` currently reads the whole bounded container
  into a heap vector in one FSPXI call. `NativeJournal::readAt/createAt`
  do one potentially ~526400-byte operation. Phase16's independent
  `NativeStore` uses 32KiB chunks and on-device readback for SD-only
  rehearsal. The Phase14 native journal has **not** been proven power-loss
  durable or safe against intermittent storage faults. Do not enable that
  backend merely because RAM tests pass.
- `NativePxi` opens the installed game **FS_OPEN_READ**, and
  `phase14::LiveWritesEnabled=false`. Even though an unreachable
  legacy `FSPXI_WriteFile` source path exists in Phase14, it is compile-time
  gated off and unavailable to Phase20. Phase20 must NOT link a native live
  writer or alter the gate. Read-only MAC calls must use the currently
  associated open file/service session; tests with a fake signer do not prove
  the native path.
- `NativeJournal::finish` currently writes a **one-byte receipt** without
  a signed hash binding to the exact plan, and its read/existence error
  handling is unsuitable as a production transaction log. No Phase20
  design can treat that receipt as a durable authority for save restoration.
- If phase17 has a stale journal or a foreign change, **do nothing to the
  installed game**. No automatic replay, emergency injection, overwrite
  or cleanup. Retain external immutable backups and report blocked state.

## Release and hardware gates still closed

The user already reported physical Phase16 read/authentication and independent
backup/journal restart success, plus Phase17 RAM-only rollback success.
Those tests did **not** write or restore the installed Emerald Rogue game.

A production transaction needs **independently verified expendable progress**,
external backups, native journal durability/power-loss evidence, independent
source identity and exact H168 2.1.2 ROM data, approved per-field write policy,
actual 3DS game load/re-save verification and reliable rollback with a reviewed
manual recovery route. No hardware work is requested here.

**Phase20 may consume now:** versioned *read-only* C status data and denial
codes, strict title/counter/fingerprint provenance reports, and synthetic
RAM-only fault results. **Phase20 may NOT consume:** Phase14 `execute/recover`,
`NativeBackend`, privileged save writer, guessed ROM mappings, a general
raw-save-to-GBA-container adapter or any write permission. Keep all PRs draft
and unmerged.
