// Native ARM11 compile-only instantiation of full Phase18 v3 batch API.
// Test link excludes all privileged game/title/restoration APIs.
#include "multi_export_v3.hpp"
namespace phase18_native_multi_v3 {
bool inspectAndPreview(const uint8_t* bytes,uint32_t n){
    const auto snapshot=rogue18::v2::inspect(bytes,n);
    rogue18::v3::Batch batch{};
    if(!rogue18::v3::start(snapshot,batch))return false;
    if(!rogue18::v3::append(snapshot,batch,rogue18::Kind::Money,0,0,0))
        return false;
    const auto preview=rogue18::v3::preview(bytes,n,batch);
    return preview.ok && !rogue18::v3::mayWriteInstalledGame(batch);
}
bool compileSDExport(const uint8_t* bytes,uint32_t n,
                     const rogue18::v3::Batch& batch){
    const auto result=rogue18::v3::exportCopiedBatch(
        bytes,n,"sdmc:/3ds/PKSM-Rogue/imports/00000001.sav",
        "sdmc:/3ds/PKSM-Rogue",batch);
    return result.ok && !rogue18::v3::mayWriteInstalledGame(batch);
}
} // namespace phase18_native_multi_v3
