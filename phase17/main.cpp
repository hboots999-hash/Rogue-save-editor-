// SPDX-License-Identifier: GPL-3.0-or-later
// Phase17 RESTRICTED READ-ONLY hardware diagnostic. RAM SHADOW writes only.
// This UI has no native create, privileged write, or game-save write handles.
#include <3ds.h>
#include <cstdio>
#include <string>
#include "native_pxi.hpp"
#include "native_store.hpp"
#include "rehearsal.hpp"
#include "rollback_lab.hpp"
#include "rosalina_exit_gate.hpp"
static_assert(!phase14::LiveWritesEnabled,"GBA VC write permission is forbidden");
namespace {
struct Screen {
    phase16::Result lastVerification{};
    phase17::Report lab{};
    bool verified=false,labRan=false;
    ::Result nativeLast=0;
};
// Protect against reintroducing the oversized native stack frame that
// previously crashed an unrelated experimental Phase19 app.
static_assert(sizeof(Screen)<512,
              "Phase17 native screen state must remain stack-small");
void draw(const Screen& state) {
    consoleClear();
    puts("PKSM ROGUE - PHASE17 RAM ROLLBACK");
    puts("--------------------------------");
    puts("LIVE GBA VC: READ ONLY");
    puts("RESTORE ENABLED: NO");
    puts("RAM-ONLY shadow transaction tests");
    puts("");
    printf("Phase16 journal: %s\n",
        state.verified?"VERIFIED":"NOT YET VERIFIED");
    if(state.verified){
        printf("Backup: %lu bytes\n",
               static_cast<unsigned long>(state.lastVerification.originalSize));
        printf("Journal: %lu bytes\n",
               static_cast<unsigned long>(state.lastVerification.journalSize));
        printf("Source counter: %lu\n",
               static_cast<unsigned long>(state.lastVerification.currentCounter));
    }
    if(state.lastVerification.code!=phase16::Code::NotRun &&
       !state.verified)
        printf("Read-only verification: %s\n",
               phase16::label(state.lastVerification.code));
    puts("");
    if(state.labRan) {
        printf("TEST RESULT: %s\n",phase17::label(state.lab.code));
        printf("RAM restore/undo: %u / %u\n",
               state.lab.normalRestores,state.lab.explicitRollbacks);
        printf("Short-write recoveries: %u / 4\n",
               state.lab.shortWriteRollbacks);
        printf("Crash restart recoveries: %u / 4\n",
               state.lab.crashRestarts);
        printf("Rollback short retries: %u / 4\n",
               state.lab.failedRollbackRetries);
        printf("Rollback crash retries: %u / 4\n",
               state.lab.interruptedRollbackRetries);
        printf("Foreign-change refusals: %u / 1\n",
               state.lab.foreignChangeBlocks);
        printf("Bytes recovered exactly: %s\n",
               state.lab.allByteExact?"YES":"NO");
    }
    puts("");
    puts("X: reopen Phase16 journal & live CMAC");
    puts("Y: simulate rollback IN RAM only");
    puts("B: clear screen    START: exit");
    puts("NO GBA VC game-save writes.");
}
}
int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP,nullptr);
    Screen s{};
    phase17::StartExitGate exitGate{};
    bool redraw=true;
    while(aptMainLoop()){
        hidScanInput();
        const u32 down=hidKeysDown(),held=hidKeysHeld(),up=hidKeysUp();
        // Do not consume START+SELECT Rosalina in either press order.
        // Standalone START exits only on release, not on key-down.
        if(exitGate.tick((down&KEY_START)!=0,(up&KEY_START)!=0,
                         (down&KEY_SELECT)!=0,(held&KEY_SELECT)!=0))
            break;
        // No recovery test is mapped to L. Ignore other buttons while
        // START/SELECT is held, so opening Rosalina cannot trigger X/Y.
        const u32 k=((down|held)&(KEY_START|KEY_SELECT))?
                    0u:(down&~(KEY_START|KEY_SELECT));
        if(k&KEY_X){
            phase14::NativePxi device;
            phase16::NativeStore files;
            s.lastVerification=phase16::verifyAfterRestart(device,files);
            s.nativeLast=device.last;
            const bool safelyClosed=device.close();
            s.verified=s.lastVerification.code==phase16::Code::Verified &&
                       safelyClosed;
            s.labRan=false;s.lab=phase17::Report{};
            redraw=true;
        }
        else if(k&KEY_Y){
            // Always recheck against the current live container first. This
            // refuses old journals if the game has been saved since Phase16.
            phase14::NativePxi device;
            phase16::NativeStore files;
            const auto check=phase16::verifyAfterRestart(device,files);
            s.lastVerification=check;
            s.verified=check.code==phase16::Code::Verified;
            s.nativeLast=device.last;
            s.labRan=false;s.lab=phase17::Report{};
            if(s.verified){
                phase14::Bytes original,journal;
                if(files.readBackup(original)&&files.readJournal(journal)){
                    // device still holds a read-only file-bound MAC service;
                    // shadow rollback modifies heap vectors ONLY.
                    s.lab=phase17::rehearse(original,journal,device);
                }
                else s.lab.code=phase17::ResultCode::BadJournal;
                s.labRan=true;
            }
            const bool safelyClosed=device.close();
            if(!safelyClosed){
                s.verified=false;
                s.lab.code=phase17::ResultCode::InvalidSignedPlan;
                s.labRan=true;
            }
            redraw=true;
        }
        else if(k&KEY_B){
            s=Screen{};
            redraw=true;
        }
        if(redraw){draw(s);redraw=false;}
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
