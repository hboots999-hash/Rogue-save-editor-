// Phase 19 standalone UI model; GPL-3.0-or-later.
// Deliberately has NO filesystem, save-service, ROM or restoration interface.
#pragma once
#include "adapter.hpp"
#include <cstdint>
#include <algorithm>

namespace phase19 {
enum class Screen : uint8_t {
    Home, Party, PC, Pokemon, Edit, Bag, Money, Backups, Recovery, Import,
    Proposal, ConfirmOne, ConfirmTwo, Notice
};
enum class Key : uint8_t { Up, Down, Left, Right, A, B, X, L, R, Start };
enum class Group : uint8_t { IV, EV, Moves, PP, Ability, Nature };

struct State {
    Screen screen = Screen::Home;
    Screen returnTo = Screen::Home;
    Screen proposalFrom = Screen::Home;
    unsigned homeRow = 0, selected = 0, box = 0, mon = 0;
    bool fromPC = false, exitRequested = false;
    // Independent session gate. Imported data cannot enter mock edit flow even
    // if its synthetic flag is accidentally toggled by a later adapter bug.
    bool demoPreviewEnabled = false;
    Group group = Group::IV;
    unsigned field = 0;
    unsigned proposal = 0;
    const char* notice = "";
    // Non-owning read-only snapshot view: no 64-KiB demo in the 3DS thread stack.
    // Caller keeps externally supplied provider snapshot alive during UI use.
    const Demo& demo;
    State():demoPreviewEnabled(true),demo(mockSnapshotRef()){}
    explicit State(const Demo& source):demoPreviewEnabled(source.synthetic),demo(source){}
};
static_assert(sizeof(State)<256,"Phase19 State must remain stack-safe on 3DS");
inline void switchView(State& s, bool toFictionalDemo) {
    // Called only after explicit native import success or explicit mock reset.
    s.demoPreviewEnabled=toFictionalDemo;
    s.screen=Screen::Home;
    s.returnTo=s.proposalFrom=Screen::Home;
    s.homeRow=s.selected=s.box=s.mon=s.field=s.proposal=0;
    s.fromPC=false;
    s.group=Group::IV;
    s.notice="";
}
constexpr unsigned PageRows=6;
inline unsigned firstVisible(unsigned selected) {
    return (selected/PageRows)*PageRows;
}
inline unsigned lastVisible(unsigned selected,unsigned total) {
    return std::min(total,firstVisible(selected)+PageRows);
}

inline unsigned count(const State& s) {
    switch(s.screen) {
        case Screen::Home: return 7;
        case Screen::Party: return s.demo.partyStatus==Availability::Ready?s.demo.partyCount:0;
        case Screen::PC: return s.demo.pcStatus==Availability::Ready && s.box<s.demo.pcBoxes ?
                                 s.demo.pcCounts[s.box]:0;
        case Screen::Bag: return s.demo.bagStatus==Availability::Ready?s.demo.bagCount:0;
        case Screen::Backups: return 3;
        case Screen::Recovery: return 5;
        case Screen::Edit: return s.group == Group::IV || s.group == Group::EV ? 6u :
                                  (s.group == Group::Moves || s.group == Group::PP) ? 4u : 1u;
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
            case Group::PP: return m.pp[s.field];
            case Group::Ability: return m.ability;
            case Group::Nature: return m.nature;
        }
    }
    return 0;
}
inline unsigned minimum(const State& s) {
    return s.proposalFrom == Screen::Edit && s.group == Group::Moves ? 1u :
           s.proposalFrom == Screen::Bag ? 1u : 0u;
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
            case Group::Moves: return 1024; // Demo IDs only; Phase18 whitelist required
            case Group::PP: return 64;
            case Group::Ability: return 3; // Phase18 EX v2.1 uses two-bit slot
            case Group::Nature: return 24; // read-only; PID mutation forbidden
        }
    }
    return 0;
}
inline const char* recoveryMessage(unsigned code) {
    switch(code) {
        case 0: return "NO SAVE / WRONG GAME\nKeep original save safe.\nCheck the installed title ID.\nCheck a separate backup.";
        case 1: return "UNKNOWN SAVE LAYOUT\nDo not edit or inject.\nVerify Emerald Rogue 2.1.2\nand the exact ROM build.";
        case 2: return "CHECKSUM FAILED\nStop: no restore or edits.\nUse a known-good untouched\nexternal recovery backup.";
        case 3: return "BACKUP NOT VERIFIED\nNo app backup was checked.\nVerify GodMode9 / Checkpoint\nbackups independently.";
        default: return "RESTORE IS BLOCKED\nAuthenticated CMAC signing\nand rollback are NOT enabled\nin the Phase 19 preview.";
    }
}
inline bool canPreview(const State& s) {
    if(!s.demo.synthetic || !s.demoPreviewEnabled) return false; // two independent gates
    if(s.proposalFrom==Screen::Money)
        return s.demo.moneyStatus==Availability::Ready;
    if(s.proposalFrom==Screen::Bag)
        return s.demo.bagStatus==Availability::Ready &&
               s.selected<s.demo.bagCount;
    if(s.proposalFrom==Screen::Edit)
        return !s.fromPC && s.demo.partyStatus==Availability::Ready &&
               s.mon<s.demo.partyCount && !pokemon(s).egg &&
               s.group!=Group::Ability && s.group!=Group::Nature;
    return false;
}
inline bool validProposal(const State& s) {
    if(!canPreview(s) || s.proposal==original(s) ||
       s.proposal<minimum(s) || s.proposal>maximum(s))return false;
    // Phase18 cannot accept new move IDs without independently verified
    // ROM-specific allowlists. This demo has none: PP-only is previewable.
    if(s.proposalFrom==Screen::Edit && s.group==Group::Moves)return false;
    return true;
}
inline void openNotice(State& s, Screen ret, const char* message) {
    s.returnTo=ret; s.notice=message; s.screen=Screen::Notice;
}
inline void startProposal(State& s, Screen from) {
    s.proposalFrom=from;
    // Backup actions are confirmations, never proposals of save-byte changes.
    if(from==Screen::Backups) { s.screen=Screen::ConfirmOne; return; }
    if(!s.demo.synthetic || !s.demoPreviewEnabled) {
        openNotice(s,from,"READ ONLY: copied data cannot enter any editing workflow.");
        return;
    }
    if(!canPreview(s)) {
        openNotice(s,from,"UNSUPPORTED: PC/ability/nature edits are not verified.");
        return;
    }
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
        if(k==Key::B) { s.screen=s.proposalFrom==Screen::Backups ?
                                      Screen::Backups:Screen::Proposal; return; }
        if(k==Key::A) {
            if(s.screen==Screen::ConfirmOne) s.screen=Screen::ConfirmTwo;
            else openNotice(s,s.proposalFrom,
                "BLOCKED: demonstration only. No save or backup changed.");
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
            case Key::A:
                if(validProposal(s))s.screen=Screen::ConfirmOne;
                else openNotice(s,Screen::Proposal,
                    "INVALID/UNVERIFIED: unchanged value or unsupported move ID.");
                break;
            case Key::B: s.screen=s.proposalFrom; break;
            default: break;
        }
        return;
    }
    if(s.screen==Screen::Pokemon) {
        if(k==Key::B) s.screen=s.fromPC?Screen::PC:Screen::Party;
        else if(k==Key::A || k==Key::X) {
            if(s.fromPC)openNotice(s,Screen::Pokemon,
                "PC edit UNSUPPORTED: storage layout not verified.");
            else if(!s.demo.synthetic || !s.demoPreviewEnabled)
                openNotice(s,Screen::Pokemon,"Imported Pokemon are READ ONLY.");
            else if(s.demo.partyStatus!=Availability::Ready)
                openNotice(s,Screen::Pokemon,"Party data unavailable.");
            else {s.group=Group::IV; s.field=0;s.screen=Screen::Edit;}
        }
        return;
    }
    if(s.screen==Screen::Import) {
        if(k==Key::B) s.screen=Screen::Home;
        return; // only native entry point performs explicitly requested copy reads
    }
    if(s.screen==Screen::Money) {
        if(k==Key::B) s.screen=Screen::Home;
        else if(k==Key::A) startProposal(s,Screen::Money);
        return;
    }
    if(s.screen==Screen::Edit) {
        if(k==Key::B) { s.screen=Screen::Pokemon; return; }
        // X and D-pad Left are full alternatives to the broken L shoulder.
        if(k==Key::L || k==Key::R || k==Key::X ||
           k==Key::Left || k==Key::Right) {
            const unsigned g=static_cast<unsigned>(s.group);
            const bool forward=k==Key::R || k==Key::Right;
            s.group=static_cast<Group>((g+(forward?1u:5u))%6u);
            s.field=0;return;
        }
    } else if(s.screen==Screen::PC) {
        if(k==Key::L || k==Key::R || k==Key::X ||
           k==Key::Left || k==Key::Right) {
            if(s.demo.pcStatus!=Availability::Ready || !s.demo.pcBoxes)return;
            const bool forward=k==Key::R || k==Key::Right;
            s.box=(s.box+(forward?1u:s.demo.pcBoxes-1u))%s.demo.pcBoxes;
            s.selected=0;return;
        }
    } else if(s.screen==Screen::Bag) {
        if(k==Key::L || k==Key::R || k==Key::X ||
           k==Key::Left || k==Key::Right) {
            const unsigned n=count(s);
            if(!n)return;
            const bool forward=k==Key::R || k==Key::Right;
            const unsigned page=firstVisible(s.selected);
            if(forward) {
                if(n>page+PageRows)s.selected=page+PageRows;
            } else if(page>=PageRows)s.selected=page-PageRows;
            return;
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
        if(s.screen==Screen::Home) { if(s.homeRow+1<7) ++s.homeRow; }
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
                case 5: s.screen=Screen::Recovery; break;
                default: s.screen=Screen::Import; break;
            }
            break;
        case Screen::Party:
        case Screen::PC:
            if(!count(s)) {
                openNotice(s,s.screen,"No validated Pokemon available for this view.");
                break;
            }
            s.fromPC=s.screen==Screen::PC; s.mon=s.selected;
            s.screen=Screen::Pokemon;
            break;
        case Screen::Edit: startProposal(s,Screen::Edit); break;
        case Screen::Recovery: openNotice(s,Screen::Recovery,recoveryMessage(s.selected)); break;
        case Screen::Bag:
            if(!count(s))openNotice(s,Screen::Bag,"Bag unavailable or empty.");
            else startProposal(s,Screen::Bag);
            break;
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
