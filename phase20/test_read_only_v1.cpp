// SPDX-License-Identifier: GPL-3.0-or-later
// Non-playable synthetic Phase18 -> Phase19 -> Phase20 + Phase17 public ABI.
// No installed games, no external ROM, no private save, no write service.
#include "phase20/read_only_v1.hpp"
// Full research model stays HOST-ONLY, never in the Phase20 ARM11 entry point.
#include "phase18/integration_v2.hpp"
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
    // Independently cross-check the lightweight native DTO against the full
    // Phase18 host-only model, including versioned source fingerprint.
    const auto research=rogue18::inspectReadOnly(source.data(),source.size(),false);
    assert(research.valid && research.apiVersion==rogue18::ModelApiVersion);
    assert(research.money==summary.money &&
           research.activeCounter==summary.activeCounter &&
           research.party.size()==summary.party && research.items.size()==summary.items);
    assert(research.sourceSha256==summary.sourceSha256);
    for(size_t i=0;i<research.party.size();++i) {
        const auto& a=research.party[i].value;
        const auto& b=displayed.party[i];
        assert(a.species==b.species && a.natureFromPid<=24 &&
               a.effectiveNature==b.nature && a.abilitySlot==b.ability);
        for(unsigned j=0;j<6;j++)
            assert(a.ivs[j]==b.iv[j] && a.evs[j]==b.ev[j]);
    }
    for(size_t i=0;i<research.items.size();++i)
        assert(research.items[i].itemId==displayed.bag[i].id &&
               research.items[i].quantity==displayed.bag[i].quantity);


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

    // Host-only Phase18 V2 offline experimental RAM proposal: never link
    // this API into the 3DS native UI or any installed-title save writer.
    static_assert(rogue18::v2::ContractVersion==2u,"Phase18 v2 version changed");
    const auto previewSource=rogue18::v2::inspect(source.data(),source.size());
    assert(previewSource.data.valid && previewSource.data.sourceSha256==summary.sourceSha256);
    rogue18::Request moneyIntent{};
    assert(rogue18::v2::begin(previewSource,rogue18::Kind::Money,0,moneyIntent));
    assert(!rogue18::v2::mayRestoreInstalledGame(moneyIntent));
    moneyIntent.money=moneyIntent.expectedMoney+1u;
    const auto candidate=rogue18::v2::preview(source.data(),source.size(),moneyIntent);
    assert(candidate.ok && candidate.candidate.size()==source.size());
    assert(rogue18::v2::verify(source.data(),source.size(),
                              candidate.candidate.data(),candidate.candidate.size(),moneyIntent));
    static phase19::Demo candidateDisplay{};
    phase20::CopyObservation candidateSummary{};
    assert(phase20::inspectCopy(candidate.candidate.data(),candidate.candidate.size(),
                                candidateDisplay,candidateSummary,reason));
    assert(candidateSummary.money==summary.money+1u);
    assert(!candidateSummary.canRestore && !candidateSummary.canModifyInstalledGame);
    assert(source.size()==phase20::CopiedRawBytes && summary.money==40001);
    for(auto kind: {rogue18::Kind::Nature,rogue18::Kind::AbilitySlot,
                    rogue18::Kind::InsertItem}) {
        const auto cap=rogue18::v2::capability(kind);
        assert(!cap.canCreateOfflineDraft && !cap.canRestoreInstalledGame);
    }
    // A changed move ID remains unsupported in V2 even when it looks numeric.
    rogue18::Request moveIntent{};
    assert(rogue18::v2::beginMove(previewSource,0,0,moveIntent));
    moveIntent.moveId=uint16_t(moveIntent.expectedMoveId+1u);
    assert(!rogue18::v2::preview(source.data(),source.size(),moveIntent).ok);
    assert(!rogue18::v2::mayRestoreInstalledGame(moveIntent));

    // Every malformed size and all 28 checksummed sectors must fail closed:
    // previous UI snapshot AND summary remain bit-for-bit unchanged.
    const auto beforeDigest=summary.sourceSha256;
    const unsigned beforeMoney=displayed.money;
    const unsigned beforeParty=displayed.party[0].species;

    // Null pointer and zero length checks
    assert(!phase20::inspectCopy(nullptr,source.size(),displayed,summary,reason));
    assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);
    assert(!phase20::inspectCopy(source.data(),0,displayed,summary,reason));
    assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);
    assert(!phase20::inspectCopy(nullptr,0,displayed,summary,reason));
    assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);

    for(size_t length: {size_t(0),size_t(1),size_t(512),size_t(4096),size_t(65536),
                        size_t(131071),size_t(131073),size_t(263168),size_t(526400)}) {
        auto bad=source;bad.resize(length,0);
        assert(!phase20::inspectCopy(bad.data(),bad.size(),displayed,summary,reason));
        assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);
    }
    for(unsigned sector=0;sector<28;sector++) {
        auto bad=source;bad[sector*rogue18::raw_v1::Sector+0x350]^=0x40;
        assert(!phase20::inspectCopy(bad.data(),bad.size(),displayed,summary,reason));
        assert(displayed.money==beforeMoney && displayed.party[0].species==beforeParty);
        assert(summary.sourceSha256==beforeDigest);

        // Also test direct sector checksum corruption at footer 0xFF6
        auto badCsum=source;badCsum[sector*rogue18::raw_v1::Sector+0xff6]^=0xff;
        assert(!phase20::inspectCopy(badCsum.data(),badCsum.size(),displayed,summary,reason));
        assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);
    }

    // Corrupted Pokemon data: partyCount > 6 fails closed
    {
        auto bad=source;
        h168::Info info{};
        assert(rogue18::validate(bad.data(),bad.size(),info));
        const size_t s1=info.banks[info.active].sector[1];
        bad[s1+0x234]=7;
        assert(!phase20::inspectCopy(bad.data(),bad.size(),displayed,summary,reason));
        assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);
    }

    // Corrupted Pokemon data: invalid party Pokemon internal checksum
    {
        auto bad=source;
        h168::Info info{};
        assert(rogue18::validate(bad.data(),bad.size(),info));
        const size_t s1=info.banks[info.active].sector[1];
        bad[s1+0x238+28]^=0xff;
        assert(!phase20::inspectCopy(bad.data(),bad.size(),displayed,summary,reason));
        assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);
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

    // Stream tests: valid stream
    std::unique_ptr<std::FILE,int(*)(std::FILE*)> input(std::tmpfile(),&std::fclose);
    assert(input);
    assert(std::fwrite(source.data(),1,source.size(),input.get())==source.size());
    assert(std::fflush(input.get())==0);
    assert(phase20::inspectStream(input.get(),displayed,summary,reason));
    assert(std::fseek(input.get(),0,SEEK_SET)==0);
    std::vector<uint8_t> after(source.size());
    assert(std::fread(after.data(),1,after.size(),input.get())==after.size());
    assert(after==source);

    // Stream tests: null stream
    assert(!phase20::inspectStream(nullptr,displayed,summary,reason));
    assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);

    // Stream tests: empty file (0 bytes)
    std::unique_ptr<std::FILE,int(*)(std::FILE*)> emptyStream(std::tmpfile(),&std::fclose);
    assert(emptyStream);
    assert(!phase20::inspectStream(emptyStream.get(),displayed,summary,reason));
    assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);

    // Stream tests: truncated file (short read)
    std::unique_ptr<std::FILE,int(*)(std::FILE*)> truncStream(std::tmpfile(),&std::fclose);
    assert(truncStream);
    std::vector<uint8_t> shortBytes(512,0xaa);
    assert(std::fwrite(shortBytes.data(),1,shortBytes.size(),truncStream.get())==shortBytes.size());
    assert(std::fflush(truncStream.get())==0);
    assert(!phase20::inspectStream(truncStream.get(),displayed,summary,reason));
    assert(displayed.money==beforeMoney && summary.sourceSha256==beforeDigest);

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
