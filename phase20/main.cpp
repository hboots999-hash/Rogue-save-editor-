/* Phase20: copied-save read + SD-only offline draft UI, GPL-3.0-or-later.
 * NO installed-title archive, game-save writes, CMAC or restoration.
 */
#include <3ds.h>
#include <cstdio>
#include "ui.hpp"
#include "screen_text.hpp"
#include "phase20/read_only_v1.hpp"
#include "phase20/offline_edit_v1.hpp"
#include "rosalina_input.hpp"
#include "move_browser.hpp"
#include "phase20/move_catalog_bridge_v1.hpp"
#include <string>
#include <new>
#include <exception>
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
// Keep long guidance inside 3DS console width without heap allocations.
void wrappedNotice(const char* message) {
    forEachWrappedLine(message,31u,[](const char* line,unsigned chars) {
        std::printf("%.*s\n",static_cast<int>(chars),line);
    });
}
void offlineTop(const State& s,const phase20::OfflineFlow& f,
                const phase20::BatchFlow& batch) {
    using phase20::EditStage;
    std::puts("OFFLINE COPY EDITOR - NO GAME IO");
    std::puts("===============================");
    std::puts("SOURCE: SD imported copy ONLY");
    std::puts("Original imported file unchanged");
    std::puts("");
    if(f.stage==EditStage::Category) {
        std::puts("Choose a party edit category");
        std::printf("%c Individual Values (IV)\n",f.category==0?'>':' ');
        std::printf("%c Effort Values (EV)\n",f.category==1?'>':' ');
        std::printf("%c Existing move PP\n",f.category==2?'>':' ');
        std::puts("\nR/Right, X/Left: change");
        std::puts("A: select   B: cancel");
    } else if(f.stage==EditStage::Field) {
        std::printf("%s / party #%u\n\n",phase20::kindName(f.choice.kind),
                    f.choice.slot+1u);
        for(unsigned i=0;i<phase20::fieldCount(f.choice.kind);i++) {
            auto c=f.choice;c.field=i;
            std::printf("%c %s%u: %u\n",i==f.choice.field?'>':' ',
              f.choice.kind==phase20::OfflineKind::PP?"Move ":"Stat ",
              i+1u,phase20::oldValue(s.demo,c));
        }
        std::puts("\nA: edit selected field   B: back");
    } else if(f.stage==EditStage::Value) {
        std::printf("%s\n",phase20::kindName(f.choice.kind));
        if(f.choice.kind==phase20::OfflineKind::ExistingItem)
            std::printf("Item ID: %u / bag slot %u\n",
              s.demo.bag[f.choice.slot].id,f.choice.slot+1u);
        if(f.choice.kind==phase20::OfflineKind::IV ||
           f.choice.kind==phase20::OfflineKind::EV ||
           f.choice.kind==phase20::OfflineKind::PP)
            std::printf("Party #%u field #%u\n",
              f.choice.slot+1u,f.choice.field+1u);
        std::printf("\nOriginal: %u\nNew value: %u\nMax: %u\n",
                    f.original,f.proposed,phase20::batchValueLimit(s.demo,f.choice,&batch));
        std::puts("\nUp/Down +/-1 Left/Right +/-10");
        std::puts("X reset; A review; B back");
    } else if(f.stage==EditStage::ConfirmOne ||
              f.stage==EditStage::ConfirmTwo) {
        std::puts(batch.collecting?"STAGE TO RAM QUEUE":"EXPORT AN OFFLINE DRAFT");
        std::printf("\n%s: %u -> %u\n",
           phase20::kindName(f.choice.kind),f.original,f.proposed);
        if(batch.collecting)
            std::puts("\nA stage edit (NO SD write)");
        else std::printf("\nConfirmation %u of 2\n",
                        f.stage==EditStage::ConfirmOne?1u:2u);
        std::puts("\nA: confirm  B: return");
        std::puts("\nONLY SD copies/backup/drafts");
        std::puts("NEVER the installed game save.");
        std::puts("Draft is NOT game-verified.");
    } else if(f.stage==EditStage::Result) {
        std::puts(batch.collecting?
                (f.succeeded?"EDIT ADDED TO RAM QUEUE":"EDIT NOT QUEUED"):
                (f.succeeded?"OFFLINE DRAFT CREATED":"OFFLINE EXPORT REFUSED"));
        wrappedNotice(f.result);
        std::puts("\nA/B: return to imported view");
        std::puts("\nOriginal is never overwritten.");
    }
}
void batchTop(const phase20::BatchFlow& q) {
    using phase20::BatchStage;
    std::puts("MULTI-EDIT SD DRAFT - NO GAME IO");
    std::puts("===============================");
    std::printf("Queued edits: %u / %u\n",
        unsigned(q.pending.edits.size()),unsigned(rogue18::v3::MaxEdits));
    std::printf("Source SHA256: %02x%02x%02x%02x...\n",
        unsigned(q.pending.expectedDigest[0]),unsigned(q.pending.expectedDigest[1]),
        unsigned(q.pending.expectedDigest[2]),unsigned(q.pending.expectedDigest[3]));
    if(q.stage==BatchStage::Review) {
        const unsigned start=q.page*5;
        for(unsigned i=start;i<q.pending.edits.size() && i<start+5;i++) {
            const auto& e=q.pending.edits[i];
            const char* kind=e.kind==rogue18::Kind::Money?"Money":
               e.kind==rogue18::Kind::ExistingQuantity?"Item":
               e.kind==rogue18::Kind::IVs?"IV":
               e.kind==rogue18::Kind::EVs?"EV":"PP";
            std::printf("%u %s[%u:%u] %u -> %u\n",
                i+1,kind,e.slot+1,e.field+1,e.expectedValue,e.value);
        }
        std::puts("\nA review export, B browse");
        std::puts("X remove last staged edit");
        std::puts("R/Right next; Left prev");
        std::puts("All edits share one source");
    } else if(q.stage==BatchStage::ConfirmOne ||
              q.stage==BatchStage::ConfirmTwo) {
        std::printf("\nFinal batch confirmation %u/2\n",
                    q.stage==BatchStage::ConfirmOne?1u:2u);
        std::puts("ALL queued edits in ONE draft.");
        std::puts("Will create immutable SD backup.");
        std::puts("Original import unchanged.");
        std::puts("A confirm; B review/cancel");
        std::puts("\nNEVER installed-game writing!");
    } else if(q.stage==BatchStage::Result) {
        std::puts(q.success?"ONE BATCH DRAFT EXPORTED":"BATCH EXPORT REFUSED");
        wrappedNotice(q.message);
        std::puts("\nA continue, B review.");
        std::puts("No live-game edit/restore.");
    }
}
void top(const State& s,const phase20::OfflineFlow& edit,
         const phase20::BatchFlow& batch) {
    if(batch.stage!=phase20::BatchStage::Browse){
        consoleClear();batchTop(batch);return;
    }
    if(edit.stage!=phase20::EditStage::Idle){
        consoleClear();offlineTop(s,edit,batch);return;
    }
    consoleClear();
    std::puts("PKSM ROGUE - PHASE20 SAFE VIEW");
    std::puts("==============================");
    std::puts(s.demo.synthetic?"SYNTHETIC DEMO - NO SAVE WRITES":
              "IMPORTED SD COPY - OFFLINE DRAFT");
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
            std::puts(s.demo.synthetic?"\nFictional example data.":"\nA offline edit; Y batch queue.");
            break;
        case Screen::Party:
            std::puts(s.demo.synthetic?"PARTY - DEMO":"PARTY - OFFLINE COPY");
            std::puts("X: move picker (READ ONLY)");
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
            std::puts(s.fromPC?"\n\nPC editing unsupported":!s.demo.synthetic?"\n\nA: OFFLINE IV / EV / PP":"\n\nA: explore party edit demo");
            if(!s.fromPC)std::puts("Y: browse moves (READ ONLY)");
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
            std::puts(s.demo.synthetic?"BAG / ITEMS - DEMO":"BAG / ITEMS - OFFLINE COPY");
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
            std::puts(s.demo.synthetic?"A: demo quantity":"A: OFFLINE existing quantity");
            std::puts("No adding unknown item IDs.");
        }
            break;
        case Screen::Money:
            std::printf("MONEY (%s)\n",availabilityText(s.demo.moneyStatus));
            if(s.demo.moneyStatus==Availability::Ready)
                std::printf("\nCurrent amount: %u\n",s.demo.money);
            else std::puts("\nNo verified value to display.");
            std::puts(s.demo.synthetic?"\nA: demo proposal":"\nA: OFFLINE money draft");
            break;
        case Screen::Backups:
            std::puts("BACKUPS / SAFE MODE");
            line(0,s.selected,"Request backup (disabled)");
            line(1,s.selected,"Check existing backups");
            line(2,s.selected,"Restore (disabled)");
            std::puts("\nPhase17 status: NOT CHECKED.");
            std::puts("This UI has NO recovery writer.");
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
        }else if(v.notice==MoveNotice::CandidateUnavailable){
            std::puts("MOVE IS UNVERIFIED");
            std::puts("PP conflict/special/unknown.");
            std::puts("This move cannot be selected.");
            std::puts("No draft/save modified.");
            std::puts("A/B: back to browse");
        }else if(v.notice==MoveNotice::DuplicateMove ||
                 v.notice==MoveNotice::SameMove){
            std::puts(v.notice==MoveNotice::SameMove?
                      "MOVE ALREADY IN THIS SLOT":
                      "POKEMON KNOWS THIS MOVE");
            std::puts("Choose another candidate.");
            std::puts("No draft/save modified.");
            std::puts("A/B: back to browse");
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
    std::puts(s.demo.synthetic?"DEMO Pokemon + fictional moves.":
              "Real party + authenticated metadata.");
    if(v.page==MovePage::Review || v.page==MovePage::ConfirmOne ||
       v.page==MovePage::ConfirmTwo)
        std::puts("Review is UI only; Apply OFF.");
}
void bottom(const State& s,const phase20::OfflineFlow& edit,
            const phase20::BatchFlow& batch) {
    consoleClear();
    std::puts("PKSM ROGUE - SD COPY ONLY");
    std::puts("----------------------------");
    std::puts("D-Pad: navigate/adjust");
    std::puts("A: select     B: cancel");
    std::puts("R/Right: next page/group");
    std::puts("X/Left: previous (context)");
    std::puts("START release: exit");
    std::puts("START+SELECT: Rosalina");
    if(!s.demo.synthetic) {
        if(batch.collecting)std::printf("Y: review queue (%u edits)\n",
                                       unsigned(batch.pending.edits.size()));
        else std::puts("Y: start multi-edit queue");
    }
    std::puts("");
    if(batch.stage!=phase20::BatchStage::Browse) {
        std::puts("ONE multi-edit SD draft only.");
        std::puts("No installed-game writer.");
    } else if(edit.stage!=phase20::EditStage::Idle) {
        std::puts("Edits export distinct SD drafts.");
        std::puts("NO installed-game write/restore.");
    } else if(s.screen==Screen::Proposal || s.screen==Screen::ConfirmOne ||
       s.screen==Screen::ConfirmTwo)
        std::puts("DEMO preview always BLOCKED.");
    else if(s.screen==Screen::Backups)
        std::puts("Backups/restore NOT ACTIVE.");
    else std::puts(s.demo.synthetic?"DEMO: no game save opened.":"Offline editing: A on money,");
    if(edit.stage==phase20::EditStage::Idle && !s.demo.synthetic)
        std::puts("bag item or party Pokemon.");
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
    static phase20::OfflineFlow editing{};
    static MoveBrowser moveView{};
    MoveNavRepeat moveRepeat{};
    static phase20::LocalMoveProvider localMoves{};
    bool viewingMoves=false;
    static phase20::BatchFlow batch{};
    static phase20::CopyObservation observation{};
    static P17ReadOnlyStatusV1 recoveryStatus{};
    (void)p17_readonly_status_init_blocked_v1(&recoveryStatus);
    // No live container is opened by this Phase20 preview.
    const auto recoveryView=phase20::inspectRecoveryStatus(&recoveryStatus);
    (void)recoveryView;
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
        } else if((down&KEY_X) && state.screen==Screen::Party &&
                  editing.stage==phase20::EditStage::Idle &&
                  batch.stage==phase20::BatchStage::Browse &&
                  !batch.collecting) {
            // X on Party launches the 6-Pokémon picker, preserving Y's
            // existing multi-edit queue action and all L-free controls.
            if(state.demo.synthetic)
                viewingMoves=moveView.beginPartySelection(
                                      syntheticMoveCatalog(),state.demo);
            else if(localMoves.read())
                viewingMoves=moveView.beginPartySelection(
                    *localMoves.verifiedMetadata(),state.demo);
            else openNotice(state,Screen::Party,
                "Missing authenticated move metadata in /3ds/PKSM-Rogue/"
                "catalogs/. No move edits are enabled.");
            moveRepeat.reset();
            redraw=true;
        } else if((down&KEY_Y) && editing.stage==phase20::EditStage::Idle &&
                  batch.stage==phase20::BatchStage::Browse) {
            // Conflict resolution: on Pokémon detail, Y opens the
            // read-only browser ONLY if no active v3 batch. In all other
            // contexts Y keeps the existing verified multi-edit workflow.
            if(state.screen==Screen::Pokemon && !state.fromPC &&
               !batch.collecting && selectedPokemonValid(state)) {
                // The 936-row authenticated metadata is supplied separately
                // by the owner and never bundled with an app executable.
                // Synthetic/fake IDs appear only in deliberately demo mode.
                if(state.demo.synthetic)
                    viewingMoves=moveView.initialize(syntheticMoveCatalog(),
                                                       state.demo,state.mon);
                else if(localMoves.read())
                    viewingMoves=moveView.initialize(
                        *localMoves.verifiedMetadata(),state.demo,state.mon);
                else
                    openNotice(state,Screen::Pokemon,
                        "No SHA-authenticated local move metadata. "
                        "Place exact_rom_move_metadata.bin in "
                        "/3ds/PKSM-Rogue/catalogs/ then retry Y. "
                        "No moves can be applied.");
            }else (void)phase20::batchShortcut(state,observation,batch);
            moveRepeat.reset();
            redraw=true;
        } else if(toKey(down & ~(KEY_START|KEY_SELECT|KEY_Y),k)) {
            if(state.screen==Screen::Import && k==Key::A) {
                std::string reason;
                bool inspected=false;
                try {
                    inspected=phase20::inspectFixedSDCopy(active,observation,reason);
                } catch (const std::bad_alloc&) {
                    reason="Insufficient memory for copied-save import.";
                } catch (const std::exception&) {
                    reason="Copied-save inspection failed safely; check the SD copy.";
                }
                if(inspected) {
                    viewingMoves=false;
                    moveRepeat.reset();
                    editing=phase20::OfflineFlow{};
                    batch=phase20::BatchFlow{};
                    switchView(state,false);
                } else {
                    std::snprintf(importStatus,sizeof(importStatus),"%s",reason.c_str());
                    openNotice(state,Screen::Import,importStatus);
                }
            } else if(state.screen==Screen::Import && k==Key::X) {
                populateMock(active);
                viewingMoves=false;
                moveRepeat.reset();
                observation=phase20::CopyObservation{};
                editing=phase20::OfflineFlow{};
                batch=phase20::BatchFlow{};
                switchView(state,true);
            } else if(!phase20::takeBatchKey(state,batch,observation,k) &&
                      !phase20::takeOfflineKey(state,editing,observation,k,
                                          nullptr,nullptr,&batch)) {
                press(state,k);
            }
            redraw=true;
        }
        if(redraw) {
            consoleSelect(&upper);
            if(viewingMoves)moveTop(state,moveView);
            else top(state,editing,batch);
            consoleSelect(&lower);
            if(viewingMoves)moveBottom(state,moveView);
            else bottom(state,editing,batch);
            redraw=false;
        }
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
