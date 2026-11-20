// SPDX-License-Identifier: GPL-3.0-or-later
// Phase20 Pokémon detail tabs: presentation only, no mutations, file or
// installed-game access. Source identity guards against stale selections.
#pragma once
#include "ui.hpp"
#include "move_browser.hpp"
#include <array>
#include <cstdint>
namespace phase20 { namespace profile {
enum class Tab : uint8_t { Overview=0, Training=1, Moves=2 };
inline Tab fromIndex(unsigned index){
    return index<3u?static_cast<Tab>(index):Tab::Overview;
}
inline Tab cycle(Tab current,bool forward){
    const unsigned index=static_cast<unsigned>(current);
    if(index>2u)return Tab::Overview;
    return fromIndex((index+(forward?1u:2u))%3u);
}
inline const char* natureName(unsigned nature){
    static constexpr const char* names[25]={
        "HARDY","LONELY","BRAVE","ADAMANT","NAUGHTY",
        "BOLD","DOCILE","RELAXED","IMPISH","LAX",
        "TIMID","HASTY","SERIOUS","JOLLY","NAIVE",
        "MODEST","MILD","QUIET","BASHFUL","RASH",
        "CALM","GENTLE","SASSY","CAREFUL","QUIRKY"
    };
    return nature<25u?names[nature]:"UNVERIFIED";
}
// Slot number is stored in the Pokémon; the ability ID/name is normally
// resolved through exact-ROM species, form, revision-mode and OT tables.
inline const char* abilitySlotLabel(unsigned slot){
    switch(slot){
        case 0:return "SLOT 1";
        case 1:return "SLOT 2";
        case 2:return "SLOT 3";
        default:return "INVALID SLOT";
    }
}
inline unsigned evTotal(const phase19::Pokemon& mon){
    unsigned total=0;
    for(unsigned v:mon.ev)total+=v;
    return total;
}
inline const char* knownMoveName(const phase19::MoveCatalog* catalog,
                                bool synthetic,unsigned moveId){
    if(!catalog || !catalog->records || !moveId ||
       catalog->count>phase19::MaxMoveRecords ||
       !catalog->count ||
       (synthetic && catalog->origin!=phase19::CatalogOrigin::Synthetic) ||
       (!synthetic && catalog->origin!=phase19::CatalogOrigin::ExactRomReviewed))
        return nullptr;
    // Caller only supplies a previously authenticated catalog (SD provider)
    // or the app's built-in synthetic demonstration, never a guessed ID.
    for(unsigned i=0;i<catalog->count;i++) {
        const auto& record=catalog->records[i];
        if(record.id==moveId &&
           phase19::validMoveName(record.name,sizeof(record.name)))
            return record.name;
    }
    return nullptr;
}
struct View {
    Tab tab=Tab::Overview;
    bool pinned=false,pc=false,synthetic=false;
    unsigned box=0,mon=0,species=0,ability=0,nature=0,counter=0;
    std::array<uint8_t,32> digest{};
    void reset(){*this=View{};}
    void sync(const phase19::State& s){
        if(s.screen!=phase19::Screen::Pokemon ||
           !phase19::selectedPokemonValid(s)) {reset();return;}
        const auto& p=phase19::pokemon(s);
        if(!pinned || pc!=s.fromPC || box!=s.box || mon!=s.mon ||
           species!=p.species || ability!=p.ability || nature!=p.nature ||
           counter!=s.demo.activeCounter || digest!=s.demo.sourceSha256 ||
           synthetic!=s.demo.synthetic){
            tab=Tab::Overview;
            pc=s.fromPC;box=s.box;mon=s.mon;species=p.species;
            ability=p.ability;nature=p.nature;counter=s.demo.activeCounter;
            digest=s.demo.sourceSha256;synthetic=s.demo.synthetic;
            pinned=true;
        }
    }
};
constexpr bool DetailTabsWriteInstalledSave=false;
static_assert(!DetailTabsWriteInstalledSave,"UI tabs must never enable game-save writing");
}} // namespace phase20::profile
