// SPDX-License-Identifier: GPL-3.0-or-later
#include "bridge/pksm_rogue_adapter.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace rogue_bridge;

    // 1. Capability & safety assertions
    auto caps = getCapabilities();
    assert(caps.canViewParty);
    assert(caps.mayEditIVs);
    assert(caps.mayEditEVs);
    assert(caps.mayEditPP);
    assert(!caps.mayEditNature);
    assert(!caps.mayEditAbilitySlot);
    assert(!caps.mayReplaceMoves);
    assert(!caps.mayWriteInstalledGame);
    assert(!caps.mayRestore);

    // 2. Standard Gen 3 Pokémon View from Phase 18 Mon Record
    RogueMonRecord recGen3{};
    recGen3.pid = 0x12345678;
    recGen3.ot = 0x00010002;
    recGen3.species = 252; // Treecko
    recGen3.checksum = 0xABCD;
    recGen3.ivs = {31, 30, 29, 28, 27, 26};
    recGen3.evs = {0, 252, 4, 252, 0, 0};
    recGen3.moves = {1, 71, 72, 0}; // Pound, Absorb, Mega Drain, None
    recGen3.pp = {35, 20, 10, 0};
    recGen3.effectiveNature = 3; // Adamant
    recGen3.abilitySlot = 0;

    RoguePKXView monGen3(recGen3, 0, 25);

    assert(monGen3.generation() == pksm::Generation::THREE);
    assert(monGen3.species() == static_cast<pksm::Species>(252));
    assert(monGen3.speciesStatus().status == MappingStatus::StandardGen3);
    assert(monGen3.speciesStatus().isStandard);

    assert(monGen3.iv(pksm::Stat::HP) == 31);
    assert(monGen3.iv(pksm::Stat::ATK) == 30);
    assert(monGen3.ev(pksm::Stat::ATK) == 252);
    assert(monGen3.ev(pksm::Stat::SPD) == 252);
    assert(monGen3.nature() == pksm::Nature::Adamant);
    assert(monGen3.abilityNumber() == 0);

    assert(monGen3.move(0) == pksm::Move::Pound);
    assert(monGen3.moveStatus(0).status == MappingStatus::StandardGen3);
    assert(monGen3.move(3) == pksm::Move::None);
    assert(monGen3.moveStatus(3).status == MappingStatus::StandardGen3);

    // 3. Expanded Emerald Rogue Pokémon (e.g. Gen 4+ species or custom move)
    RogueMonRecord recExpanded{};
    recExpanded.species = 448; // Lucario (Rogue expanded species)
    recExpanded.moves = {405, 0, 0, 0}; // Bug Buzz (Rogue expanded move)

    RoguePKXView monExpanded(recExpanded, 1, 50);

    assert(monExpanded.speciesStatus().status == MappingStatus::RogueExpanded);
    assert(!monExpanded.speciesStatus().isStandard);
    // Must return None for standard PKSM Species enum to prevent aliasing
    assert(monExpanded.species() == pksm::Species::None);

    assert(monExpanded.moveStatus(0).status == MappingStatus::RogueExpanded);
    assert(!monExpanded.moveStatus(0).isStandard);
    assert(monExpanded.move(0) == pksm::Move::None);

    // 4. Staging IV/EV/PP modifications
    std::array<uint8_t, 6> stagedStats{};
    // IV edit: HP to 31 valid, >31 rejected
    assert(stageIVEdit(monGen3, pksm::Stat::HP, 31, stagedStats));
    assert(stagedStats[0] == 31);
    assert(!stageIVEdit(monGen3, pksm::Stat::HP, 32, stagedStats));

    // EV edit: valid <= 252 and total <= 510
    // Currently 0 + 252 + 4 + 252 + 0 + 0 = 508. Setting SpAtk to 2 -> 510 (valid).
    assert(stageEVEdit(monGen3, pksm::Stat::SPATK, 2, stagedStats));
    // Setting SpAtk to 3 -> 511 (invalid).
    assert(!stageEVEdit(monGen3, pksm::Stat::SPATK, 3, stagedStats));
    // Setting an EV > 252 (invalid).
    assert(!stageEVEdit(monGen3, pksm::Stat::HP, 253, stagedStats));

    // PP edit: slot 0 (Pound) to 40 valid, slot 3 (empty move) rejected
    uint8_t stagedPP = 0;
    assert(stagePPEdit(monGen3, 0, 40, stagedPP));
    assert(stagedPP == 40);
    assert(!stagePPEdit(monGen3, 3, 10, stagedPP));
    assert(!stagePPEdit(monGen3, 0, 65, stagedPP));

    std::cout << "PASS PKSM Rogue Edition Compatibility Bridge synthetic verification\n";
    return 0;
}
