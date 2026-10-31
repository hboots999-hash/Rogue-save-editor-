/* Phase 12 H168 read-only live backup, GPL-3.0-or-later.
 * Luma-only PxiFS0 read is triggered EXPLICITLY by Y. No installed save
 * writes; old SD-import offline +1 draft code remains unchanged.
 */
#include <3ds.h>
#include <cstdio>
#include <memory>
#include <string>
#include "offline.hpp"
#include "live_read.hpp"
#include "live_backup.hpp"
namespace {
const char* const Path="sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
const char* const Root="sdmc:/3ds/PKSM-Rogue";
struct State {
    unsigned page=0,selected=0,confirm=0,backupConfirm=0; // 0 viewer, 1 and 2 confirm
    bool moneyEdit=true,attempted=false,liveView=false,backupAttempted=false;
    std::string message;
};
void show(const State& s,const h168::Info& info,const uint8_t* data) {
    consoleClear();
    puts("PKSM ROGUE - PHASE 12 LIVE BACKUP");
    puts("--------------------------------");
    puts(s.liveView ? "LIVE SAVE - READ AND BACKUP ONLY" : "EXPORTED SD COPY - OFFLINE EDIT");
    puts("NO LIVE SAVE WRITES OR RESTORE");
    puts("TARGET: 0004000007D7BA00");
    printf("PAGE %u/3\n",s.page+1u);
    if(!data){puts("\nNO SD SAVE IMPORT FOUND");
        puts("/3ds/PKSM-Rogue/imports/");
        puts("00000001.sav (128 KiB)");
        puts("\nPlace a COPY; never move original.");
    } else if(!info.valid){
        puts("\nIMPORT NOT ACCEPTED");
        puts("Need 2 valid sector banks,");
        puts("104-byte party + contiguous bag.");
        puts("No editing available.");
    } else if(s.liveView && s.backupConfirm>0){
        puts("\nBACK UP CURRENT LIVE SAVE?");
        puts("A: verify & back up to SD only");
        puts("B: cancel without writing");
        puts("Installed save is NEVER changed.");
    } else if(s.confirm>0){
        puts("\nOFFLINE +1 DRAFT ONLY");
        puts("Original SD copy is untouched.");
        if(s.moneyEdit)printf("MONEY: %lu -> %lu\n",
            (unsigned long)info.money,(unsigned long)(info.money+1));
        else{
            unsigned id=0,qty=0;
            if(h168::item(data,info,s.selected,id,qty))
                printf("ITEM ID %u: %u -> %u\n",id,qty,qty+1u);
        }
        printf("Confirm %u/2: A\n",s.confirm);
        puts("B: CANCEL, no files written.");
        puts("Do not inject drafts automatically.");
    } else if(s.page==0){
        printf("\nMoney: %lu\n",(unsigned long)info.money);
        printf("Save bank: %u\n",info.active);
        printf("Counter: %lu\n",(unsigned long)info.banks[info.active].counter);
        printf("Party: %u, bag entries: %u\n",info.partyCount,info.items);
        if(!s.liveView)puts("X: MONEY +1 SD-only draft");
    } else if(s.page==1){
        puts("\nPARTY (numeric species IDs)");
        if(!info.partyCount)puts("Party empty");
        for(unsigned i=0;i<info.partyCount;i++)
            printf("%c Slot %u: species ID %u\n",
                s.selected==i?'>':' ',i+1,h168::species(data,info,i));
        puts("Read only; not yet IV editor.");
    } else {
        puts("\nBAG (numeric item IDs)");
        if(!info.items)puts("Bag empty");
        const unsigned first=s.selected/8u*8u;
        for(unsigned i=first;i<info.items&&i<first+8;i++){
            unsigned id=0,qty=0;
            if(h168::item(data,info,i,id,qty))
                printf("%c %u: item ID %u x%u\n",
                    s.selected==i?'>':' ',i+1,id,qty);
        }
        if(!s.liveView)puts("X: item +1 SD-only draft");
    }
    if(!s.message.empty()){
        puts("\nRESULT:");
        puts(s.message.c_str());
    }
    puts("\nL/R page  UP/DOWN select");
    puts(s.liveView ? "A backup live  B return to SD" : "Y: read LIVE GBA VC (read-only)");
    if(!s.liveView)puts("X proposal; A A confirm; B cancel");
    puts("START exit - NO LIVE WRITES.");
}
}
int main(){
    gfxInitDefault();
    consoleInit(GFX_TOP,nullptr);
    auto input=h168::readExact(Path);
    h168::Info info{};
    if(input)h168::inspect(input.get(),h168::Size,info);
    State state;
    std::unique_ptr<uint8_t[]> liveData;
    h168::Info liveInfo{};
    bool dirty=true;
    while(aptMainLoop()){
        hidScanInput();
        const u32 keys=hidKeysDown();
        if(keys&KEY_START)break;
        const h168::Info& shown=state.liveView?liveInfo:info;
        if(state.backupConfirm){
            if(keys&KEY_B){
                state.backupConfirm=0;state.message="Live backup cancelled";dirty=true;
            }else if(keys&KEY_A){
                state.backupConfirm=0;
                state.backupAttempted=true;
                const auto receipt=rogue_live_backup::backupVerified(
                    [](){return rogue_live::loadInstalledReadOnly();},Root);
                state.message=receipt.message;
                dirty=true;
            }
        }else if(state.confirm){
            if(keys&KEY_B){state.confirm=0;state.message="Cancelled: no export";dirty=true;}
            else if(keys&KEY_A){
                if(state.confirm==1){state.confirm=2;dirty=true;}
                else {
                    state.confirm=0;
                    if(state.attempted)state.message="One export per launch; restart.";
                    else{
                        state.attempted=true;
                        const auto r=h168::exportIncrement(input.get(),Path,Root,
                            state.moneyEdit,state.selected);
                        state.message=r.message;
                    }
                    dirty=true;
                }
            }
        }else if(keys&KEY_R){
            state.page=(state.page+1)%3;state.selected=0;state.message.clear();dirty=true;
        }else if(keys&KEY_L){
            state.page=(state.page+2)%3;state.selected=0;state.message.clear();dirty=true;
        }else if(keys&KEY_DOWN){
            const unsigned count=state.page==1?shown.partyCount:state.page==2?shown.items:0;
            if(shown.valid&&state.selected+1<count)state.selected++;
            dirty=true;
        }else if(keys&KEY_UP){
            if(state.selected>0)state.selected--;
            dirty=true;
        }else if(keys&KEY_A){
            if(state.liveView){
                if(state.backupAttempted)state.message="One backup per launch. Restart.";
                else {state.backupConfirm=1;state.message.clear();}
                dirty=true;
            }
        }else if(keys&KEY_B){
            if(state.liveView){state.liveView=false;state.page=0;state.selected=0;}
            state.message.clear();dirty=true;
        }else if(keys&KEY_Y){
            if(!state.liveView){
                auto result=rogue_live::loadInstalledReadOnly();
                state.message=result.stage;
                if(result.ok){
                    liveData=std::move(result.save);
                    liveInfo=result.decoded;
                    state.liveView=true;
                    state.page=0;state.selected=0;
                }
                dirty=true;
            }
        }else if(keys&KEY_X){
            if(state.liveView){state.message="LIVE VIEW is READ ONLY. B for SD.";dirty=true;}
            else if(info.valid&&(state.page==0||state.page==2)&&!state.attempted){
                state.moneyEdit=state.page==0;
                if(state.moneyEdit?info.money<999999u:state.selected<info.items){
                    state.confirm=1;state.message.clear();
                }else state.message="Nothing to edit: at limit/empty";
            }else state.message="Invalid import or already attempted";
            dirty=true;
        }
        if(dirty){
            show(state,state.liveView?liveInfo:info,
                 state.liveView?liveData.get():input.get());
            dirty=false;
        }
        gspWaitForVBlank();
    }
    liveData.reset();input.reset();gfxExit();return 0;
}
