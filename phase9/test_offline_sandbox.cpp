// Offline synthetic-only exporter integration test. Never uses a real game save.
#include "offline_sandbox.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
namespace fs=std::filesystem;
using Bytes=std::vector<uint8_t>;
static Bytes read(const fs::path& p) {
    std::ifstream f(p,std::ios::binary); assert(f);
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
static void write(const fs::path& p,const Bytes& b) {
    std::ofstream f(p,std::ios::binary|std::ios::trunc);assert(f);
    f.write(reinterpret_cast<const char*>(b.data()),b.size());assert(f.good());
}
static fs::path setup(const fs::path& parent,const char* name,const Bytes& original){
    const fs::path root=parent/name;
    fs::create_directories(root/"imports");
    write(root/"imports/00000001.sav",original);
    return root;
}
static rogue_sandbox::Result run(const fs::path& root,const Bytes& initial,
                                 rogue_sandbox::Edit action,unsigned slot=0){
    return rogue_sandbox::exportOne(initial.data(),
             (root/"imports/00000001.sav").c_str(),root.c_str(),action,slot);
}
int main(int argc,char** argv){
    assert(argc==2);
    const Bytes initial=read(argv[1]);
    assert(initial.size()==ROGUE_RAW_SAVE_SIZE);
    assert(rogue_sandbox::exactSynthetic(initial.data(),initial.size()));
    auto altered=initial;altered[altered.size()-1]^=0x5a;
    assert(!rogue_sandbox::exactSynthetic(altered.data(),altered.size()));
    assert(!rogue_sandbox::exactSynthetic(initial.data(),initial.size()-1));
    const fs::path temp=fs::temp_directory_path()/
        ("pksm_phase9_2_"+std::to_string(::getpid()));
    assert(!fs::exists(temp));fs::create_directories(temp);

    const auto reject=setup(temp,"real_or_changed",initial);
    auto r=run(reject,altered,rogue_sandbox::Edit::Money);
    assert(!r.ok && !fs::exists(reject/"backups") && !fs::exists(reject/"drafts"));
    write(reject/"imports/00000001.sav",altered);
    r=run(reject,initial,rogue_sandbox::Edit::Money);
    assert(!r.ok && !fs::exists(reject/"backups") && !fs::exists(reject/"drafts"));

    const auto blocked=setup(temp,"backup_failure",initial);
    const Bytes sentinel={'N','O','T','_','A','_','D','I','R'};
    write(blocked/"backups",sentinel);
    r=run(blocked,initial,rogue_sandbox::Edit::Money);
    assert(!r.ok && read(blocked/"backups")==sentinel && !fs::exists(blocked/"drafts"));
    assert(read(blocked/"imports/00000001.sav")==initial);

    const auto good=setup(temp,"positive",initial);
    r=run(good,initial,rogue_sandbox::Edit::Money);
    assert(r.ok && r.changed>0);
    assert(read(good/"backups/import_001.sav")==initial);
    assert(read(good/"imports/00000001.sav")==initial);
    Bytes draft=read(good/"drafts/rogue_001.sav");
    assert(draft!=initial);
    rogue_preview p{};assert(rogue_preview_copy(draft.data(),draft.size(),&p));
    assert(p.money==999999);

    r=run(good,initial,rogue_sandbox::Edit::ExistingItem,0);
    assert(r.ok && r.changed>0);
    assert(read(good/"backups/import_002.sav")==initial);
    assert(read(good/"imports/00000001.sav")==initial);
    draft=read(good/"drafts/rogue_002.sav");
    rogue_item_candidate item{};
    std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
       rogue_snapshot_open(draft.data(),draft.size()),rogue_snapshot_close);
    assert(snap && rogue_snapshot_item(snap.get(),0,&item));
    assert(item.quantity==999 && item.item_id==113);
    r=run(good,initial,rogue_sandbox::Edit::ExistingItem,999);
    assert(!r.ok && !fs::exists(good/"drafts/rogue_003.sav"));

    // Each Pokemon operation starts from the pristine exact toy fixture and
    // writes to a distinct separate draft, leaving the game/SD import intact.
    for(const auto op : {rogue_sandbox::Edit::AllIV, rogue_sandbox::Edit::OffensiveEV,
                         rogue_sandbox::Edit::FirstMove}){
        const auto toy=setup(temp,op==rogue_sandbox::Edit::AllIV?"all_iv":
                               op==rogue_sandbox::Edit::OffensiveEV?"offensive_ev":"move_slot",initial);
        const auto result=run(toy,initial,op,0);
        assert(result.ok && result.changed>0);
        assert(read(toy/"imports/00000001.sav")==initial);
        assert(read(toy/"backups/import_001.sav")==initial);
        const auto bytes=read(toy/"drafts/rogue_001.sav");
        assert(bytes.size()==ROGUE_RAW_SAVE_SIZE && bytes!=initial);
        std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> check(
            rogue_snapshot_open(bytes.data(),bytes.size()),rogue_snapshot_close);
        rogue_party_candidate mon{};
        assert(check && rogue_snapshot_party(check.get(),0,&mon));
        assert(mon.species_id==25);
        if(op==rogue_sandbox::Edit::AllIV){
            for(auto iv:mon.ivs) assert(iv==31);
            assert(mon.moves[0]==33);
        }else if(op==rogue_sandbox::Edit::OffensiveEV){
            const uint8_t spread[]={4,0,0,252,252,0};
            assert(std::memcmp(mon.evs,spread,6)==0);
            assert(mon.ivs[0]==10);
        }else{
            assert(mon.moves[0]==85);
            assert(mon.ivs[0]==10);
        }
        // Attempt to reuse a stale or unknown copy must never create another
        // backup or edited draft.
        auto unknown=initial;unknown[unknown.size()-1]^=0xa5;
        const auto blockedResult=run(toy,unknown,op);
        assert(!blockedResult.ok);
        assert(!fs::exists(toy/"drafts/rogue_002.sav"));
        const auto badSlot=run(toy,initial,op,1);
        assert(!badSlot.ok && !fs::exists(toy/"drafts/rogue_002.sav"));
    }

    // Configurable values: verify the proposal, not just a hard-coded preset.
    struct CustomCase{const char* name;rogue_sandbox::Edit edit;};
    const CustomCase cases[]={
        {"money_custom",rogue_sandbox::Edit::Money},
        {"quantity_custom",rogue_sandbox::Edit::ExistingItem},
        {"iv_custom",rogue_sandbox::Edit::AllIV},
        {"ev_custom",rogue_sandbox::Edit::OffensiveEV},
        {"move_custom",rogue_sandbox::Edit::FirstMove}
    };
    for(const auto& test:cases){
        const auto folder=setup(temp,test.name,initial);
        rogue_sandbox::Values values{};
        if(test.edit==rogue_sandbox::Edit::Money)values.money=654321;
        if(test.edit==rogue_sandbox::Edit::ExistingItem)values.quantity=50;
        if(test.edit==rogue_sandbox::Edit::AllIV)
            for(auto& iv:values.ivs)iv=21;
        if(test.edit==rogue_sandbox::Edit::OffensiveEV){
            const uint8_t ev[]={4,20,0,252,200,0};
            std::memcpy(values.evs,ev,6);
        }
        if(test.edit==rogue_sandbox::Edit::FirstMove){
            values.moveId=77;values.movePP=20;
        }
        assert(rogue_sandbox::valid(test.edit,values));
        const auto result=rogue_sandbox::exportOne(initial.data(),
            (folder/"imports/00000001.sav").c_str(),folder.c_str(),
            test.edit,0,values);
        assert(result.ok && result.changed>0);
        assert(read(folder/"backups/import_001.sav")==initial);
        assert(read(folder/"imports/00000001.sav")==initial);
        const Bytes modified=read(folder/"drafts/rogue_001.sav");
        assert(modified!=initial);
        rogue_preview preview{};
        assert(rogue_preview_copy(modified.data(),modified.size(),&preview));
        if(test.edit==rogue_sandbox::Edit::Money)assert(preview.money==654321);
        else if(test.edit==rogue_sandbox::Edit::ExistingItem){
            rogue_item_candidate found{};
            std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
                rogue_snapshot_open(modified.data(),modified.size()),rogue_snapshot_close);
            assert(snap && rogue_snapshot_item(snap.get(),0,&found));
            assert(found.quantity==50);
        }else{
            rogue_party_candidate mon{};
            std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
                rogue_snapshot_open(modified.data(),modified.size()),rogue_snapshot_close);
            assert(snap && rogue_snapshot_party(snap.get(),0,&mon));
            if(test.edit==rogue_sandbox::Edit::AllIV)
                assert(std::memcmp(mon.ivs,values.ivs,6)==0);
            if(test.edit==rogue_sandbox::Edit::OffensiveEV)
                assert(std::memcmp(mon.evs,values.evs,6)==0);
            if(test.edit==rogue_sandbox::Edit::FirstMove)
                assert(mon.moves[0]==77 && mon.move_pp[0]==20);
        }
    }
    {
        const auto invalid=setup(temp,"invalid_ev",initial);
        rogue_sandbox::Values vals{};
        for(auto& ev:vals.evs)ev=252;
        assert(!rogue_sandbox::valid(rogue_sandbox::Edit::OffensiveEV,vals));
        const auto r=rogue_sandbox::exportOne(initial.data(),
            (invalid/"imports/00000001.sav").c_str(),invalid.c_str(),
            rogue_sandbox::Edit::OffensiveEV,0,vals);
        assert(!r.ok && !fs::exists(invalid/"backups") && !fs::exists(invalid/"drafts"));
        vals=rogue_sandbox::Values{};vals.moveId=0;
        assert(!rogue_sandbox::valid(rogue_sandbox::Edit::FirstMove,vals));
        vals=rogue_sandbox::Values{};vals.ivs[0]=32;
        assert(!rogue_sandbox::valid(rogue_sandbox::Edit::AllIV,vals));
        assert(read(invalid/"imports/00000001.sav")==initial);
    }

    // Insert-item actions reuse the vetted Phase 7 routine; fixture only.
    for(const unsigned desired : {113u,1u}){
        const auto folder=setup(temp,desired==113?"insert_candy":"insert_masterball",initial);
        rogue_sandbox::Values values{};
        values.insertItemId=static_cast<uint16_t>(desired);
        values.quantity=222;
        const auto result=rogue_sandbox::exportOne(initial.data(),
            (folder/"imports/00000001.sav").c_str(),folder.c_str(),
            rogue_sandbox::Edit::InsertItem,0,values);
        assert(result.ok && result.changed>0);
        assert(read(folder/"imports/00000001.sav")==initial);
        assert(read(folder/"backups/import_001.sav")==initial);
        const auto bytes=read(folder/"drafts/rogue_001.sav");
        assert(bytes.size()==ROGUE_RAW_SAVE_SIZE && bytes!=initial);
        rogue_preview parsed{};
        assert(rogue_preview_copy(bytes.data(),bytes.size(),&parsed));
        std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> check(
            rogue_snapshot_open(bytes.data(),bytes.size()),rogue_snapshot_close);
        bool found=false;
        for(unsigned i=0;i<parsed.occupied_bag_slots;i++){
            rogue_item_candidate item{};
            if(check && rogue_snapshot_item(check.get(),i,&item) &&
               item.item_id==desired && item.quantity==222)found=true;
        }
        assert(found);
        auto stale=initial;
        stale[stale.size()-1]^=0x5a;
        const auto denied=rogue_sandbox::exportOne(stale.data(),
            (folder/"imports/00000001.sav").c_str(),folder.c_str(),
            rogue_sandbox::Edit::InsertItem,0,values);
        assert(!denied.ok && !fs::exists(folder/"drafts/rogue_002.sav"));
    }
    // Pre-existing names cannot be replaced. Both exports skip occupied slots.
    const auto occupied=setup(temp,"occupied",initial);
    fs::create_directories(occupied/"backups");
    fs::create_directories(occupied/"drafts");
    write(occupied/"backups/import_001.sav",sentinel);
    write(occupied/"drafts/rogue_001.sav",sentinel);
    r=run(occupied,initial,rogue_sandbox::Edit::Money);
    assert(r.ok && read(occupied/"backups/import_001.sav")==sentinel);
    assert(read(occupied/"drafts/rogue_001.sav")==sentinel);
    assert(read(occupied/"backups/import_002.sav")==initial);
    assert(read(occupied/"drafts/rogue_002.sav")!=initial);

    // If draft directory cannot be created, no draft is written; backup remains.
    const auto draftBlocked=setup(temp,"draft_failure",initial);
    write(draftBlocked/"drafts",sentinel);
    r=run(draftBlocked,initial,rogue_sandbox::Edit::Money);
    assert(!r.ok);
    assert(read(draftBlocked/"drafts")==sentinel);
    assert(read(draftBlocked/"backups/import_001.sav")==initial);
    assert(read(draftBlocked/"imports/00000001.sav")==initial);

    fs::remove_all(temp);
    std::cout<<"PASS synthetic-only offline money/item/IV/EV/move export: byte-exact gate, stale input, backup failure, verified copies, exclusive names, draft I/O refusal\n";
}
