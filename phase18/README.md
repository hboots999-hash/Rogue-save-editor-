# Phase 18 — isolated offline H168 editing engine (DRAFT)

This branch diverges from `feature/phase7-native-ci` and intentionally does **not** merge or alter Phase 13–16 PR branches. There is **no** installed-title read/write, live restore, CMAC generation, CIA packaging, or game launcher in this phase.

## What actually works

- `offline_engine.hpp` accepts exactly one **128 KiB copied, unwrapped** H168 candidate. Uses existing Phase10 H168 section mapping and encryption helpers, but additionally validates both banks, all 28 sector footers, complete unique IDs, consecutive counters, both banks' contiguous bag quantities, populated 104-byte Pokémon records, and each encrypted Pokémon checksum. Uses the latest valid bank and preserves the other bank.
- Pure-RAM proposals: money, existing bag stack quantities, per-stat IVs/EVs, four moves with per-slot PP and verified move IDs, and the **source-derived 2-bit ability slot** (not arbitrary ability ID). Source-derived nature editing uses an unencrypted 5-bit XOR modifier **without changing PID**, but is **blocked by default** until exact H168 ROM semantics are verified. The Gigantamax flag is never used as ability data. No edit permits changing a species or OT ID.
- Item insertion is implemented as a **guarded append** (not a guessed pocket search/shift). It is rejected by default until the exact H168 item-ID/pocket map and bag-capacity upgrade value are independently verified. The public EX-v2.1 source defines 80 initial total slots, 10 per upgrade, 450 maximum with 50 key-item slots and 999 items per stack; these numbers alone cannot establish the private 2.1.2 upgrade-byte offset. Likewise, new move IDs, three-way ability indices, nature modifier semantics and pocket mappings require verified `Evidence` supplied by integration; the default is to block.
- Proposals are checked by re-decoding the entire output, preserving both bank counters and mapping, checking Pokémon and sector checksums, and comparing **every changed byte** to a field-specific allowlist. `verifyExact` independently regenerates the candidate and compares every byte, including historical bank and unused tail.
- `offline_export.hpp` is the separately opt-in SD-only bridge: read exactly `<root>/imports/00000001.sav`, compare input to the captured bytes, generate and verify RAM candidate, create a uniquely named immutable `backups/phase18_original_NNN.sav` with readback, recheck input, create a uniquely named `drafts/phase18_draft_NNN.sav` and verify it again. No original, installed game or older draft is overwritten. On actual 3DS, root is restricted to `sdmc:/3ds/PKSM-Rogue`.
- `test_offline_engine.cpp`, `test_offline_export.cpp`, `test_pc_readonly.cpp` and `test_editor_model.cpp` use a **non-playable generated** fixture with rotated physical sectors, different per-bank encryption keys, two Pokémon in different permutations, 104-byte stride and expanded species ID. Tests positive changes, checksum integrity, exact rollback, stale counters/identity, denied unknown IDs, EV limits, corrupt data, appended duplicates, invalid paths, no-overwrite, stale imports, and immutable output.
- `.github/workflows/phase18-offline-editors.yml` runs strict C++17 plus ASan/UBSan host suites and cross-checks the fixture with the independent read-only Phase9 Python inspector.

## Research boundaries

`phase9/profiles/confirmed_h168_save_layout_READ_ONLY.md` documents an earlier private read-only H168 copy inspection (28/28 sector checksums, dual keys, 104-byte party stride). The exact user's **private save/ROM is not in CI** and was not available for this phase's independent fixture testing. That evidence is more relevant than vanilla Emerald's 100-byte party assumption.

The upstream [pret/pokeemerald Pokémon layout](https://github.com/pret/pokeemerald/blob/master/include/pokemon.h) describes 48 encrypted bytes, 24 shuffles, 30 IV bits, an egg bit and a binary ability slot. It is **not** an authoritative ID database for Emerald Rogue 2.1.2. The Rogue EX-v2.1 source stores a **three-way ability index** separately from the **Gigantamax flag**, as explained in [SOURCE_FINDINGS.md](SOURCE_FINDINGS.md). The public source's old offset comments predate its 104-byte party records; field-size calculations reproduce the historically observed H168 money (0x4A8) and bag (0x578) positions. The ROM's bag upgrade byte and content metadata still require independent private-build verification.

No official v2.1.2 move table, custom ability mapping or verified bag pocket insertion semantics has been independently extracted from the **matching embedded ROM SHA-256** `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`. The fingerprint identifies the observed supplied ROM; it is not permission to edit a different build.

## Host tests

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Iphase18 phase18/test_offline_engine.cpp -o /tmp/phase18-engine
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Iphase18 phase18/test_offline_export.cpp -o /tmp/phase18-export\ng++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Iphase18 phase18/test_pc_readonly.cpp -o /tmp/phase18-pc\ng++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Iphase18 phase18/test_editor_model.cpp -o /tmp/phase18-model
/tmp/phase18-engine /tmp/phase18-nonplayable.sav
/tmp/phase18-export /tmp/phase18-nonplayable.sav\n/tmp/phase18-pc /tmp/phase18-nonplayable.sav\n/tmp/phase18-model /tmp/phase18-nonplayable.sav
python3 phase9/tools/inspect_gbavc_save.py /tmp/phase18-nonplayable.sav
```

This is code/test development, **not an approved ROM-specific live-save editor**. Synthetic CI cannot prove in-game navigation, reload, save/reload, precise move IDs, semantic ability behavior, SD power-loss persistence or rollback on hardware.

## Integration procedure (later, separate reviewed PR)

1. Include `phase18/offline_engine.hpp` (or `offline_export.hpp` for a copied import) in a **new** offline-only native editor UI. Do not remove the Phase9 exact-synthetic gate or modify the read-only Phase11–16 launcher in this branch.
2. Read the offline **copy**, inspect the bank and Pokémon, capture its active counter and the explicit expected Pokémon PID/OT/species/checksum, move or bag slot values. Build `Request` with all guards. Never edit original save buffers in place.
3. Populate `Evidence` only from independently audited matching Rogue ROM tables and disposable gameplay tests; never set flags merely to make an unsupported edit work. Keep unknown moves, species, pocket, PC and ability-ID changes rejected; never enable nature/ability/index fields without exact-game evidence.
4. Require deliberate user review and fresh source matching. Call `exportCopied` once, verify **both** a unique backup and a unique draft independently, then show their filenames. Exports are **not a request to restore**.
5. Run tests on separately backed-up **disposable copies** with the matching ROM: open, check game content, enter/exit hub/run, save in game, close/reopen, confirm Pokémon stats/moves/items, and demonstrate independent recovery. Only then consider a separate gated integration; never enable live writes as part of Phase18.

See `PHASE_HANDOFF.md` here for status and integration requirements. Never put personal .sav, .cia, .gba, ROM or trainer identifiers in Git.


## Phase19 data-model contract

`phase18/editor_model.hpp` defines `ModelApiVersion=1`, `SaveView`, `EditorCapabilities`, `PartyView`, `ItemView`, `inspectReadOnly` and `beginRequest`. Use the current copied-save view to build request guards (counter, identity, checksum, old values), then alter only desired target fields after explicit user confirmation. PC parsing is an **optional read-only research candidate**; the layout remains unproven against the exact installed v2.1.2 ROM. See [SOURCE_FINDINGS.md](SOURCE_FINDINGS.md) for the public-source differences.


## Additional Phase20 guardrails

The live-save code remains untouched. Phase18's original `Request`/RAM proposal
API is extended without changing its operation enum: `Request.expectedDigest`
can pin the whole original copied save using portable SHA-256, and every
request created from Phase19's `SaveView` pins this digest automatically.
`beginMoveRequest(view,partySlot,moveSlot,request)` selects any existing
move (0–3) with slot-specific optimistic locks. Unverified move identifiers,
ability meaning, nature behaviour, bag inserts and PC edits remain disabled.
The bank validator also handles a legitimate UINT32_MAX-to-zero save counter
rollover, preserving the actual newest sector bank.

The source geometry reconciliation and strict feature status are recorded in
[SOURCE_FINDINGS.md](SOURCE_FINDINGS.md) and
[PHASE20_FEATURE_MATRIX.md](PHASE20_FEATURE_MATRIX.md).
Synthetic C++ and ARM11 compile tests do not establish real ROM compatibility
or gameplay-correct recalculation of cached IV/EV/nature-dependent party stats.

## Independent QA integration hardening

The supported next integration entrypoint is
[`integration_v2.hpp`](integration_v2.hpp), not the historical low-level
`propose` with synthetic research `Evidence` flags.
The internal reader now uses the scoped `rogue18::raw_v1` namespace;
the global `::h168` of Phase17 is not defined by this branch.
The SD exporter is restricted to an SHA-256-pinned v2-allowed offline
candidate, backed up separately and never written to installed storage.
See [PHASE20_INTEGRATION_CONTRACT_V2.md](PHASE20_INTEGRATION_CONTRACT_V2.md)
for exact phase ownership, integration code and capability matrix.
The existing PR #7 Phase19 header needs its own adapter/re-pin to v2;
this branch does not modify other feature branches.

QA PR #10 identified that Phase13/14 permits only money+1 and existing
item quantity+1; wider Phase18 proposals remain separate and cannot
be used to request an installed restore, even if their RAM checksums
validate. The Phase18 PC scratch array was also moved to heap to avoid
large 3DS stack frames. The host and ARM11 CI now compile a pinned
Phase17/Phase18 unified translation unit.
