// Phase 19 standalone UI model; GPL-3.0-or-later.
// Deliberately has NO filesystem, save-service, ROM or restoration interface.
#pragma once
#include <array>
#include <cstdint>
#include <algorithm>

namespace phase19 {
enum class Screen : uint8_t {
    Home, Party, PC, Pokemon, Edit, Bag, Money, Backups,
    Proposal, ConfirmOne, ConfirmTwo, Notice
};
enum class Key : uint8_t { Up, Down, Left, Right, A, B, X, L, R, Start };
enum class Group : uint8_t { IV, EV, Moves, Ability, Nature };

struct Pokemon {
    const char* name;
    unsigned species, level;
    std::array<unsigned, 6> iv, ev;
    std::array<unsigned, 4> moves;
    unsigned ability, nature;
};
struct Item { const char* name; unsigned id, quantity; };
struct Demo {
    unsigned money = 42000;
    std::array<Pokemon, 3> party {{
        {"Volcarona",637,72,{{31,10,30,31,31,31}},{{0,0,0,252,4,252}},{{53,483,94,247}},1,10},
        {"Swampert",260,68,{{31,31,31,4,31,25}},{{252,252,4,0,0,0}},{{89,127,182,57}},0,3},
        {"Gardevoir",282,65,{{27,1,24,31,30,31}},{{0,0,4,252,0,252}},{{94,85,247,347}},1,15}
    }};
    std::array<std::array<Pokemon, 3>, 2> pc {{
        {{{"Gengar",94,60,{{31,5,20,31,15,31}},{{0,0,0,252,4,252}},{{94,247,109,399}},0,10},
          {"Lucario",448,62,{{31,31,20,15,25,31}},{{0,252,4,0,0,252}},{{370,245,396,197}},1,3},
          {"Metagross",376,64,{{31,31,31,5,31,10}},{{252,252,4,0,0,0}},{{309,89,428,418}},0,2}}},
        {{{"Garchomp",445,67,{{31,31,31,3,15,31}},{{0,252,4,0,0,252}},{{89,200,242,337}},1,3},
          {"Milotic",350,55,{{31,2,31,31,31,18}},{{252,0,252,4,0,0}},{{57,58,105,392}},0,5},
          {"Breloom",286,56,{{31,31,23,8,20,31}},{{4,252,0,0,0,252}},{{147,183,317,264}},1,3}}}
    }};
    std::array<Item, 5> bag {{
        {"Rare Candy",68,25}, {"Poke Ball",4,50}, {"Ultra Ball",2,30},
        {"Potion",13,18}, {"Repel",86,12}
    }};
};
struct State {
    Screen screen = Screen::Home;
    Screen returnTo = Screen::Home;
    Screen proposalFrom = Screen::Home;
    unsigned homeRow = 0, selected = 0, box = 0, mon = 0;
    bool fromPC = false, exitRequested = false;
    Group group = Group::IV;
    unsigned field = 0;
    unsigned proposal = 0;
    const char* notice = "";
    // Synthetic sample is immutable throughout every transition. No real save is loaded.
    const Demo demo{};
};

inline unsigned count(const State& s) {
    switch(s.screen) {
        case Screen::Home: return 6;
        case Screen::Party: return static_cast<unsigned>(s.demo.party.size());
        case Screen::PC: return static_cast<unsigned>(s.demo.pc[s.box].size());
        case Screen::Bag: return static_cast<unsigned>(s.demo.bag.size());
        case Screen::Backups: return 3;
        case Screen::Edit: return s.group == Group::IV || s.group == Group::EV ? 6u :
                                  s.group == Group::Moves ? 4u : 1u;
        default: return 0;
    }
}
inline const Pokemon& pokemon(const State& s) {
    return s.fromPC ? s.demo.pc[s.box][s.mon] : s.demo.party[s.mon];
}
inline unsigned original(const State& s) {
    if(s.proposalFrom == Screen::Money) return s.demo.money;
    if(s.proposalFrom == Screen::Bag) return s.demo.bag[s.selected].quantity;
    if(s.proposalFrom == Screen::Edit) {
        const auto& m=pokemon(s);
        switch(s.group) {
            case Group::IV: return m.iv[s.field];
            case Group::EV: return m.ev[s.field];
            case Group::Moves: return m.moves[s.field];
            case Group::Ability: return m.ability;
            case Group::Nature: return m.nature;
        }
    }
    return 0;
}
inline unsigned minimum(const State& s) {
    return s.proposalFrom == Screen::Edit && s.group == Group::Moves ? 1u : 0u;
}
inline unsigned maximum(const State& s) {
    if(s.proposalFrom == Screen::Money) return 999999;
    if(s.proposalFrom == Screen::Bag) return 999;
    if(s.proposalFrom == Screen::Edit) {
        switch(s.group) {
            case Group::IV: return 31;
            case Group::EV: {
                const auto& m=pokemon(s);
                unsigned other=0;
                for(unsigned i=0;i<6;i++) if(i!=s.field) other+=m.ev[i];
                return std::min(252u, other <= 510u ? 510u-other : 0u);
            }
            case Group::Moves: return 1024;
            case Group::Ability: return 255;
            case Group::Nature: return 24;
        }
    }
    return 0;
}
inline void openNotice(State& s, Screen ret, const char* message) {
    s.returnTo=ret; s.notice=message; s.screen=Screen::Notice;
}
inline void startProposal(State& s, Screen from) {
    s.proposalFrom=from;
    // Backup actions are confirmations, never proposals of save-byte changes.
    if(from==Screen::Backups) { s.screen=Screen::ConfirmOne; return; }
    s.proposal=original(s);
    s.screen=Screen::Proposal;
}
inline void adjust(State& s, int delta) {
    const auto lo=static_cast<int64_t>(minimum(s));
    const auto hi=static_cast<int64_t>(maximum(s));
    const auto v=static_cast<int64_t>(s.proposal)+delta;
    s.proposal=static_cast<unsigned>(std::max(lo,std::min(hi,v)));
}
inline void press(State& s, Key k) {
    if(k==Key::Start) { s.exitRequested=true; return; }
    if(s.exitRequested) return;
    if(s.screen==Screen::Notice) {
        if(k==Key::B || k==Key::A) s.screen=s.returnTo;
        return;
    }
    if(s.screen==Screen::ConfirmOne || s.screen==Screen::ConfirmTwo) {
        if(k==Key::B) { s.screen=s.proposalFrom; return; }
        if(k==Key::A) {
            if(s.screen==Screen::ConfirmOne) s.screen=Screen::ConfirmTwo;
            else openNotice(s,s.proposalFrom,
                "BLOCKED: demo only. No files or live saves changed.");
        }
        return;
    }
    if(s.screen==Screen::Proposal) {
        switch(k) {
            case Key::Up: adjust(s,1); break;
            case Key::Down: adjust(s,-1); break;
            case Key::Right: adjust(s,10); break;
            case Key::Left: adjust(s,-10); break;
            case Key::X: s.proposal=original(s); break;
            case Key::A: s.screen=Screen::ConfirmOne; break;
            case Key::B: s.screen=s.proposalFrom; break;
            default: break;
        }
        return;
    }
    if(s.screen==Screen::Pokemon) {
        if(k==Key::B) s.screen=s.fromPC?Screen::PC:Screen::Party;
        else if(k==Key::A || k==Key::X) { s.group=Group::IV; s.field=0; s.screen=Screen::Edit; }
        return;
    }
    if(s.screen==Screen::Money) {
        if(k==Key::B) s.screen=Screen::Home;
        else if(k==Key::A) startProposal(s,Screen::Money);
        return;
    }
    if(s.screen==Screen::Edit) {
        if(k==Key::B) { s.screen=Screen::Pokemon; return; }
        if(k==Key::L || k==Key::R) {
            unsigned g=static_cast<unsigned>(s.group);
            g=(g+(k==Key::R?1u:4u))%5u;
            s.group=static_cast<Group>(g); s.field=0; return;
        }
    } else if(s.screen==Screen::PC) {
        if(k==Key::L || k==Key::R) {
            s.box=(s.box+(k==Key::R?1u:1u))%2u;
            s.selected=0; return;
        }
    }
    if(k==Key::B) { s.screen=Screen::Home; s.selected=0; return; }
    if(k==Key::Up) {
        if(s.screen==Screen::Home) { if(s.homeRow>0) --s.homeRow; }
        else if(s.screen==Screen::Edit) { if(s.field>0) --s.field; }
        else if(s.selected>0) --s.selected;
        return;
    }
    if(k==Key::Down) {
        if(s.screen==Screen::Home) { if(s.homeRow+1<6) ++s.homeRow; }
        else if(s.screen==Screen::Edit) { if(s.field+1<count(s)) ++s.field; }
        else if(s.selected+1<count(s)) ++s.selected;
        return;
    }
    if(k!=Key::A) return;
    switch(s.screen) {
        case Screen::Home:
            s.selected=0;
            switch(s.homeRow) {
                case 0: s.screen=Screen::Party; break;
                case 1: s.screen=Screen::PC; s.box=0; break;
                case 2: s.screen=Screen::Bag; break;
                case 3: s.screen=Screen::Money; break;
                case 4: s.screen=Screen::Backups; break;
                default: openNotice(s,Screen::Home,
                    "Recovery: use external verified backups; never inject a draft blindly."); break;
            }
            break;
        case Screen::Party:
        case Screen::PC:
            s.fromPC=s.screen==Screen::PC; s.mon=s.selected;
            s.screen=Screen::Pokemon;
            break;
        case Screen::Edit: startProposal(s,Screen::Edit); break;
        case Screen::Bag: startProposal(s,Screen::Bag); break;
        case Screen::Backups:
            if(s.selected==1) openNotice(s,Screen::Backups,
                "Backup status UNKNOWN. Check GodMode9/Checkpoint outside this demo.");
            else if(s.selected==2) openNotice(s,Screen::Backups,
                "Recovery: restore disabled. Keep independent original backups.");
            else startProposal(s,Screen::Backups);
            break;
        default: break;
    }
}
} // namespace phase19
