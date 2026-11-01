// SPDX-License-Identifier: GPL-3.0-or-later
// Purely synthetic, non-playable SD-copy editor integration test.
// Only uses temporary host files; NEVER writes an installed title.
#include "phase20/offline_edit_v1.hpp"
// Host-only v3 contract verification; Phase20 native UI remains single-edit.
#include "phase18/multi_export_v3.hpp"
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
#include <new>
#include <stdexcept>
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
struct HostExportCtx {
    const std::vector<uint8_t>* source;
    fs::path root;
    fs::path import;
};
static rogue18::ExportResult hostSubmit(
    const phase19::Demo& display,const phase20::CopyObservation& captured,
    const phase20::EditChoice& choice,void* ctx) {
    const auto& c=*static_cast<const HostExportCtx*>(ctx);
    return phase20::exportGuarded(c.source->data(),c.source->size(),
        display,captured,choice,c.import.c_str(),c.root.c_str());
}
static rogue18::ExportResult failAllocation(
    const phase19::Demo&,const phase20::CopyObservation&,
    const phase20::EditChoice&,void*) {
    throw std::bad_alloc{};
}
static rogue18::ExportResult failUnexpected(
    const phase19::Demo&,const phase20::CopyObservation&,
    const phase20::EditChoice&,void*) {
    throw std::runtime_error("simulated exporter exception");
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
        {OfflineKind::EV,0,0,display.party[0].ev[0]+1u},
        {OfflineKind::PP,0,0,display.party[0].pp[0]>0?
                                         display.party[0].pp[0]-1u:1u}
    };
    unsigned completed=0;
    for(const auto& choice:examples){
        assert(choice.kind!=OfflineKind::EV ||
               choice.value!=display.party[0].ev[0]);
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
    assert(completed==5u); // every supported class gets a real distinct draft
    // The Phase18 v3 transaction is an ADDITIVE offline contract. Prove that
    // the existing Phase20 imported snapshot can generate one combined draft
    // without entering any installed-game API or replacing the v2 UI.
    static_assert(rogue18::v3::ContractVersion==3u,"v3 API changed");
    const auto v3Source=rogue18::v2::inspect(original.data(),original.size());
    assert(v3Source.data.valid && v3Source.data.sourceSha256==captured.sourceSha256);
    rogue18::v3::Batch batch{};
    assert(rogue18::v3::start(v3Source,batch));
    assert(!rogue18::v3::mayWriteInstalledGame(batch));
    assert(rogue18::v3::append(v3Source,batch,rogue18::Kind::Money,
                                0,0,40002));
    assert(rogue18::v3::append(v3Source,batch,rogue18::Kind::ExistingQuantity,
                                0,0,display.bag[0].quantity+1u));
    assert(rogue18::v3::append(v3Source,batch,rogue18::Kind::IVs,
                                0,0,(display.party[0].iv[0]+1u)%32u));
    assert(rogue18::v3::append(v3Source,batch,rogue18::Kind::EVs,
                                0,3,display.party[0].ev[3]-1u));
    assert(rogue18::v3::append(v3Source,batch,rogue18::Kind::Move,
                                0,0,display.party[0].pp[0]+1u));
    assert(batch.edits.size()==5u);
    assert(!rogue18::v3::append(v3Source,batch,rogue18::Kind::Money,
                                 0,0,40003)); // duplicate logical field
    const auto v3Candidate=rogue18::v3::preview(original.data(),original.size(),batch);
    assert(v3Candidate.ok && v3Candidate.candidate.size()==original.size());
    assert(rogue18::v3::verify(original.data(),original.size(),
                              v3Candidate.candidate.data(),v3Candidate.candidate.size(),
                              batch));
    const auto batchExport=rogue18::v3::exportCopiedBatch(
        original.data(),original.size(),input.c_str(),root.c_str(),batch);
    assert(batchExport.ok && batchExport.changed>0);
    assert(readBytes(input)==original);
    assert(readBytes(root/"backups"/batchExport.backupName)==original);
    const auto combinedDraft=readBytes(root/"drafts"/batchExport.draftName);
    assert(combinedDraft==v3Candidate.candidate);
    assert(rogue18::v3::verify(original.data(),original.size(),
                              combinedDraft.data(),combinedDraft.size(),batch));
    static phase19::Demo combinedDisplay{};
    CopyObservation combinedCaptured{};
    assert(inspectCopy(combinedDraft.data(),combinedDraft.size(),
                       combinedDisplay,combinedCaptured,message));
    assert(combinedDisplay.money==40002 &&
           combinedDisplay.bag[0].quantity==display.bag[0].quantity+1u &&
           combinedDisplay.party[0].iv[0]==(display.party[0].iv[0]+1u)%32u &&
           combinedDisplay.party[0].ev[3]==display.party[0].ev[3]-1u &&
           combinedDisplay.party[0].pp[0]==display.party[0].pp[0]+1u);
    assert(!combinedCaptured.canModifyInstalledGame && !combinedCaptured.canRestore);

    // Every request is fingerprint pinned to the first displayed import.
    auto stale=original;
    stale.back()^=0x20; // normally unchecksummed trailing sector
    rogue18::Request request{};
    assert(!requestForCopy(stale.data(),stale.size(),display,captured,
                           examples[0],request,message));
    assert(!exportGuarded(stale.data(),stale.size(),display,captured,
                          examples[0],input.c_str(),root.c_str()).ok);
    assert(readBytes(input)==original);
    // Missing and truncated SD import must fail closed without a new draft.
    fs::remove(input);
    assert(!exportGuarded(original.data(),original.size(),display,captured,
                          examples[0],input.c_str(),root.c_str()).ok);
    writeBytes(input,std::vector<uint8_t>(131071u,0));
    assert(!exportGuarded(original.data(),original.size(),display,captured,
                          examples[0],input.c_str(),root.c_str()).ok);
    writeBytes(input,original);
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
    assert(readBytes(input)==original); // no draft on cancelled second confirmation
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A));
    assert(flow.stage==EditStage::ConfirmTwo);
    HostExportCtx host{&original,root,input};
    assert(takeOfflineKey(state,flow,captured,phase19::Key::A,
                          &hostSubmit,&host));
    assert(flow.stage==EditStage::Result && flow.succeeded);
    assert(std::string(flow.result).find("OFFLINE COPY ONLY")!=std::string::npos);
    assert(readBytes(input)==original);
    assert(takeOfflineKey(state,flow,captured,phase19::Key::B));
    assert(flow.stage==EditStage::Idle);
    assert(state.screen==phase19::Screen::Money);
    // Fault-injected failures must not escape into native console event loop.
    // They MUST NOT report a successful draft or change the source.
    for(const auto callback:{&failAllocation,&failUnexpected}) {
        flow=OfflineFlow{};
        flow.stage=EditStage::ConfirmTwo;
        flow.choice={OfflineKind::Money,0,0,display.money+1};
        flow.original=display.money;
        flow.proposed=display.money+1;
        assert(takeOfflineKey(state,flow,captured,phase19::Key::A,
                              callback,nullptr));
        assert(flow.stage==EditStage::Result && !flow.succeeded);
        assert(std::string(flow.result).find("EXPORT")!=std::string::npos ||
               std::string(flow.result).find("OUT OF MEMORY")!=std::string::npos);
        assert(readBytes(input)==original);
        assert(takeOfflineKey(state,flow,captured,phase19::Key::B));
        assert(flow.stage==EditStage::Idle);
    }
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
    // Additional QA Coverage (Assignment 2):
    // 1. Min/max boundary checks across all 5 offline edit operations
    // Money: min 0, max 999999
    {
        rogue18::Request rMin{}, rMax{};
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::Money,0,0,0},rMin,message));
        assert(rMin.money==0);
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::Money,0,0,999999},rMax,message));
        assert(rMax.money==999999);
    }
    // ExistingItem: min 1, max 999
    {
        rogue18::Request rMin{}, rMax{};
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::ExistingItem,0,0,1},rMin,message));
        assert(rMin.quantity==1);
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::ExistingItem,0,0,999},rMax,message));
        assert(rMax.quantity==999);
    }
    // IV: min 0, max 31
    {
        rogue18::Request rMin{}, rMax{};
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::IV,0,0,0},rMin,message));
        assert(rMin.stats[0]==0);
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::IV,0,0,31},rMax,message));
        assert(rMax.stats[0]==31);
    }
    // EV: min 0, max bounded by 252 and total 510
    {
        rogue18::Request rMin{}, rMax{};
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::EV,0,0,0},rMin,message));
        assert(rMin.stats[0]==0);
        unsigned other=0;
        for(unsigned j=1;j<6;j++) other+=display.party[0].ev[j];
        unsigned maxEv=std::min(252u,510u-other);
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::EV,0,0,maxEv},rMax,message));
        assert(rMax.stats[0]==maxEv);
    }
    // PP: min 0, max 64
    {
        rogue18::Request rMin{}, rMax{};
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::PP,0,0,0},rMin,message));
        assert(rMin.pp==0);
        assert(requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::PP,0,0,64},rMax,message));
        assert(rMax.pp==64);
    }

    // 2. Invalid party slots, missing Pokemon, empty move slots and invalid item entries
    {
        rogue18::Request rBad{};
        // Out-of-bounds party slot
        assert(!requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::IV,display.partyCount,0,10},rBad,message));
        assert(!requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::EV,display.partyCount,0,10},rBad,message));
        assert(!requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::PP,display.partyCount,0,10},rBad,message));
        // Empty move slot check
        for(unsigned m=0;m<4;m++) {
            if(display.party[0].moves[m]==0) {
                assert(!requestForCopy(original.data(),original.size(),display,captured,
                                      {OfflineKind::PP,0,m,10},rBad,message));
            }
        }
        // Out-of-bounds bag slot
        assert(!requestForCopy(original.data(),original.size(),display,captured,
                              {OfflineKind::ExistingItem,display.bagCount,0,5},rBad,message));
    }

    // 3. UI flow: cancellation at ConfirmOne, ConfirmTwo, and reset via X
    {
        state.screen=phase19::Screen::Money;
        OfflineFlow testFlow{};
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::A));
        assert(testFlow.stage==EditStage::Value);
        // Up (+1)
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::Up));
        assert(testFlow.proposed==testFlow.original+1);
        // X resets to original
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::X));
        assert(testFlow.proposed==testFlow.original);
        // Up again (+1) and proceed to ConfirmOne
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::Up));
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::A));
        assert(testFlow.stage==EditStage::ConfirmOne);
        // Cancel with B returns to Value
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::B));
        assert(testFlow.stage==EditStage::Value);
        // Re-advance to ConfirmTwo
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::A));
        assert(testFlow.stage==EditStage::ConfirmOne);
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::A));
        assert(testFlow.stage==EditStage::ConfirmTwo);
        // Cancel with B returns to Value
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::B));
        assert(testFlow.stage==EditStage::Value);
        // Cancel with B returns to Idle
        assert(takeOfflineKey(state,testFlow,captured,phase19::Key::B));
        assert(testFlow.stage==EditStage::Idle);
    }

    fs::remove_all(root);
    std::cout<<"PASS Phase20 offline UI intent, SHA-lock, source-safe backup/draft and guarded export\n";
}
