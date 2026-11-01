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
} // namespace phase18_native_compile
