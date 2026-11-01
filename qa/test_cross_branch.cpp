// Isolated cross-branch synthetic contract checks. No ROMs or private saves.
// Include Phase18's existing fixture without its standalone main().
#define main qa_fixture_reference_main
#include "phase18/test_offline_engine.cpp"
#undef main
#include "phase18/editor_model.hpp"
#include "phase19/read_only_import.hpp"
#include "phase19/ui.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    const auto original=fixture();
    assert(original.size()==h168::Size);
    static phase19::Demo displayed{};
    std::string reason;
    assert(phase19::inspectCopiedBuffer(original.data(),original.size(),displayed,reason));
    assert(!displayed.synthetic && displayed.moneyStatus==phase19::Availability::Ready);
    assert(displayed.partyCount==2 && displayed.pcStatus==phase19::Availability::Unsupported);
    assert(displayed.backupStatus==phase19::Availability::Unverified);

    const auto model=rogue18::inspectReadOnly(original.data(),original.size());
    assert(model.valid && model.apiVersion==rogue18::ModelApiVersion);
    assert(!model.capabilities.liveWrite && !model.capabilities.restore);
    assert(!model.capabilities.pcEdit && !model.capabilities.insertItem);
    assert(!model.capabilities.arbitraryMoveId && !model.capabilities.abilityId);
    assert(!model.capabilities.nature);
    assert(model.sourceSha256==rogue18::sourceDigest(original.data(),original.size()));

    rogue18::Request money{};
    assert(rogue18::beginRequest(model,rogue18::Kind::Money,0,money));
    money.money=model.money+1u;
    const auto candidate=rogue18::propose(original.data(),original.size(),money);
    assert(candidate.ok && candidate.candidate.size()==h168::Size);
    assert(rogue18::verifyExact(original.data(),original.size(),
                               candidate.candidate.data(),candidate.candidate.size(),money));
    assert(phase19::inspectCopiedBuffer(candidate.candidate.data(),
                                        candidate.candidate.size(),displayed,reason));
    assert(displayed.money==model.money+1u);
    assert(!displayed.synthetic);

    // The viewer must not grant edit permission to an imported draft.
    phase19::State ui(displayed);
    ui.homeRow=3;
    phase19::press(ui,phase19::Key::A);
    phase19::press(ui,phase19::Key::A);
    assert(ui.screen==phase19::Screen::Notice && !ui.exitRequested);

    rogue18::Request forbidden{};
    assert(rogue18::beginRequest(model,rogue18::Kind::Nature,0,forbidden));
    forbidden.nature=uint8_t((forbidden.nature+1u)%25u);
    assert(!rogue18::propose(original.data(),original.size(),forbidden).ok);
    assert(rogue18::beginRequest(model,rogue18::Kind::AbilitySlot,0,forbidden));
    forbidden.abilitySlot=uint8_t((forbidden.abilitySlot+1u)%3u);
    assert(!rogue18::propose(original.data(),original.size(),forbidden).ok);
    assert(rogue18::beginRequest(model,rogue18::Kind::InsertItem,
                                static_cast<unsigned>(model.items.size()),forbidden));
    forbidden.itemId=4;
    forbidden.quantity=1;
    assert(!rogue18::propose(original.data(),original.size(),forbidden).ok);
    assert(rogue18::beginMoveRequest(model,0,0,forbidden));
    forbidden.moveId=65535;
    assert(!rogue18::propose(original.data(),original.size(),forbidden).ok);

    // Per-sector corruption in both banks must fail without replacing current UI.
    const unsigned originalMoney=displayed.money;
    const unsigned originalSpecies=displayed.party[0].species;
    for (unsigned sector=0;sector<28;sector++) {
        auto corrupt=original;
        corrupt[sector*h168::Sector+0x350]^=0x10;
        assert(!phase19::inspectCopiedBuffer(corrupt.data(),corrupt.size(),
                                              displayed,reason));
        assert(displayed.money==originalMoney && !displayed.synthetic);
        assert(displayed.party[0].species==originalSpecies);
    }
    for (size_t n : {size_t(0),size_t(1),h168::Size-1,h168::Size+1,size_t(263168)}) {
        auto malformed=original;
        malformed.resize(n,0);
        assert(!phase19::inspectCopiedBuffer(malformed.data(),malformed.size(),
                                              displayed,reason));
        assert(displayed.money==originalMoney && !displayed.synthetic);
    }
    // Trailing unauthenticated bytes can change without a sector checksum
    // failure: Phase18's SHA-256 optimistic lock must still detect stale input.
    auto stale=original;
    stale.back()^=0x01;
    h168::Info parsed{};
    assert(rogue18::validate(stale.data(),stale.size(),parsed));
    assert(!rogue18::propose(stale.data(),stale.size(),money).ok);
    assert(!rogue18::verifyExact(stale.data(),stale.size(),
                                 candidate.candidate.data(),candidate.candidate.size(),money));
    assert(original.size()==h168::Size && original.back()!=stale.back());
    std::cout<<"PASS: current Phase18-to-Phase19 read-only contract; invalid copies fail closed; unsupported edit gates remain closed\n";
}
