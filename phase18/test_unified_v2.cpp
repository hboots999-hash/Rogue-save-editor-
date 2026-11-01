// Deterministic combined-translation-unit compile probe for QA-08.
// The Phase17 branch supplies GLOBAL ::h168, and Phase18 uses only
// rogue18::raw_v1. No restore code is invoked.
// Build with pinned Phase17 sources through CI, not duplicated source files.
#include "recovery_evidence_gate.hpp"
#include "integration_v2.hpp"
#include <type_traits>
static_assert(rogue18::v2::ContractVersion==2,"Version mismatch");
static_assert(!std::is_same<rogue18::raw_v1::Info,::h168::Info>::value,
              "Separate Phase17 wrapper and Phase18 inner raw-save types");
static_assert(sizeof(rogue18::raw_v1::Info)>0 &&
              sizeof(::h168::Info)>0,"Both independent headers compile");
int main(){
    const auto a=rogue18::v2::capability(rogue18::Kind::Money);
    const rogue18::Request empty{};
    return (a.canRestoreInstalledGame ||
            rogue18::v2::mayRestoreInstalledGame(empty) ||
            phase14::LiveWritesEnabled)?1:0;
}
