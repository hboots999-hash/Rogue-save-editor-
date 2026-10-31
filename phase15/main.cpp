// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental on-device Phase 15. Live GBA container opened FS_OPEN_READ only.
#include <3ds.h>
#include <cerrno>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include "diagnostic.hpp"
#include "native_pxi.hpp"
#include "native_journal.hpp"
#include "sd_probe.hpp"
static_assert(!phase14::LiveWritesEnabled,"LIVE RESTORATION MUST REMAIN DISABLED");
namespace {
struct Screen {
    phase15::Report report{};
    bool checked=false,sdAttempted=false,sdConfirm=false;
    std::string sdStatus;
    ::Result nativeError=0;
};
const char* recordState(){
    struct stat st{};
    errno=0;
    if(::stat("sdmc:/3ds/PKSM-Rogue/recovery/phase14.rec",&st)==0)
        return "PRESENT: preserve recovery journal";
    if(errno==ENOENT)return "No Phase14 recovery record";
    return "UNKNOWN: journal path inaccessible";
}
void show(const Screen& s){
    consoleClear();
    puts("PKSM ROGUE - PHASE 15 DIAGNOSTIC");
    puts("--------------------------------");
    puts("H168 2.1.2 - READ ONLY");
    puts("NO LIVE WRITES / RESTORE");
    puts("Title: 0004000007D7BA00");
    puts("");
    if(!s.checked)puts("Y: verify live GBA VC container");
    else{
        printf("RESULT: %s\n",phase15::label(s.report.code));
        printf("Container bytes: %lu\n",
            static_cast<unsigned long>(s.report.containerBytes));
        printf("Two identical reads: %s\n",s.report.stable?"YES":"NO");
        for(unsigned i=0;i<2;i++){
            const auto& slot=s.report.slots[i];
            printf("Slot %u: HDR %c RAW %c CMAC %c\n",i,
                slot.header?'Y':'-',slot.raw?'Y':'-',
                slot.cmac?'Y':'-');
            printf("   Wrapper counter: %lu\n",
                static_cast<unsigned long>(slot.counter));
        }
        if(s.report.code==phase15::Code::Pass)
            printf("Latest slot: %lu, counter: %lu\n",
                static_cast<unsigned long>(s.report.activeSlot),
                static_cast<unsigned long>(s.report.latestCounter));
        else printf("Native last rc: 0x%08lX\n",
            static_cast<unsigned long>(static_cast<uint32_t>(s.nativeError)));
        puts("PASS is diagnostic, NOT restore OK.");
    }
    puts("");
    puts("RECOVERY JOURNAL (read-only status):");
    puts(recordState());
    puts("");
    if(s.sdConfirm){
        puts("CREATE A NEW 256-BYTE SD TEST?");
        puts("A confirm   B cancel");
        puts("Does NOT access the game save.");
    }else {
        puts("A: optional SD-only scratch test");
        if(s.sdAttempted)puts(s.sdStatus.c_str());
    }
    puts("");
    puts("Y: run read-only validation");
    puts("START: exit. NEVER edits game.");
}
} // namespace
int main(){
    gfxInitDefault();
    consoleInit(GFX_TOP,nullptr);
    Screen state{};
    bool dirty=true;
    while(aptMainLoop()){
        hidScanInput();
        const u32 keys=hidKeysDown();
        if(keys&KEY_START)break;
        if(state.sdConfirm){
            if(keys&KEY_B){
                state.sdConfirm=false;dirty=true;
            }else if(keys&KEY_A){
                state.sdConfirm=false;
                state.sdAttempted=true;
                const auto result=phase15::scratchProbe();
                state.sdStatus=result.message;
                dirty=true;
            }
        }else if(keys&KEY_Y){
            phase14::NativePxi device;
            state.report=phase15::run(device);
            state.nativeError=device.last;
            const bool closed=device.close();
            if(!closed && state.report.code==phase15::Code::Pass)
                state.report.code=phase15::Code::FirstReadFailed;
            state.checked=true;dirty=true;
        }else if(keys&KEY_A){
            if(!state.sdAttempted)state.sdConfirm=true;
            dirty=true;
        }else if(keys&KEY_B){
            state.checked=false;
            state.report=phase15::Report{};
            dirty=true;
        }
        if(dirty){show(state);dirty=false;}
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
