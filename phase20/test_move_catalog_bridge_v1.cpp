// Fail-closed Phase20 exact-ROM metadata bridge tests. NEVER use real ROM.
// Invalid metadata must never produce a trusted move catalogue or Apply.
#include "phase20/move_catalog_bridge_v1.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
int main(){
    static phase20::LocalMoveProvider provider{}; // fixed BSS, not ARM11 stack
    static_assert(sizeof(phase20::LocalMoveProvider)<65536u,
                  "Catalog must remain a bounded 3DS static object");
    static_assert(!rogue18::move_v2::ApplyApproved,
                  "No move replacement from this browser");
    assert(!provider.verifiedMetadata());
    assert(!provider.read(nullptr));
    assert(!provider.read("/tmp/pksm-phase20-catalog-does-not-exist"));
    const char* path="/tmp/pksm-phase20-invalid-catalog.bin";
    for(size_t bytes: {0u,1u,static_cast<unsigned>(rogue18::move_v2::PackedBytes),
                       static_cast<unsigned>(rogue18::move_v2::PackedBytes+1u)}) {
        std::vector<char> synthetic(bytes,'Q');
        {std::ofstream out(path,std::ios::binary|std::ios::trunc);
         if(!synthetic.empty())out.write(synthetic.data(),synthetic.size());}
        assert(!provider.read(path));
        assert(!provider.loaded && !provider.verifiedMetadata());
    }
    std::remove(path);
    const auto& demo=phase19::syntheticMoveCatalog();
    assert(demo.origin==phase19::CatalogOrigin::Synthetic);
    assert(phase19::validMoveCatalog(demo));
    static phase19::Demo d{};
    phase19::populateMock(d);
    static phase19::MoveBrowser browser{};
    assert(browser.initialize(demo,d,0));
    assert(browser.page==phase19::MovePage::Slots);
    assert(browser.browse(d,phase19::MoveKey::A));
    assert(browser.browse(d,phase19::MoveKey::A));
    assert(browser.page==phase19::MovePage::Review);
    assert(browser.browse(d,phase19::MoveKey::A));
    assert(browser.page==phase19::MovePage::Notice);
    assert(!rogue18::move_v2::ApplyApproved);
    std::cout<<"PASS Phase20 optional exact-ROM metadata rejects bad inputs; "
                "synthetic browser cannot Apply or export\n";
}
