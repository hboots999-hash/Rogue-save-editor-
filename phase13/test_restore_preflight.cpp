// Phase 13 host proof for exact small edited-save payloads; no live saves.
#include "restore_preflight.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
using namespace rogue_restore;
using Bytes=std::vector<uint8_t>;
int main(int argc,char** argv){
    assert(argc==2);
    std::ifstream file(argv[1],std::ios::binary);
    Bytes original((std::istreambuf_iterator<char>(file)),{});
    assert(original.size()==h168::Size);
    constexpr size_t bankSize=14*h168::Sector;
    // deterministic toy fixture has only one save; duplicate bank and
    // increase the newer counter. No real ROM or personal save is used.
    std::memcpy(original.data()+bankSize,original.data(),bankSize);
    for(unsigned k=0;k<14;k++)
        h168::w32(original.data(),bankSize+k*h168::Sector+0xffc,1);
    h168::Info info{};
    assert(h168::inspect(original.data(),original.size(),info));
    assert(info.active==1);
    const Bytes unchanged=original;
    for(auto op:{Operation::MoneyPlusOne,Operation::ItemQuantityPlusOne}){
        Validation proof{};
        auto after=prepare(original.data(),op,0,proof);
        assert(after && proof.ok && proof.newValue==proof.oldValue+1);
        assert(proof.changed>=1 && proof.changed<=6);
        assert(std::memcmp(after.get(),original.data(),bankSize)==0);
        assert(unchanged==original);
        auto checked=validate(original.data(),after.get(),op,0);
        assert(checked.ok && checked.changed==proof.changed);
        assert(!validate(after.get(),original.data(),op,0).ok);
        Bytes tampered(after.get(),after.get()+h168::Size);
        // A structurally valid extra modification cannot be accepted.
        const size_t sector=info.banks[info.active].sector[1];
        tampered[sector+0x20]^=1;
        h168::w16(tampered.data(),sector+0xff6,
                  h168::checksum(tampered.data()+sector));
        h168::Info validTampered{};
        assert(h168::inspect(tampered.data(),tampered.size(),validTampered));
        assert(!validate(original.data(),tampered.data(),op,0).ok);
        tampered[sector+0x20]^=1;
        h168::w16(tampered.data(),sector+0xff6,
                  h168::checksum(tampered.data()+sector));
        assert(validate(original.data(),tampered.data(),op,0).ok);
        // Changed live source must never validate as the old source.
        auto changedOriginal=original;
        changedOriginal[0]^=1;
        assert(!validate(changedOriginal.data(),after.get(),op,0).ok);
        // Corrupt candidate checksum.
        tampered[sector+0xff6]^=1;
        assert(!validate(original.data(),tampered.data(),op,0).ok);
    }
    Validation invalid{};
    assert(!prepare(original.data(),static_cast<Operation>(99),0,invalid));
    assert(!prepare(original.data(),Operation::ItemQuantityPlusOne,999,invalid));
    assert(!validate(original.data(),original.data(),
                     Operation::MoneyPlusOne).ok);
    assert(!validate(nullptr,original.data(),Operation::MoneyPlusOne).ok);
    assert(unchanged==original);
    std::cout<<"PASS Phase 13: exact value delta and footprint, two-bank integrity, stale/corrupt/no-op rejection\n";
}
