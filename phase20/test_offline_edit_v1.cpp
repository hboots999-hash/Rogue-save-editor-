// SPDX-License-Identifier: GPL-3.0-or-later
// Purely synthetic, non-playable SD-copy editor integration test.
// Only uses temporary host files; NEVER writes an installed title.
#include "phase20/offline_edit_v1.hpp"
#define main phase18_fixture_entry_for_phase20
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "phase18/test_offline_engine.cpp"
#pragma GCC diagnostic pop
#undef main
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
namespace fs=std::filesystem;

static std::vector<uint8_t> readBytes(const fs::path& path){
    std::ifstream f(path,std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});
}
static void writeBytes(const fs::path& path,const std::vector<uint8_t>& bytes){
    std::ofstream f(path,std::ios::binary|std::ios::trunc);
    f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    f.close();assert(f.good());
}
int main(){
    using namespace phase20;
    const auto original=fixture();
    static phase19::Demo display{};
    CopyObservation captured{};
    std::string message;
    assert(inspectCopy(original.data(),original.size(),display,captured,message));
    assert(!display.synthetic && !captured.canModifyInstalledGame);
    fs::path root=fs::temp_directory_path()/"pksm_phase20_synthetic_editor";
    fs::remove_all(root);
    fs::create_directories(root/"imports");
    const fs::path input=root/"imports"/"00000001.sav";
    writeBytes(input,original);
    const EditChoice examples[]={
        {OfflineKind::Money,0,0,40002},
        {OfflineKind::ExistingItem,0,0,display.bag[0].quantity+1u},
        {OfflineKind::IV,0,0,(display.party[0].iv[0]+1u)%32u},
        {OfflineKind::EV,0,0,0},
        {OfflineKind::PP,0,0,display.party[0].pp[0]>0?
                                         display.party[0].pp[0]-1u:1u}
    };
    unsigned completed=0;
    for(const auto& choice:examples){
        if(choice.kind==OfflineKind::EV &&
           choice.value==display.party[0].ev[0])continue;
        rogue18::Request request{};
        assert(requestForCopy(original.data(),original.size(),
                              display,captured,choice,request,message));
        assert(!rogue18::v2::mayRestoreInstalledGame(request));
        const auto output=exportGuarded(original.data(),original.size(),
                     display,captured,choice,input.c_str(),root.c_str());
        assert(output.ok && output.changed>0);
        assert(readBytes(input)==original);
        assert(readBytes(root/"backups"/output.backupName)==original);
        const auto draft=readBytes(root/"drafts"/output.draftName);
        assert(draft.size()==original.size());
        assert(rogue18::v2::verify(original.data(),original.size(),
                                  draft.data(),draft.size(),request));
        assert(!output.draftName.empty() && !output.backupName.empty());
        completed++;
    }
    assert(completed>=4u);
    // Every request is fingerprint pinned to the first displayed import.
    auto stale=original;
    stale.back()^=0x20; // normally unchecksummed trailing sector
    rogue18::Request request{};
    assert(!requestForCopy(stale.data(),stale.size(),display,captured,
                           examples[0],request,message));
    assert(!exportGuarded(stale.data(),stale.size(),display,captured,
                          examples[0],input.c_str(),root.c_str()).ok);
    assert(readBytes(input)==original);
    // The exporter re-reads the actual fixed source and refuses changed input.
    writeBytes(input,stale);
    assert(!exportGuarded(original.data(),original.size(),display,captured,
                          examples[0],input.c_str(),root.c_str()).ok);
    writeBytes(input,original);
    assert(!exportGuarded(original.data(),original.size(),display,captured,
                          examples[0],(root/"backups"/"fake.sav").c_str(),
                          root.c_str()).ok);
    for(auto bad: {EditChoice{OfflineKind::Money,0,0,1000000},
                   EditChoice{OfflineKind::ExistingItem,500,0,2},
                   EditChoice{OfflineKind::IV,0,6,23},
                   EditChoice{OfflineKind::EV,0,0,253},
                   EditChoice{OfflineKind::PP,0,4,5},
                   EditChoice{OfflineKind::Money,0,0,display.money}}){
        assert(!requestForCopy(original.data(),original.size(),display,
                               captured,bad,request,message));
    }
    // Changing the snapshot while preserving the counter must fail closed.
    const unsigned moneyBefore=display.money;
    display.money++;
    assert(!requestForCopy(original.data(),original.size(),display,captured,
                           examples[0],request,message));
    display.money=moneyBefore;
    // Untrusted demo and user-toggled synthetic state cannot export a draft.
    display.synthetic=true;
    assert(!requestForCopy(original.data(),original.size(),display,captured,
                           examples[0],request,message));
    display.synthetic=false;
    auto denied=captured;
    denied.canRestore=true;
    assert(!requestForCopy(original.data(),original.size(),display,denied,
                           examples[0],request,message));

    // Phase19 read-only model is kept unchanged. Only the Phase20 intercept
    // enters separate offline flow for already validated imported sources.
    phase19::State state(display);
    state.homeRow=3;
    phase19::press(state,phase19::Key::A); // money view
    OfflineFlow flow{};
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    assert(flow.stage==EditStage::Value && flow.original==display.money);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::Up));
    assert(flow.proposed==display.money+1u);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    assert(flow.stage==EditStage::ConfirmOne);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::B));
    assert(flow.stage==EditStage::Value);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    assert(flow.stage==EditStage::ConfirmTwo);
    assert(!flow.succeeded);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::B));
    assert(flow.stage==EditStage::Value);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::B));
    assert(flow.stage==EditStage::Idle);
    assert(state.screen==phase19::Screen::Money);
    state.demoPreviewEnabled=true;
    assert(!takeOfflineKey(state,flow,captured,phase19::Key::A));
    state.demoPreviewEnabled=false;
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    flow=OfflineFlow{};
    // Party category/field selection accepts IV, EV, PP only, without L.
    state.screen=phase19::Screen::Pokemon;
    state.mon=0;state.fromPC=false;
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    assert(flow.stage==EditStage::Category);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::R));
    assert(flow.category==1);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    assert(flow.stage==EditStage::Field && flow.choice.kind==OfflineKind::EV);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::B));
    assert(flow.stage==EditStage::Category);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::B));
    assert(flow.stage==EditStage::Idle);
    assert(readBytes(input)==original);
    fs::remove_all(root);
    std::cout<<"PASS Phase20 offline UI intent, SHA-lock, source-safe backup/draft and guarded export\n";
}
