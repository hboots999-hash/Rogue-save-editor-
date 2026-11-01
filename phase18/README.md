# Phase 18 — isolated offline H168 editing engine (DRAFT)

This branch diverges from `feature/phase7-native-ci` and intentionally does **not** merge or alter Phase 13–16 PR branches. There is **no** installed-title read/write, live restore, CMAC generation, CIA packaging, or game launcher in this phase.

## What actually works

- `offline_engine.hpp` accepts exactly one **128 KiB copied, unwrapped** H168 candidate. Uses existing Phase10 H168 section mapping and encryption helpers, but additionally validates both banks, all 28 sector footers, complete unique IDs, consecutive counters, both banks' contiguous bag quantities, populated 104-byte Pokémon records, and each encrypted Pokémon checksum. Uses the latest valid bank and preserves the other bank.
- Pure-RAM proposals: money, existing bag stack quantities, per-stat IVs/EVs, four moves with per-slot PP and verified move IDs, and ability **slot bit** (not arbitrary ability ID). Natures may be displayed as `personality % 25`, but **nature changes are rejected** because changing PID can affect shiny, gender, Unown form, encryption and custom Rogue mechanics. No edit permits changing a species or OT ID.
- Item insertion is implemented as a **guarded append** (not a guessed pocket search/shift). It is rejected by default until an independently confirmed H168 item-ID/pocket map is supplied as explicit `Evidence`. Likewise, new move IDs and the meaning of the vanilla ability bit require verified `Evidence` supplied by integration; the default is to block.
- Proposals are checked by re-decoding the entire output, preserving both bank counters and mapping, checking Pokémon and sector checksums, and comparing **every changed byte** to a field-specific allowlist. `verifyExact` independently regenerates the candidate and compares every byte, including historical bank and unused tail.
- `offline_export.hpp` is the separately opt-in SD-only bridge: read exactly `<root>/imports/00000001.sav`, compare input to the captured bytes, generate and verify RAM candidate, create a uniquely named immutable `backups/phase18_original_NNN.sav` with readback, recheck input, create a uniquely named `drafts/phase18_draft_NNN.sav` and verify it again. No original, installed game or older draft is overwritten. On actual 3DS, root is restricted to `sdmc:/3ds/PKSM-Rogue`.
- `test_offline_engine.cpp` and `test_offline_export.cpp` use a **non-playable generated** fixture with rotated physical sectors, different per-bank encryption keys, two Pokémon in different permutations, 104-byte stride and expanded species ID. Tests positive changes, checksum integrity, exact rollback, stale counters/identity, denied unknown IDs, EV limits, corrupt data, appended duplicates, invalid paths, no-overwrite, stale imports, and immutable output.
- `.github/workflows/phase18-offline-editors.yml` runs strict C++17 plus ASan/UBSan host suites and cross-checks the fixture with the independent read-only Phase9 Python inspector.

## Research boundaries

`phase9/profiles/confirmed_h168_save_layout_READ_ONLY.md` documents an earlier private read-only H168 copy inspection (28/28 sector checksums, dual keys, 104-byte party stride). The exact user's **private save/ROM is not in CI** and was not available for this phase's independent fixture testing. That evidence is more relevant than vanilla Emerald's 100-byte party assumption.

The upstream [pret/pokeemerald Pokémon layout](https://github.com/pret/pokeemerald/blob/master/include/pokemon.h) describes 48 encrypted bytes, 24 shuffles, 30 IV bits, an egg bit and a binary ability slot. It is **not** an authoritative ID database or nature/ability ABI for Emerald Rogue 2.1.2. In particular, `MON_DATA_ABILITY_NUM` is only a two-way flag, not a generic custom ability-ID setter.

No official v2.1.2 move table, custom ability mapping or verified bag pocket insertion semantics has been independently extracted from the **matching embedded ROM SHA-256** `111b0008bcec519c59a02de57895a924fee5d3633c8b8fbb394a497153778ca3`. The fingerprint identifies the observed supplied ROM; it is not permission to edit a different build.

## Host tests

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Iphase18 phase18/test_offline_engine.cpp -o /tmp/phase18-engine
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Iphase18 phase18/test_offline_export.cpp -o /tmp/phase18-export
/tmp/phase18-engine /tmp/phase18-nonplayable.sav
/tmp/phase18-export /tmp/phase18-nonplayable.sav
python3 phase9/tools/inspect_gbavc_save.py /tmp/phase18-nonplayable.sav
```

This is code/test development, **not an approved ROM-specific live-save editor**. Synthetic CI cannot prove in-game navigation, reload, save/reload, precise move IDs, semantic ability behavior, SD power-loss persistence or rollback on hardware.

## Integration procedure (later, separate reviewed PR)

1. Include `phase18/offline_engine.hpp` (or `offline_export.hpp` for a copied import) in a **new** offline-only native editor UI. Do not remove the Phase9 exact-synthetic gate or modify the read-only Phase11–16 launcher in this branch.
2. Read the offline **copy**, inspect the bank and Pokémon, capture its active counter and the explicit expected Pokémon PID/OT/species/checksum, move or bag slot values. Build `Request` with all guards. Never edit original save buffers in place.
3. Populate `Evidence` only from independently audited matching Rogue ROM tables and disposable gameplay tests; never set flags merely to make an unsupported edit work. Keep unknown move, nature, species, pocket and ability-ID changes rejected.
4. Require deliberate user review and fresh source matching. Call `exportCopied` once, verify **both** a unique backup and a unique draft independently, then show their filenames. Exports are **not a request to restore**.
5. Run tests on separately backed-up **disposable copies** with the matching ROM: open, check game content, enter/exit hub/run, save in game, close/reopen, confirm Pokémon stats/moves/items, and demonstrate independent recovery. Only then consider a separate gated integration; never enable live writes as part of Phase18.

See `PHASE_HANDOFF.md` here for status and integration requirements. Never put personal .sav, .cia, .gba, ROM or trainer identifiers in Git.
