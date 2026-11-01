// Compile-only compatibility check for Phase 18 headers on Nintendo 3DS.
// No main() or runtime actions.  No privileged filesystem/game archive calls.
#include "editor_model.hpp"
#include "offline_export.hpp"
namespace phase18_native_compile {
bool smoke(const uint8_t* copied,uint32_t n){
    const rogue18::SaveView view=rogue18::inspectReadOnly(copied,n,false);
    rogue18::Request request{};
    return view.valid &&
           rogue18::beginRequest(view,rogue18::Kind::Money,0,request) &&
           !view.capabilities.liveWrite && !view.capabilities.restore;
}
// Compile and symbol-audit the complete SD-only exporter, not just
// the read-only model. This is NOT invoked by a 3DS application or test.
bool compileOfflineExport(const uint8_t* copied,uint32_t n,
                          const rogue18::Request& guarded){
    const auto result=rogue18::exportCopied(
        copied,n,"sdmc:/3ds/PKSM-Rogue/imports/00000001.sav",
        "sdmc:/3ds/PKSM-Rogue",guarded);
    return result.ok && !rogue18::v2::mayRestoreInstalledGame(guarded);
}
} // namespace phase18_native_compile
