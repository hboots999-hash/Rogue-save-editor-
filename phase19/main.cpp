/* Phase 19: synthetic demo and opt-in read-only SD copy. GPL-3.0-or-later.
 * No live GBA VC title access, game archive, save edit, or restoration.
 */
#include <3ds.h>
#include <cstdio>
#include "ui.hpp"
#include "screen_text.hpp"
#include "offline_edit_presentation.hpp" // compile/size-audit shared UI-only Phase20 adapter
#include "read_only_import.hpp"
#include "rosalina_input.hpp"
#include "move_browser.hpp"
#include <string>
using namespace phase19;
static_assert(sizeof(OfflineApprovalGate)<16u,"ARM11 approval gate should stay tiny");
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
// Keep long guidance inside 3DS console width without heap allocations.
void wrappedNotice(const char* message) {
    // Reserve return/navigation text; long SD errors must not scroll it away.
    forEachBoundedLine(message,31u,11u,[](const char* line,unsigned chars) {
        std::printf("%.*s\n",static_cast<int>(chars),line);
    });
}
void top(const State& s) {
    consoleClear();
    std::puts("PKSM ROGUE - PHASE 19 COPY VIEW");
    std::puts("==============================");
    std::puts(s.demo.synthetic?"SYNTHETIC DEMO - NO SAVE WRITES":
              "IMPORTED SD COPY - READ ONLY");
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
            line(6,s.homeRow,"Inspect SD copy (read-only)");
            std::puts(s.demo.synthetic?"\nFictional example data.":"\nValidated copy view (no edits).");
            break;
        case Screen::Party:
            std::puts(s.demo.synthetic?"PARTY - DEMO":"PARTY - COPIED SAVE (READ ONLY)");
            std::printf("Status: %s\n",availabilityText(s.demo.partyStatus));
            for(unsigned i=0;i<count(s);i++) {
                const auto& m=s.demo.party[i];
                if(m.level)std::printf("%c %u %-14s Lv%u\n",i==s.selected?'>':' ',i+1u,m.name,m.level);
                else std::printf("%c %u %-14s Lv?\n",i==s.selected?'>':' ',i+1u,m.name);
            }
            break;
        case Screen::PC: {
            if(s.demo.pcStatus==Availability::Ready)
                std::printf("PC BOX %u/%u - READABLE\n",s.box+1u,s.demo.pcBoxes);
            else
                std::printf("PC BOXES - %s\n",availabilityText(s.demo.pcStatus));
            if(s.demo.pcStatus!=Availability::Ready) {
                std::puts("Real PC layout not established.");
                std::puts("No PC editing is permitted.");
            }
            const unsigned first=firstVisible(s.selected);
            const unsigned stop=lastVisible(s.selected,count(s));
            for(unsigned i=first;i<stop;i++) {
                const auto& m=s.demo.pc[s.box][i];
                std::printf("%c %u %-14s Lv%u\n",i==s.selected?'>':' ',i+1u,m.name,m.level);
            }
            if(count(s))std::printf("Viewing %u-%u / %u\n",first+1u,stop,count(s));
            std::puts("\nR/Right: next box");
            std::puts("X/Left: previous; A: details");
            break;
        }
        case Screen::Pokemon: {
            const auto& m=pokemon(s);
            if(m.level)std::printf("%s Lv%u (Species %u)\n",m.name,m.level,m.species);
            else std::printf("%s Lv? (Species %u)\n",m.name,m.species);
            std::printf("Source: %s\n",s.fromPC?"PC demo":
                        s.demo.synthetic?"Party demo":"Copied SD import");
            std::printf("Ability slot %u  Nature %u\n",m.ability,m.nature);
            std::puts("\nStat       IV     EV");
            for(unsigned i=0;i<6;i++)
                std::printf("%-7s   %2u    %3u\n",names[i],m.iv[i],m.ev[i]);
            std::puts("\nMove ID / PP:");
            for(unsigned i=0;i<4;i+=2)
                std::printf("%u:%u PP%u  %u:%u PP%u\n",
                    i+1u,m.moves[i],m.pp[i],i+2u,m.moves[i+1u],m.pp[i+1u]);
            std::puts(s.fromPC||!s.demo.synthetic?"\nREAD ONLY: editing disabled":"\nA: explore party edit demo");
            if(!s.fromPC)std::puts("Y: searchable move browser (RO)");
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
            std::puts("\nR/Right: next category");
            std::puts("X/Left: previous; A: preview");
            if(s.group==Group::Nature)std::puts("PID nature changes: UNSUPPORTED");
            else if(s.group==Group::Ability)std::puts("Ability mapping: NOT VERIFIED");
            else if(s.group==Group::Moves)std::puts("New move IDs: NOT VERIFIED");
            else std::puts("Proposal is DEMO ONLY.");
            std::puts("Nothing can be applied.");
            break;
        }
        case Screen::Bag:
            std::puts(s.demo.synthetic?"BAG / ITEMS - DEMO":"BAG / ITEMS - READ ONLY");
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
            std::puts("\nR/Right: next page");
            std::puts("X/Left: previous page");
            std::puts(s.demo.synthetic?"A: demo quantity":"Imported bag READ ONLY");
            std::puts("No adding unknown item IDs.");
        }
            break;
        case Screen::Money:
            std::printf("MONEY (%s)\n",availabilityText(s.demo.moneyStatus));
            if(s.demo.moneyStatus==Availability::Ready)
                std::printf("\nCurrent amount: %u\n",s.demo.money);
            else std::puts("\nNo verified value to display.");
            std::puts(s.demo.synthetic?"\nA: demo proposal":"\nImported balance is READ ONLY");
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
        case Screen::Import:
            std::puts("INSPECT COPIED SD SAVE ONLY");
            std::puts("\nA: validate fixed offline import");
            std::puts("X: switch back to demo mode");
            std::puts("B: return to main menu");
            std::puts("\nRequired exact file (128 KiB):");
            std::puts("/3ds/PKSM-Rogue/imports/");
            std::puts("00000001.sav");
            std::puts("\nNo installed game is opened.");
            std::puts("263168-byte VC wrapper blocked.");
            std::puts("Layout check != ROM identity.");
            break;
        case Screen::Proposal:
            std::puts("EDIT PROPOSAL - NOT APPLIED");
            if(s.proposalFrom==Screen::Money) std::puts("Money");
            else if(s.proposalFrom==Screen::Bag)
                std::printf("Item: %.18s\n",
                    s.selected<s.demo.bagCount && s.selected<s.demo.bag.size()?
                    s.demo.bag[s.selected].name:"<selection unavailable>");
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
            wrappedNotice(s.notice);
            std::puts("\nNo save was changed.");
            std::puts("A/B: return");
            break;
    }
}
// Move Browser uses fictional test metadata. Not a verified ROM database,
// writer, move replacement API or part of any SD draft export.
void moveTop(const State& s,const MoveBrowser& v) {
    consoleClear();
    std::puts("MOVE BROWSER - READ ONLY");
    std::puts(v.catalog?moveCatalogSourceLabel(v.catalog->origin):"MOVE CATALOGUE UNAVAILABLE");
    std::puts("===========================");
    if(v.page==MovePage::Party) {
        std::puts("CHOOSE PARTY POKEMON (1-6)");
        const unsigned n=s.demo.partyStatus==Availability::Ready?
            std::min(6u,s.demo.partyCount):0u;
        if(!n)std::puts("No party Pokemon available.");
        for(unsigned i=0;i<n;i++) {
            const auto& m=s.demo.party[i];
            std::printf("%c %u: %.17s ID%u%s\n",
                i==v.partySlot?'>':' ',i+1u,m.name,m.species,
                m.egg?" EGG":"");
        }
        std::puts("Up/Down: choose   A: open");
        std::puts("B: close (no changes)");
        std::puts("Empty/egg Pokemon blocked.");
        return;
    }
    if(!v.activeParty(s.demo)) {
        std::puts("Party data changed or missing.");
        std::puts("Return and reselect Pokemon.");
        std::puts("B: return; NO SAVE WRITES");
        return;
    }
    const auto& p=s.demo.party[v.partySlot];
    std::printf("Party %u: %.15s (#%u)\n",v.partySlot+1u,p.name,p.species);
    if(v.page==MovePage::Slots){
        std::puts("SELECT EXISTING MOVE SLOT");
        for(unsigned i=0;i<4u;i++)
            std::printf("%c Slot %u  ID %u  PP %u\n",
                        i==v.slot?'>':' ',i+1u,p.moves[i],p.pp[i]);
        std::puts("\nA: browse this slot");
        std::puts("Up/Down: slot  B: back");
        std::puts("No replacement can be saved.");
    }else if(v.page==MovePage::Catalog){
        std::printf("Slot %u old ID%u PP%u\n",v.slot+1u,p.moves[v.slot],p.pp[v.slot]);
        std::printf("Search: %.18s\n",v.query[0]?v.query:"(all moves)");
        std::printf("Matches %u  (alphabetical)\n",v.matchCount);
        const unsigned first=(v.selected/BrowserPageRows)*BrowserPageRows;
        const unsigned end=std::min(first+BrowserPageRows,v.matchCount);
        for(unsigned i=first;i<end;i++) {
            unsigned idx=0;
            if(!v.matchAt(i,idx))break;
            const auto& m=v.catalog->records[idx];
            // No false long-name truncation: full name appears on Review.
            // Mark other than matching ordinary candidate with '!'.
            const char flag=m.evidence==MoveEvidence::Candidate?' ':'!';
            char shortName[16]{};
            formatMoveShortName(m.name,shortName);
            char ppText[4]="--";
            if(m.evidence==MoveEvidence::Candidate)
                std::snprintf(ppText,sizeof(ppText),"%u",m.basePP);
            std::printf("%c%c %-15s %5u %2s\n",
                v.selected==i?'>':' ',flag,shortName,m.id,ppText);
        }
        if(!v.matchCount)std::puts("No results. Y: edit search.");
        else std::printf("%u-%u / %u ! NOT VERIFIED\n",
             first+1u,end,v.matchCount);
        std::puts("R/Right: next page");
        std::puts("X/Left: previous page");
        std::puts("Y: search  A: review");
        std::puts("B: return to slots");
    }else if(v.page==MovePage::Keyboard){
        std::puts("SEARCH MOVE NAME (max 18)");
        std::printf("Find: %.18s\n",v.pending);
        std::puts("D-Pad: choose key");
        for(unsigned y=0;y<5u;y++) {
            for(unsigned x=0;x<6u;x++) {
                const unsigned i=y*6u+x;
                const char k=MoveBrowser::Keyboard[i];
                if(i==v.keyboardIndex)std::printf("[%c] ",k==' '?'_':k);
                else std::printf(" %c  ",k==' '?'_':k);
            }
            std::putchar('\n');
        }
        std::puts("A: type / <: delete / !: apply");
        std::puts("X: delete  Y: apply");
        std::puts("B: cancel search");
        std::puts("No save files are modified.");
    }else if(v.page==MovePage::Review ||
             v.page==MovePage::ConfirmOne ||
             v.page==MovePage::ConfirmTwo){
        std::puts(v.page==MovePage::Review?"REPLACEMENT COMPARISON":
                  v.page==MovePage::ConfirmOne?"REVIEW CONFIRM 1 OF 2":
                  "REVIEW CONFIRM 2 OF 2");
        std::printf("Party %u: %.16s\n",v.partySlot+1u,p.name);
        std::printf("Move slot: %u\n",v.slot+1u);
        const MoveRecord* old=v.existingMove(s.demo,v.slot);
        std::printf("OLD ID%u PP%u\n",p.moves[v.slot],p.pp[v.slot]);
        if(old)std::printf("Old: %.29s\n",old->name);
        else std::puts("Old name: unverified ID");
        const MoveRecord* m=v.selectedMove();
        if(m){
            std::puts("NEW (READ-ONLY):");
            forEachBoundedLine(m->name,30u,2u,[](const char* row,unsigned n){
                std::printf("%.*s\n",int(n),row);
            });
            std::printf("New ID%u\n",m->id);
            if(m->evidence==MoveEvidence::Candidate)
                std::printf("Metadata base PP: %u\n",m->basePP);
            else std::puts("New PP: UNVERIFIED");
            wrappedNotice(evidenceLabel(m->evidence));
        }
        std::puts("APPLY DISABLED - NO SD DRAFT");
        if(v.page==MovePage::ConfirmTwo)
            std::puts("Release keys; X: view BLOCK");
        else std::puts("Release keys; A: next review");
        std::puts("B: cancel (no changes)");
    }else{
        if(v.notice==MoveNotice::EmptySlot){
            std::puts("EMPTY MOVE SLOT");
            std::puts("Choose an occupied move slot.");
            std::puts("No replacement is available.");
            std::puts("A/B: back to move slots");
        }else if(v.notice==MoveNotice::PartyChanged){
            std::puts("PARTY DATA CHANGED");
            std::puts("Return and reselect Pokemon.");
            std::puts("A/B: back");
        }else{
            std::puts("MOVE REPLACEMENT BLOCKED");
            std::puts("Exact-ROM move table and PP");
            std::puts("effects not authenticated.");
            std::puts("Phase18 production Apply OFF.");
            std::puts("No draft or save was changed.");
            std::puts("A/B: back to browse");
        }
    }
}
void moveBottom(const State& s,const MoveBrowser& v){
    consoleClear();
    std::puts("PKSM ROGUE - MOVE RESEARCH");
    std::puts("-------------------------");
    std::puts(v.catalog?moveCatalogSourceLabel(v.catalog->origin):"CATALOGUE MISSING");
    std::puts("936-record API supported.");
    std::puts("40 PP conflicts must be");
    std::puts("flagged by a verified source.");
    std::puts("NO MOVE REPLACEMENT ACTIVE");
    std::puts("No installed game write.");
    std::puts("No offline draft export.");
    std::puts("L button is NOT required.");
    std::puts("R/Right: forward (browser)");
    std::puts("X/Left: back (browser)");
    std::puts("START: release to exit");
    std::puts("START+SELECT: Rosalina");
    std::puts(s.demo.synthetic?"Fictional Pokemon and IDs.":
              "Real party, SYNTHETIC list.");
    if(v.page==MovePage::Review || v.page==MovePage::ConfirmOne ||
       v.page==MovePage::ConfirmTwo)
        std::puts("Review is UI only; Apply OFF.");
}
void bottom(const State& s) {
    consoleClear();
    std::puts("PKSM ROGUE - NONWRITING BUILD");
    std::puts("----------------------------");
    std::puts("D-Pad: navigate/adjust");
    std::puts("A: select     B: cancel");
    std::puts("R/Right: next page/group");
    std::puts("X/Left: previous (context)");
    std::puts("START release: exit");
    std::puts("START+SELECT: Rosalina");
    std::puts("");
    if(s.screen==Screen::Proposal || s.screen==Screen::ConfirmOne ||
       s.screen==Screen::ConfirmTwo)
        std::puts("Final confirmation = BLOCKED.");
    else if(s.screen==Screen::Backups)
        std::puts("Backups/restore NOT ACTIVE.");
    else std::puts(s.demo.synthetic?"DEMO: no game save opened.":"SD COPY VIEW / NO WRITES.");
    std::puts("\nEXPERIMENTAL - NOT A RELEASE");
}
bool toKey(u32 keys,Key& key) {
    if(keys&KEY_B) key=Key::B;
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
    // Hardware finding: never allocate large UI sample snapshots on ARM11
    // main-thread stack. The model references a process-lifetime static fixture.
    static Demo active{};
    populateMock(active);
    static State state(active);
    static char importStatus[200]{};
    static MoveBrowser moveView{};
    MoveNavRepeat moveRepeat{};
    bool viewingMoves=false;
    StartExitGate startGate{};
    bool redraw=true;
    while(aptMainLoop()&&!state.exitRequested) {
        hidScanInput();
        const u32 down=hidKeysDown();
        const u32 held=hidKeysHeld();
        const u32 up=hidKeysUp();
        // Do not steal START+SELECT from Rosalina in either press order.
        // A standalone START exits only when released.
        if(startGate.tick((down&KEY_START)!=0,(up&KEY_START)!=0,
                          (down&KEY_SELECT)!=0,(held&KEY_SELECT)!=0)) {
            press(state,Key::Start);
            redraw=true;
        }
        Key k=Key::X;
        if(viewingMoves) {
            if(moveView.page!=MovePage::Party &&
               !moveView.activeParty(state.demo)) {
                viewingMoves=false;moveRepeat.reset();redraw=true;
            }else if((held&(KEY_SELECT|KEY_START))!=0u) {
                // Reserve either-order START+SELECT for Rosalina.
                // Never navigate or confirm while either key is held.
                moveRepeat.reset();
            }else{
                MoveKey mk=MoveKey::B;
                bool moveKey=false;
                if(down&KEY_B){mk=MoveKey::B;moveKey=true;}
                else if(down&KEY_A){mk=MoveKey::A;moveKey=true;}
                else if(down&KEY_X){mk=MoveKey::X;moveKey=true;}
                else if(down&KEY_Y){mk=MoveKey::Y;moveKey=true;}
                if(moveKey)moveRepeat.reset();
                else if((held&(KEY_A|KEY_B|KEY_X|KEY_Y))==0u) {
                    bool nav=true;
                    if(held&KEY_UP)mk=MoveKey::Up;
                    else if(held&KEY_DOWN)mk=MoveKey::Down;
                    else if(held&KEY_LEFT)mk=MoveKey::Left;
                    else if(held&KEY_RIGHT)mk=MoveKey::Right;
                    else if(held&KEY_R)mk=MoveKey::R;
                    else nav=false;
                    moveKey=moveRepeat.poll(mk,nav);
                } else moveRepeat.reset();
                if(!moveKey && (held&(KEY_A|KEY_X))==0u)
                    moveView.releaseApproval();
                if(moveKey){
                    if(moveView.page==MovePage::Party && mk==MoveKey::B) {
                        viewingMoves=false;moveRepeat.reset();
                    }else if(moveView.page==MovePage::Slots &&
                             mk==MoveKey::B && !moveView.fromPartyPicker){
                        viewingMoves=false;moveRepeat.reset();
                    }else moveView.browse(state.demo,mk);
                    redraw=true;
                }
            }
        } else if((down&KEY_Y) &&
                  (state.screen==Screen::Party ||
                   (state.screen==Screen::Pokemon && !state.fromPC &&
                    selectedPokemonValid(state)))) {
            viewingMoves=state.screen==Screen::Party?
                moveView.beginPartySelection(syntheticMoveCatalog(),state.demo):
                moveView.initialize(syntheticMoveCatalog(),state.demo,state.mon);
            moveRepeat.reset();
            if(!viewingMoves)
                openNotice(state,Screen::Pokemon,catalogProblemText(moveView.problem));
            redraw=true;
        } else if(toKey(down & ~(KEY_START|KEY_SELECT),k)) {
            if(state.screen==Screen::Import && k==Key::A) {
                viewingMoves=false;
                std::string reason;
                if(inspectFixedSDCopy(active,reason)) {
                    switchView(state,false);
                } else {
                    std::snprintf(importStatus,sizeof(importStatus),"%s",reason.c_str());
                    openNotice(state,Screen::Import,importStatus);
                }
            } else if(state.screen==Screen::Import && k==Key::X) {
                viewingMoves=false;
                populateMock(active);
                switchView(state,true);
            } else press(state,k);
            redraw=true;
        }
        if(redraw) {
            consoleSelect(&upper);
            if(viewingMoves)moveTop(state,moveView); else top(state);
            consoleSelect(&lower);
            if(viewingMoves)moveBottom(state,moveView); else bottom(state);
            redraw=false;
        }
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
