/* Phase 10 H168 hardware evaluation preview; GPL-3.0-or-later.
 * Reads ONLY sdmc:/3ds/PKSM-Rogue/imports/00000001.sav.
 * Never accesses AGBSAVE, live installed title, extdata, or other installed app.
 * Exports after double confirmation only to separate verified offline files.
 */
#include <3ds.h>
#include <cstdio>
#include <memory>
#include <string>
#include "offline.hpp"
namespace {
const char* const Path="sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
const char* const Root="sdmc:/3ds/PKSM-Rogue";
struct State {
    unsigned page=0,selected=0,confirm=0; // 0 viewer, 1 and 2 confirm
    bool moneyEdit=true,attempted=false;
    std::string message;
};
void show(const State& s,const h168::Info& info,const uint8_t* data) {
    consoleClear();
    puts("PKSM ROGUE - H168 HARDWARE PREVIEW");
    puts("--------------------------------");
    puts("EXPERIMENTAL / OFFLINE SD ONLY");
    puts("NO LIVE GAME ACCESS OR RESTORE");
    puts("ROM ID NOT CHECKED ON THIS 3DS");
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
        puts("X: create MONEY +1 proposal");
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
        puts("X: selected item +1 proposal");
    }
    if(!s.message.empty()){
        puts("\nRESULT:");
        puts(s.message.c_str());
    }
    puts("\nL/R page  UP/DOWN select");
    puts("X proposal; A A confirm; B cancel");
    puts("START exit - no live restore.");
}
}
int main(){
    gfxInitDefault();
    consoleInit(GFX_TOP,nullptr);
    auto input=h168::readExact(Path);
    h168::Info info{};
    if(input)h168::inspect(input.get(),h168::Size,info);
    State state;
    bool dirty=true;
    while(aptMainLoop()){
        hidScanInput();
        const u32 keys=hidKeysDown();
        if(keys&KEY_START)break;
        if(state.confirm){
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
            const unsigned count=state.page==1?info.partyCount:state.page==2?info.items:0;
            if(info.valid&&state.selected+1<count)state.selected++;
            dirty=true;
        }else if(keys&KEY_UP){
            if(state.selected>0)state.selected--;
            dirty=true;
        }else if(keys&KEY_B){state.message.clear();dirty=true;
        }else if(keys&KEY_X){
            if(info.valid&&(state.page==0||state.page==2)&&!state.attempted){
                state.moneyEdit=state.page==0;
                if(state.moneyEdit?info.money<999999u:state.selected<info.items){
                    state.confirm=1;state.message.clear();
                }else state.message="Nothing to edit: at limit/empty";
            }else state.message="Invalid import or already attempted";
            dirty=true;
        }
        if(dirty){show(state,info,input.get());dirty=false;}
        gspWaitForVBlank();
    }
    input.reset();gfxExit();return 0;
}
