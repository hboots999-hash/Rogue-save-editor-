/* Phase 19 isolated native 3DS UI; GPL-3.0-or-later.
 * Synthetic data only. NO filesystem, imported save, installed-title access, or restore.
 */
#include <3ds.h>
#include <cstdio>
#include "ui.hpp"
using namespace phase19;
namespace {
const char* names[6]={"HP","Atk","Def","SpA","SpD","Spe"};
const char* groupName(Group g) {
    switch(g) {
        case Group::IV: return "Individual Values";
        case Group::EV: return "Effort Values";
        case Group::Moves: return "Move IDs";
        case Group::PP: return "Move PP";
        case Group::Ability: return "Ability slot (RO)";
        case Group::Nature: return "Nature";
    }
    return "Unknown";
}
void line(unsigned i,unsigned selected,const char* str) {
    std::printf("%c %s\n",i==selected?'>':' ',str);
}
void top(const State& s) {
    consoleClear();
    std::puts("PKSM ROGUE - PHASE 19 UI DEMO");
    std::puts("==============================");
    std::puts("SYNTHETIC ONLY / NO SAVE WRITES");
    std::puts("");
    switch(s.screen) {
        case Screen::Home:
            std::puts("Main menu");
            line(0,s.homeRow,"Party Pokemon");
            line(1,s.homeRow,"PC Boxes");
            line(2,s.homeRow,"Bag / Items");
            line(3,s.homeRow,"Money");
            line(4,s.homeRow,"Backups");
            line(5,s.homeRow,"Recovery help");
            std::puts("\nFictional example data only.");
            break;
        case Screen::Party:
            std::puts("PARTY - demonstration roster");
            std::printf("Status: %s\n",availabilityText(s.demo.partyStatus));
            for(unsigned i=0;i<count(s);i++) {
                const auto& m=s.demo.party[i];
                std::printf("%c %u %-14s Lv%u\n",i==s.selected?'>':' ',i+1u,m.name,m.level);
            }
            break;
        case Screen::PC: {
            std::printf("PC BOX %u/%u - %s\n",s.box+1u,s.demo.pcBoxes,
                        availabilityText(s.demo.pcStatus));
            if(s.demo.pcStatus!=Availability::Ready) {
                std::puts("Real PC layout not established.");
                std::puts("No PC editing is permitted.");
            }
            const unsigned first=(s.selected/6u)*6u;
            const unsigned stop=std::min(count(s),first+6u);
            for(unsigned i=first;i<stop;i++) {
                const auto& m=s.demo.pc[s.box][i];
                std::printf("%c %u %-14s Lv%u\n",i==s.selected?'>':' ',i+1u,m.name,m.level);
            }
            if(count(s))std::printf("Viewing %u-%u / %u\n",first+1u,stop,count(s));
            std::puts("\nL/R: box; A: detail (read-only)");
            break;
        }
        case Screen::Pokemon: {
            const auto& m=pokemon(s);
            std::printf("%s  Lv%u (Species %u)\n",m.name,m.level,m.species);
            std::printf("Source: %s\n",s.fromPC?"PC demo":"Party demo");
            std::printf("Ability slot %u  Nature %u\n",m.ability,m.nature);
            std::puts("\nStat       IV     EV");
            for(unsigned i=0;i<6;i++)
                std::printf("%-7s   %2u    %3u\n",names[i],m.iv[i],m.ev[i]);
            std::puts("\nMove IDs:");
            for(unsigned i=0;i<4;i++) std::printf("%u: %u  ",i+1u,m.moves[i]);
            std::puts(s.fromPC?"\n\nPC fields: READ ONLY":"\n\nA: explore party edit demo");
            break;
        }
        case Screen::Edit: {
            const auto& m=pokemon(s);
            std::printf("%s / %s\n",m.name,groupName(s.group));
            std::puts("");
            for(unsigned i=0;i<count(s);i++) {
                unsigned v=0;
                switch(s.group) {
                    case Group::IV: v=m.iv[i];break;
                    case Group::EV: v=m.ev[i];break;
                    case Group::Moves: v=m.moves[i];break;
                    case Group::PP: v=m.pp[i];break;
                    case Group::Ability: v=m.ability;break;
                    case Group::Nature: v=m.nature;break;
                }
                const char* n=s.group==Group::IV||s.group==Group::EV?names[i]:
                    (s.group==Group::Moves || s.group==Group::PP)?"Slot":"Value";
                std::printf("%c %-7s %u: %u\n",s.field==i?'>':' ',n,i+1u,v);
            }
            if(s.group==Group::EV) {
                unsigned sum=0;for(auto v:m.ev) sum+=v;
                std::printf("\nEV total: %u / 510\n",sum);
            }
            std::puts("\nL/R category; A propose");
            if(s.group==Group::Nature)std::puts("PID nature changes: UNSUPPORTED");
            else if(s.group==Group::Ability)std::puts("Ability mapping: NOT VERIFIED");
            else if(s.group==Group::Moves)std::puts("New move IDs: NOT VERIFIED");
            else std::puts("Proposal is DEMO ONLY.");
            std::puts("Nothing can be applied.");
            break;
        }
        case Screen::Bag:
            std::puts("BAG / ITEMS - DEMO");
        {
            std::printf("Status: %s\n",availabilityText(s.demo.bagStatus));
            const unsigned first=(s.selected/6u)*6u;
            const unsigned stop=std::min(count(s),first+6u);
            for(unsigned i=first;i<stop;i++) {
                const auto& item=s.demo.bag[i];
                std::printf("%c %-16.16s x%-3u ID%u\n",
                            s.selected==i?'>':' ',item.name[0]?item.name:"Item",
                            item.quantity,item.id);
            }
            if(count(s))std::printf("%u-%u of %u stacks\n",first+1u,stop,count(s));
            std::puts("\nL/R: page   A: quantity demo");
            std::puts("No adding unknown item IDs.");
        }
            break;
        case Screen::Money:
            std::printf("MONEY (%s)\n",availabilityText(s.demo.moneyStatus));
            if(s.demo.moneyStatus==Availability::Ready)
                std::printf("\nCurrent amount: %u\n",s.demo.money);
            else std::puts("\nNo verified value to display.");
            std::puts("\nA: propose amount");
            break;
        case Screen::Backups:
            std::puts("BACKUPS / SAFE MODE");
            line(0,s.selected,"Request backup (disabled)");
            line(1,s.selected,"Check existing backups");
            line(2,s.selected,"Restore (disabled)");
            std::puts("\nPhase16 verification is external.");
            std::puts("This UI has no backup reader.");
            std::puts("Status: NOT CHECKED IN APP.");
            std::puts("Retain independent GodMode9");
            std::puts("and Checkpoint original backups.");
            break;
        case Screen::Recovery:
            std::puts("RECOVERY / ERROR GUIDE");
            line(0,s.selected,"No save or title mismatch");
            line(1,s.selected,"Unknown layout/version");
            line(2,s.selected,"Checksum failure");
            line(3,s.selected,"Backup not verified");
            line(4,s.selected,"Restore not approved");
            std::puts("\nA: show recovery guidance");
            break;
        case Screen::Proposal:
            std::puts("EDIT PROPOSAL - NOT APPLIED");
            if(s.proposalFrom==Screen::Money) std::puts("Money");
            else if(s.proposalFrom==Screen::Bag)
                std::printf("Item: %s\n",s.demo.bag[s.selected].name);
            else std::printf("%s / %s\n",pokemon(s).name,groupName(s.group));
            std::printf("\nBefore: %u\nProposed: %u\nAllowed: %u - %u\n",
                original(s),s.proposal,minimum(s),maximum(s));
            std::puts("\nUp/Down +/-1; Left/Right +/-10");
            std::puts("X reset; A review; B cancel");
            break;
        case Screen::ConfirmOne:
        case Screen::ConfirmTwo:
            std::puts("CONFIRMATION - SAFE MODE");
            if(s.proposalFrom==Screen::Backups)
                std::puts("\nBackup creation is disabled.");
            else std::printf("\nBefore: %u\nProposal: %u\n",original(s),s.proposal);
            std::printf("\nConfirmation %u/2\n",s.screen==Screen::ConfirmOne?1u:2u);
            std::puts("A: continue to BLOCKED notice");
            std::puts("B: return to review (no write)");
            std::puts("\nNO FILES WILL BE WRITTEN.");
            break;
        case Screen::Notice:
            std::puts("STATUS / RECOVERY");
            std::puts(s.notice);
            std::puts("\nNo save was changed.");
            std::puts("A/B: return");
            break;
    }
}
void bottom(const State& s) {
    consoleClear();
    std::puts("PKSM ROGUE - NONWRITING BUILD");
    std::puts("----------------------------");
    std::puts("D-Pad: navigate/adjust");
    std::puts("A: select     B: cancel");
    std::puts("L/R: PC box or edit category");
    std::puts("START: exit");
    std::puts("");
    if(s.screen==Screen::Proposal || s.screen==Screen::ConfirmOne ||
       s.screen==Screen::ConfirmTwo)
        std::puts("Final confirmation = BLOCKED.");
    else if(s.screen==Screen::Backups)
        std::puts("Backups/restore NOT ACTIVE.");
    else std::puts("No real save file is opened.");
    std::puts("\nEXPERIMENTAL - NOT A RELEASE");
}
bool toKey(u32 keys,Key& key) {
    if(keys&KEY_START) key=Key::Start;
    else if(keys&KEY_B) key=Key::B;
    else if(keys&KEY_A) key=Key::A;
    else if(keys&KEY_UP) key=Key::Up;
    else if(keys&KEY_DOWN) key=Key::Down;
    else if(keys&KEY_LEFT) key=Key::Left;
    else if(keys&KEY_RIGHT) key=Key::Right;
    else if(keys&KEY_L) key=Key::L;
    else if(keys&KEY_R) key=Key::R;
    else if(keys&KEY_X) key=Key::X;
    else return false;
    return true;
}
}
int main() {
    gfxInitDefault();
    PrintConsole upper{},lower{};
    consoleInit(GFX_TOP,&upper);
    consoleInit(GFX_BOTTOM,&lower);
    State state;
    bool redraw=true;
    while(aptMainLoop()&&!state.exitRequested) {
        hidScanInput();
        Key k=Key::X;
        if(toKey(hidKeysDown(),k)) { press(state,k); redraw=true; }
        if(redraw) {
            consoleSelect(&upper); top(state);
            consoleSelect(&lower); bottom(state);
            redraw=false;
        }
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
