# PKSM Rogue Edition Compatibility Bridge Integration Guide

## 1. Overview
This bridge provides a versioned, lightweight compatibility layer between FlagBrew's original PKSM Nintendo 3DS graphical interface (`EditorScreen`, `StatsEditScreen`, `MoveEditScreen`, `PKX`, `PK3`, `Sav3`) and our verified Phase 18 Emerald Rogue 2.1.2 H168 data engine.

## 2. Licensing & Legal Attributions
- This module is licensed under the GNU General Public License v3.0 (GPL-3.0-or-later).
- Based on PKSM-Core and PKSM interfaces:
  `Copyright (C) 2016-2022 Bernardo Giordano, Admiral Fish, piepie62, Pk11`
- Preserves Additional Terms 7.b and 7.c of GPLv3 requiring preservation of author attributions and prohibiting misrepresentation of the origin of modified material.

## 3. Data Mapping Specifications

### 3.1 Species
- **Standard Gen 3 Range (1..386):** Directly maps to standard PKSM `Species` enum and conventional Pokémon display names/sprites.
- **Rogue Expanded Range (387+):** Emerald Rogue incorporates modern Pokémon from Generations 4–8. Because standard Gen 3 PKSM definitions do not map to these extended indices, `RoguePKXView::species()` returns `pksm::Species::None` to prevent accidental aliasing, while `speciesStatus()` returns `MappingStatus::RogueExpanded`.

### 3.2 Moves
- **Standard Gen 3 Range (1..354):** Directly maps to standard PKSM `Move` enum.
- **Rogue Expanded Range (355..936):** Emerald Rogue adds modern moves (e.g. Bug Buzz ID 405, Moonblast ID 585). To prevent misrepresentation in standard Gen 3 screens, `move()` returns `pksm::Move::None` for expanded moves, and `moveStatus()` returns `MappingStatus::RogueExpanded`.

### 3.3 Stats, IVs, EVs, and Level
- **IVs (0..31):** Stored in Gen 3 Substructure M. Mapped directly to PKSM `Stat::HP` through `Stat::SPDEF`.
- **EVs (0..252):** Stored in Gen 3 Substructure E. Mapped directly to PKSM `Stat::HP` through `Stat::SPDEF`, with an enforced total budget limit of 510.
- **Level & Shininess:** Mapped directly from party structure and PID/TID/SID calculations.

### 3.4 Nature & Abilities
- **Nature:** Stored in unencrypted header offset +18 as an effective mint modifier (`oldMon.natureFromPid ^ req.nature`). Mapped to PKSM `Nature` enum (0..24).
- **Abilities:** Stored in Substructure M word 44 bits 29–30 (Slot 0 = Ability 1, Slot 1 = Ability 2, Slot 2 = Hidden). Returns slot index via `abilityNumber()`.

## 4. Consumption by Phase 19 & Phase 20

```cpp
#include "bridge/pksm_rogue_adapter.hpp"
#include "phase18/offline_engine.hpp"

// Convert Phase 18 Mon into a PKSM RoguePKXView
rogue_bridge::RoguePKXView view{};
view.pid = mon.pid;
view.ot = mon.ot;
view.speciesRaw = mon.species;
view.ivs = mon.ivs;
view.evs = mon.evs;
view.movesRaw = mon.moves;
view.pp = mon.pp;
view.effectiveNature = mon.effectiveNature;
view.abilitySlot = mon.abilitySlot;

// Staging an IV modification from PKSM's StatsEditScreen:
std::array<uint8_t, 6> newIVs{};
if (rogue_bridge::stageIVEdit(view, pksm::Stat::HP, 31, newIVs)) {
    // Pass newIVs to Phase 18 / Phase 20 offline export queue
}
```

## 5. Security & Safety Gates
1. **No Live Game Writes:** Does not touch `ARCHIVE_SAVEDATA`, `ARCHIVE_USER_SAVEDATA`, `AGBSAVE`, or PXI restore commands.
2. **Read-Only / Draft-Only Export:** Changes are routed strictly through Phase 18's guarded SD-only draft exporter (`exportCopied` / `exportCopiedBatch`).
3. **No Fabricated Mappings:** Unverified expanded IDs are explicitly surfaced as unsupported rather than guessed.
