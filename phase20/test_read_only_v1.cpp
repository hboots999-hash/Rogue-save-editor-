// SPDX-License-Identifier: GPL-3.0-or-later
// Non-playable synthetic Phase18 -> Phase19 -> Phase20 + Phase17 public ABI.
// No installed games, no external ROM, no private save, no write service.
#include "phase20/read_only_v1.hpp"
#define main phase18_test_fixture_main
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "phase18/test_offline_engine.cpp"
#pragma GCC diagnostic pop
#undef main
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    auto source=fixture();
    assert(source.size()==phase20::CopiedRawBytes);
    static phase19::Demo displayed{};
    phase20::CopyObservation summary{};
    std::string reason;
    assert(phase20::inspectCopy(source.data(),source.size(),displayed,summary,reason));
    assert(summary.structurallyReadable && !summary.romIdentityVerified);
    assert(!summary.canModifyInstalledGame && !summary.canRestore);
    assert(!displayed.synthetic && displayed.sourceModelApi==rogue18::ModelApiVersion);
    assert(summary.money==displayed.money && summary.money==40001);
    assert(summary.party==2 && summary.items==3);
    assert(summary.sourceSha256==rogue18::sourceDigest(source.data(),source.size()));
    assert(summary.activeCounter==41 && displayed.activeCounter==41);

    phase19::State ui(displayed);
    ui.homeRow=3;
    phase19::press(ui,phase19::Key::A);
    phase19::press(ui,phase19::Key::A);
    assert(ui.screen==phase19::Screen::Notice); // no money editing
    assert(!ui.demoPreviewEnabled);
    ui.demoPreviewEnabled=true; // a faulty future caller cannot override imported flag
    phase19::press(ui,phase19::Key::B);
    phase19::press(ui,phase19::Key::A);
    assert(ui.screen==phase19::Screen::Notice);
    assert(!phase19::canPreview(ui));
    ui.homeRow=2;
    phase19::switchView(ui,false);
    phase19::press(ui,phase19::Key::A); // party
    phase19::press(ui,phase19::Key::A); // first Pokémon
    phase19::press(ui,phase19::Key::A); // edit attempt
    assert(ui.screen==phase19::Screen::Notice);

    // Every malformed size and all 28 checksummed sectors must fail closed:
    // previous UI snapshot AND summary remain bit-for-bit unchanged.
    const auto beforeDigest=summary.sourceSha256;
    const unsigned beforeMoney=displayed.money;
    const unsigned beforeParty=displayed.party[0].species;
    for(size_t length: {size_t(0),size_t(1),size_t(131071),
                        size_t(131073),size_t(263168)}) {
        auto bad=source;bad.resize(length,0);
        assert(!phase20::inspectCopy(bad.data(),bad.size(),displayed,summary,reason));
        assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);
    }
    for(unsigned sector=0;sector<28;sector++) {
        auto bad=source;bad[sector*rogue18::raw_v1::Sector+0x350]^=0x40;
        assert(!phase20::inspectCopy(bad.data(),bad.size(),displayed,summary,reason));
        assert(displayed.money==beforeMoney && displayed.party[0].species==beforeParty);
        assert(summary.sourceSha256==beforeDigest);
    }
    // The 4 unchecksummed trailing sectors still contribute to the digest.
    auto stale=source;
    stale.back()^=1u;
    assert(phase20::inspectCopy(stale.data(),stale.size(),displayed,summary,reason));
    assert(summary.sourceSha256!=beforeDigest);
    assert(!summary.canRestore && !summary.canModifyInstalledGame);
    assert(rogue18::sourceDigest(source.data(),source.size())==beforeDigest);
    // Restore display to the validated original for remaining tests.
    assert(phase20::inspectCopy(source.data(),source.size(),displayed,summary,reason));

    std::unique_ptr<std::FILE,int(*)(std::FILE*)> input(std::tmpfile(),&std::fclose);
    assert(input);
    assert(std::fwrite(source.data(),1,source.size(),input.get())==source.size());
    assert(std::fflush(input.get())==0);
    assert(phase20::inspectStream(input.get(),displayed,summary,reason));
    assert(std::fseek(input.get(),0,SEEK_SET)==0);
    std::vector<uint8_t> after(source.size());
    assert(std::fread(after.data(),1,after.size(),input.get())==after.size());
    assert(after==source);
    // Wrapped container is not auto-trimmed into a candidate.
    std::unique_ptr<std::FILE,int(*)(std::FILE*)> wrapped(std::tmpfile(),&std::fclose);
    assert(wrapped);
    std::vector<uint8_t> wrapper(phase20::AuthenticatedContainerBytes,0);
    assert(std::fwrite(wrapper.data(),1,wrapper.size(),wrapped.get())==wrapper.size());
    assert(std::fflush(wrapped.get())==0);
    assert(!phase20::inspectStream(wrapped.get(),displayed,summary,reason));
    assert(summary.sourceSha256==beforeDigest);

    // Phase17 C ABI reports cannot turn into write or restore permissions.
    P17ReadOnlyStatusV1 report{};
    assert(p17_readonly_status_init_blocked_v1(&report));
    assert(p17_readonly_status_shape_valid_v1(&report));
    auto observation=phase20::inspectRecoveryStatus(&report);
    assert(observation.level==phase20::RecoveryLevel::NotChecked);
    assert(!observation.canRestore && !observation.canModifyInstalledGame);
    report.capability_bits=1; // malicious/stale envelope
    observation=phase20::inspectRecoveryStatus(&report);
    assert(observation.level==phase20::RecoveryLevel::InvalidEnvelope);
    report.capability_bits=0;
    report.code=P17_RO_RAM_REHEARSAL_READABLE;
    observation=phase20::inspectRecoveryStatus(&report);
    assert(observation.level==phase20::RecoveryLevel::InvalidEnvelope);
    report.expected_title=0x0004000007D7BA00ull;
    report.container_bytes=phase20::AuthenticatedContainerBytes;
    report.journal_bytes=phase20::RehearsalJournalBytes;
    report.source_sha256[0]=1; // synthetic fake hash, NOT authenticated by Phase20
    observation=phase20::inspectRecoveryStatus(&report);
    assert(observation.level==phase20::RecoveryLevel::RamRehearsalClaimOnly);
    assert(!observation.canRestore && !observation.canModifyInstalledGame);
    assert(!phase20::inspectRecoveryStatus(nullptr).canRestore);
    std::cout<<"PASS Phase20 synthetic read-only integration, guarded SD copy, dual ABI and hard no-write gates\n";
}
