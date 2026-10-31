/* PKSM Rogue Edition standalone 3DS READ-ONLY runtime diagnostic.
 * GPL-3.0-or-later. No installed game save, extdata, restore or write API.
 */
#include <3ds.h>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <new>
#include "rogue_bridge.h"

namespace {
constexpr const char* importPath="sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
struct Closer {void operator()(FILE* f)const {if(f)fclose(f);}};
using File=std::unique_ptr<FILE,Closer>;
std::unique_ptr<uint8_t[]> readExact() {
    File input(fopen(importPath,"rb"));
    if(!input)return {};
    std::unique_ptr<uint8_t[]> data(new(std::nothrow) uint8_t[ROGUE_RAW_SAVE_SIZE]);
    if(!data || fread(data.get(),1,ROGUE_RAW_SAVE_SIZE,input.get())!=ROGUE_RAW_SAVE_SIZE
      || fgetc(input.get())!=EOF || ferror(input.get()))return {};
    return data;
}
void show(int page,const rogue_snapshot* snapshot,const rogue_preview& p,unsigned selected,bool loaded){
    consoleClear();
    puts("PKSM ROGUE - 3DS RUNTIME TEST");
    puts("==============================");
    puts("READ ONLY. NO EXT DATA.");
    puts("NO LIVE SAVE ACCESS OR WRITES.");
    printf("PAGE %d/3 - INPUT %s\n\n",page+1,loaded?"PRESENT":"MISSING");
    if(!snapshot){
        puts("No supported SD copy loaded.");
        puts("Optional SD input path:");
        puts("/3ds/PKSM-Rogue/imports/");
        puts("00000001.sav");
        puts("This is NOT a game save restore.");
    } else if(page==0) {
        printf("Marker %u, counter %lu\n",p.serialized_marker,(unsigned long)p.save_counter);
        printf("Money %lu\nParty %u\nBag %u\n",
           (unsigned long)p.money,p.party_count,p.occupied_bag_slots);
        puts("ROM compatibility NOT verified.");
    } else if(page==1){
        puts("PARTY - numerical IDs only");
        if(!p.party_count)puts("(empty)");
        for(unsigned i=0;i<p.party_count && i<6;i++){
            rogue_party_candidate mon{};
            if(rogue_snapshot_party(snapshot,i,&mon))
                printf("%c %u: species %u, HP IV %u\n",
                   selected==i?'>':' ',i+1,mon.species_id,mon.ivs[0]);
        }
    } else {
        puts("BAG - numerical IDs only");
        if(!p.occupied_bag_slots)puts("(empty)");
        for(unsigned i=(selected/7)*7;i<p.occupied_bag_slots && i<(selected/7)*7+7;i++){
            rogue_item_candidate item{};
            if(rogue_snapshot_item(snapshot,i,&item))
                printf("%c slot %u: ID %u x%u\n",
                  selected==i?'>':' ',item.slot_index,item.item_id,item.quantity);
        }
    }
    puts("\nL/R pages   Up/Down select");
    puts("START: safe exit");
    puts("This test CANNOT edit or restore.");
}
}
int main(){
    gfxInitDefault(); consoleInit(GFX_TOP,nullptr);
    Result sd=sdmcInit();
    std::unique_ptr<uint8_t[]> data;
    if(R_SUCCEEDED(sd))data=readExact();
    std::unique_ptr<rogue_snapshot,decltype(&rogue_snapshot_close)> snap(
        data?rogue_snapshot_open(data.get(),ROGUE_RAW_SAVE_SIZE):nullptr,rogue_snapshot_close);
    rogue_preview info{};
    if(snap && !rogue_snapshot_summary(snap.get(),&info))snap.reset();
    int page=0;unsigned selected=0;bool dirty=true;
    while(aptMainLoop()){
        hidScanInput();const u32 keys=hidKeysDown();
        if(keys&KEY_START)break;
        if(keys&KEY_R){page=(page+1)%3;selected=0;dirty=true;}
        if(keys&KEY_L){page=(page+2)%3;selected=0;dirty=true;}
        const unsigned count= !snap?0:page==1?info.party_count:page==2?info.occupied_bag_slots:0;
        if((keys&KEY_DOWN) && count && selected+1<count){selected++;dirty=true;}
        if((keys&KEY_UP) && selected){selected--;dirty=true;}
        if(dirty){show(page,snap.get(),info,selected,data!=nullptr);dirty=false;}
        gspWaitForVBlank();
    }
    snap.reset();data.reset();
    if(R_SUCCEEDED(sd))sdmcExit();
    gfxExit();return 0;
}
