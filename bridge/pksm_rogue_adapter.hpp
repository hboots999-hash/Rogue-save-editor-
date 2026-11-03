// SPDX-License-Identifier: GPL-3.0-or-later
/*
 *   PKSM Rogue Edition Compatibility Bridge
 *   Copyright (C) 2026 Jules & PKSM Rogue Contributors
 *   Based on PKSM interfaces Copyright (C) 2016-2022 Bernardo Giordano, Admiral Fish, piepie62
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
 *       * Requiring preservation of specified reasonable legal notices or
 *         author attributions in that material or in the Appropriate Legal
 *         Notices displayed by works containing it.
 *       * Prohibiting misrepresentation of the origin of that material,
 *         or requiring that modified versions of such material be marked in
 *         reasonable ways as different from the original version.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include <memory>

// Forward compatibility types for PKSM-Core enums
namespace pksm {
    enum class Generation : uint8_t {
        ONE = 1, TWO = 2, THREE = 3, FOUR = 4, FIVE = 5, SIX = 6, SEVEN = 7, LGPE = 8, EIGHT = 9
    };
    enum class Stat : uint8_t {
        HP = 0, ATK = 1, DEF = 2, SPD = 3, SPATK = 4, SPDEF = 5
    };
    enum class Species : uint16_t {
        None = 0,
        Bulbasaur = 1,
        // Standard Gen 3 ends at Deoxys (386)
        Deoxys = 386,
        MAX_GEN3 = 386
    };
    enum class Move : uint16_t {
        None = 0,
        Pound = 1,
        // Standard Gen 3 ends at Psycho Boost (354)
        PsychoBoost = 354,
        MAX_GEN3 = 354
    };
    enum class Ability : uint16_t {
        None = 0,
        Stench = 1,
        // Standard Gen 3 ends at Air Lock (77)
        AirLock = 77,
        MAX_GEN3 = 77
    };
    enum class Nature : uint8_t {
        Hardy = 0, Lonely = 1, Brave = 2, Adamant = 3, Naughty = 4,
        Bold = 5, Docile = 6, Relaxed = 7, Impish = 8, Lax = 9,
        Timid = 10, Hasty = 11, Serious = 12, Jolly = 13, Naive = 14,
        Modest = 15, Mild = 16, Quiet = 17, Bashful = 18, Rash = 19,
        Calm = 20, Gentle = 21, Sassy = 22, Careful = 23, Quirky = 24
    };
} // namespace pksm

namespace rogue_bridge {

constexpr uint32_t AdapterApiVersion = 1;

// Verification status flags for Rogue-expanded data
enum class MappingStatus : uint8_t {
    StandardGen3,       // Mapped directly to stock Gen 3 PKSM definitions
    RogueExpanded,      // Valid Emerald Rogue extension, not stock Gen 3
    UnsupportedOrUnknown // Unverified or invalid ID in Gen 3 context
};

struct FieldStatus {
    MappingStatus status = MappingStatus::StandardGen3;
    uint32_t rawId = 0;
    bool isStandard = true;
};

// Intermediate representation mirror of Phase 18 Mon struct
struct RogueMonRecord {
    uint32_t pid = 0;
    uint32_t ot = 0;
    uint16_t species = 0;
    uint16_t heldItem = 0;
    uint16_t checksum = 0;
    std::array<uint16_t, 4> moves{};
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 6> ivs{};
    std::array<uint8_t, 6> evs{};
    uint8_t abilitySlot = 0;
    uint8_t natureFromPid = 0;
    uint8_t effectiveNature = 0;
    uint8_t gigantamaxFactor = 0;
    uint8_t rawNatureModifier = 0;
    bool egg = false;
};

// Safe PKSM-compatible Pokémon view wrapping an Emerald Rogue H168 Mon
class RoguePKXView {
public:
    // Core identity & metadata
    uint32_t pid = 0;
    uint32_t ot = 0;
    uint16_t speciesRaw = 0;
    uint16_t heldItemRaw = 0;
    uint16_t pokemonChecksum = 0;
    uint8_t level = 1;
    bool isEgg = false;
    bool isParty = true;
    uint8_t partySlot = 0;

    // Stats
    std::array<uint8_t, 6> ivs{};
    std::array<uint8_t, 6> evs{};
    std::array<uint16_t, 4> movesRaw{};
    std::array<uint8_t, 4> pp{};

    // Modern Rogue Extensions
    uint8_t abilitySlot = 0;         // 0 = Ability 1, 1 = Ability 2, 2 = Hidden
    uint8_t natureFromPid = 0;       // vanilla Gen 3 (PID % 25)
    uint8_t effectiveNature = 0;     // Mint modifier (monAt + 18)
    uint8_t gigantamaxFactor = 0;

    RoguePKXView() = default;

    // Direct converter constructor from Phase 18 Mon representation
    explicit RoguePKXView(const RogueMonRecord& mon, uint8_t slot = 0, uint8_t monLevel = 1) noexcept
        : pid(mon.pid), ot(mon.ot), speciesRaw(mon.species), heldItemRaw(mon.heldItem),
          pokemonChecksum(mon.checksum), level(monLevel), isEgg(mon.egg), isParty(true),
          partySlot(slot), ivs(mon.ivs), evs(mon.evs), movesRaw(mon.moves), pp(mon.pp),
          abilitySlot(mon.abilitySlot), natureFromPid(mon.natureFromPid),
          effectiveNature(mon.effectiveNature), gigantamaxFactor(mon.gigantamaxFactor) {}

    // PKSM PKX Interface Methods
    [[nodiscard]] pksm::Generation generation() const noexcept {
        return pksm::Generation::THREE;
    }

    [[nodiscard]] FieldStatus speciesStatus() const noexcept {
        if (speciesRaw == 0) return {MappingStatus::StandardGen3, 0, true};
        if (speciesRaw <= static_cast<uint16_t>(pksm::Species::MAX_GEN3)) {
            return {MappingStatus::StandardGen3, speciesRaw, true};
        }
        return {MappingStatus::RogueExpanded, speciesRaw, false};
    }

    [[nodiscard]] pksm::Species species() const noexcept {
        if (speciesRaw <= static_cast<uint16_t>(pksm::Species::MAX_GEN3)) {
            return static_cast<pksm::Species>(speciesRaw);
        }
        // Expanded species (Gen 4-8) in Rogue must not be aliased to stock Gen 3
        return pksm::Species::None;
    }

    [[nodiscard]] FieldStatus moveStatus(unsigned slot) const noexcept {
        if (slot >= 4) return {MappingStatus::UnsupportedOrUnknown, 0, false};
        uint16_t m = movesRaw[slot];
        if (m == 0) return {MappingStatus::StandardGen3, 0, true};
        if (m <= static_cast<uint16_t>(pksm::Move::MAX_GEN3)) {
            return {MappingStatus::StandardGen3, m, true};
        }
        return {MappingStatus::RogueExpanded, m, false};
    }

    [[nodiscard]] pksm::Move move(unsigned slot) const noexcept {
        if (slot >= 4) return pksm::Move::None;
        uint16_t m = movesRaw[slot];
        if (m <= static_cast<uint16_t>(pksm::Move::MAX_GEN3)) {
            return static_cast<pksm::Move>(m);
        }
        return pksm::Move::None;
    }

    [[nodiscard]] uint8_t movePP(unsigned slot) const noexcept {
        return slot < 4 ? pp[slot] : 0;
    }

    [[nodiscard]] uint8_t iv(pksm::Stat stat) const noexcept {
        auto idx = static_cast<size_t>(stat);
        return idx < 6 ? ivs[idx] : 0;
    }

    [[nodiscard]] uint16_t ev(pksm::Stat stat) const noexcept {
        auto idx = static_cast<size_t>(stat);
        return idx < 6 ? evs[idx] : 0;
    }

    [[nodiscard]] pksm::Nature nature() const noexcept {
        return static_cast<pksm::Nature>(effectiveNature < 25 ? effectiveNature : 0);
    }

    [[nodiscard]] uint8_t abilityNumber() const noexcept {
        return abilitySlot;
    }

    [[nodiscard]] bool shiny() const noexcept {
        uint32_t tid = ot & 0xFFFF;
        uint32_t sid = (ot >> 16) & 0xFFFF;
        uint32_t pidLow = pid & 0xFFFF;
        uint32_t pidHigh = (pid >> 16) & 0xFFFF;
        return ((tid ^ sid ^ pidLow ^ pidHigh) < 8);
    }
};

// Adapter Bridge connecting PKSM UI with Phase 18 Engine
struct AdapterCapability {
    bool canViewParty = true;
    bool mayEditIVs = true;
    bool mayEditEVs = true;
    bool mayEditPP = true;
    bool mayEditNature = false;      // Gated pending verified nature modifier evidence
    bool mayEditAbilitySlot = false; // Gated pending verified ability mapping evidence
    bool mayReplaceMoves = false;    // Gated pending exact ROM table approval
    bool mayWriteInstalledGame = false; // PERMANENT HARD LOCK: Live game writes barred
    bool mayRestore = false;            // PERMANENT HARD LOCK: Live restore barred
};

inline AdapterCapability getCapabilities() noexcept {
    return AdapterCapability{};
}

// Stage an IV modification for the Phase 18 pipeline
inline bool stageIVEdit(const RoguePKXView& mon, pksm::Stat stat, uint8_t newIV,
                        std::array<uint8_t, 6>& outStats) {
    if (newIV > 31) return false;
    outStats = mon.ivs;
    outStats[static_cast<size_t>(stat)] = newIV;
    return true;
}

// Stage an EV modification for the Phase 18 pipeline
inline bool stageEVEdit(const RoguePKXView& mon, pksm::Stat stat, uint8_t newEV,
                        std::array<uint8_t, 6>& outStats) {
    if (newEV > 252) return false;
    outStats = mon.evs;
    outStats[static_cast<size_t>(stat)] = newEV;
    unsigned total = 0;
    for (auto val : outStats) total += val;
    return (total <= 510);
}

// Stage a PP adjustment for the Phase 18 pipeline
inline bool stagePPEdit(const RoguePKXView& mon, unsigned moveSlot, uint8_t newPP,
                        uint8_t& outPP) {
    if (moveSlot >= 4 || mon.movesRaw[moveSlot] == 0 || newPP > 64) return false;
    outPP = newPP;
    return true;
}

} // namespace rogue_bridge
