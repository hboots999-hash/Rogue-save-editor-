// SPDX-License-Identifier: GPL-3.0-or-later
// Phase16: READ-ONLY GBA VC + isolated SD recovery-journal rehearsal.
// NEVER write, replace, delete or mount installed game SAVE as writable.
#include <3ds.h>
#include <cstdio>
#include <string>
#include "diagnostic.hpp"
#include "native_pxi.hpp"
#include "native_store.hpp"
#include "rehearsal.hpp"
static_assert(!phase14::LiveWritesEnabled,"Live game restoration is disabled");
namespace {
struct Screen {
    bool confirming=false,attempted=false,checked=false,hasDiagnostic=false;
    phase16::Result test{};
    phase15::Report diagnostic{};
    ::Result lastNativeCode=0;
};
const char* showPresence(phase16::State v){
    switch(v){
        case phase16::State::Missing:return "NOT PRESENT";
        case phase16::State::Present:return "PRESENT: preserved";
        case phase16::State::Error:return "SD ERROR: blocked";
    }
    return "UNKNOWN";
}
void draw(const Screen& s) {
    consoleClear();
    puts("PKSM ROGUE - PHASE16 JOURNAL TEST");
    puts("--------------------------------");
    puts("EMERALD ROGUE H168: READ ONLY");
    puts("NO LIVE GAME WRITES / RESTORE");
    puts("Uses separate SD:/3ds/PKSM-Rogue/phase16");
    puts("");
    phase16::NativeStore disk{};
    printf("Full backup: %s\n",showPresence(disk.backupState()));
    printf("Trial journal: %s\n",showPresence(disk.journalState()));
    puts("");
    if(s.hasDiagnostic)
        printf("LIVE CMAC: %s\n",phase15::label(s.diagnostic.code));
    if(s.checked) {
        printf("RESULT: %s\n",phase16::label(s.test.code));
        printf("Backup bytes: %lu\n",
            static_cast<unsigned long>(s.test.originalSize));
        printf("Journal bytes: %lu\n",
            static_cast<unsigned long>(s.test.journalSize));
        printf("Prior counter: %lu\n",
            static_cast<unsigned long>(s.test.currentCounter));
        printf("Live unchanged? %s\n",s.test.liveSafe?"YES":"NOT CONFIRMED");
        if(!s.test.liveSafe)
            printf("Last PXI rc: %08lX\n",
                static_cast<unsigned long>(static_cast<uint32_t>(s.lastNativeCode)));
    }
    puts("");
    if(s.confirming){
        puts("CREATE IMMUTABLE FULL SD BACKUP");
        puts("+ ISOLATED JOURNAL (about 790 KB)");
        puts("Contains PRIVATE save information!");
        puts("NEVER touches installed game save.");
        puts("A: CONFIRM   B: CANCEL");
    }else{
        puts("X: check live slot CMAC (read-only)");
        puts("A A: create SD backup + journal ONCE");
        puts("Y: reopen & verify saved journal");
        puts("B: clear result   START: exit");
    }
    puts("Leave SD files intact; NO restore.");
}
}
int main(){
    gfxInitDefault();
    consoleInit(GFX_TOP,nullptr);
    Screen s{};
    bool dirty=true;
    while(aptMainLoop()){
        hidScanInput();
        const u32 key=hidKeysDown();
        if(key&KEY_START)break;
        if(s.confirming) {
            if(key&KEY_B){s.confirming=false;dirty=true;}
            else if(key&KEY_A){
                s.confirming=false;s.attempted=true;
                phase14::NativePxi live;
                phase16::NativeStore disk;
                s.test=phase16::createRehearsal(live,disk);
                s.lastNativeCode=live.last;
                const bool closed=live.close();
                if(!closed && s.test.code==phase16::Code::Created){
                    s.test.code=phase16::Code::RecheckReadError;
                    s.test.liveSafe=false;
                }
                s.checked=true;dirty=true;
            }
        }else if(key&KEY_X){
            phase14::NativePxi live;
            s.diagnostic=phase15::run(live);
            s.lastNativeCode=live.last;
            const bool closed=live.close();
            if(!closed&&s.diagnostic.code==phase15::Code::Pass)
                s.diagnostic.code=phase15::Code::FirstReadFailed;
            s.hasDiagnostic=true;dirty=true;
        }else if(key&KEY_Y){
            phase14::NativePxi live;
            phase16::NativeStore disk;
            s.test=phase16::verifyAfterRestart(live,disk);
            s.lastNativeCode=live.last;
            const bool closed=live.close();
            if(!closed && s.test.code==phase16::Code::Verified){
                s.test.code=phase16::Code::RecheckReadError;
                s.test.liveSafe=false;
            }
            s.checked=true;dirty=true;
        }else if(key&KEY_A){
            if(!s.attempted)s.confirming=true;
            dirty=true;
        }else if(key&KEY_B){
            s.checked=false;s.hasDiagnostic=false;
            s.test=phase16::Result{};
            s.diagnostic=phase15::Report{};
            dirty=true;
        }
        if(dirty){draw(s);dirty=false;}
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
