// Phase18 PC-only fixture / negative tests: no private save.
#include "pc_readonly.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace rogue18;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL PC line "<<__LINE__<<": "<<#x<<"\n";return 1;}}while(0)
static void setPC(Bytes& b,const h168::Bank& bank,size_t logical,uint8_t value){
    const size_t logicalSector=logical/PcSectionData;
    const size_t within=logical%PcSectionData;
    assert(logicalSector<PcSections);
    b[bank.sector[5+logicalSector]+within]=value;
}
static void refresh(Bytes& b,const h168::Bank& bank){
    for(unsigned i=5;i<14;i++){
        const auto pos=bank.sector[i];
        h168::w16(b.data(),pos+0xff6,h168::checksum(b.data()+pos));
    }
}
int main(int argc,char** argv){
    CHECK(argc==2);
    std::ifstream f(argv[1],std::ios::binary);
    Bytes base(std::istreambuf_iterator<char>(f),{});
    CHECK(base.size()==h168::Size);
    h168::Info info{};
    CHECK(validate(base.data(),base.size(),info));
    auto empty=inspectPcCandidate(base.data(),base.size());
    CHECK(empty.status==PcStatus::UnconfirmedEmpty && empty.occupied==0);
    Bytes b=base;
    const auto bank=info.banks[info.active];
    const auto s1=bank.sector[1];
    for(const unsigned index:{0u,102u}){
        const size_t src=s1+0x238+(index==0?0:104);
        const size_t dst=PcBoxArrayOffset+index*PcBoxStride;
        for(size_t i=0;i<PcBoxStride;i++)
            setPC(b,bank,dst+i,base[src+i]);
        setPC(b,bank,dst+19,uint8_t(base[src+19]|2u)); // source hasSpecies
    }
    refresh(b,bank);
    CHECK(validate(b.data(),b.size(),info));
    const auto pc=inspectPcCandidate(b.data(),b.size());
    CHECK(pc.status==PcStatus::StructuralCandidate && pc.occupied==2);
    CHECK(pc.slots.size()==2);
    CHECK(pc.slots[0].box==0 && pc.slots[0].index==0 && pc.slots[0].species==25);
    CHECK(pc.slots[1].box==3 && pc.slots[1].index==12 && pc.slots[1].species==676);
    CHECK(pc.slots[1].heldItem==99);
    CHECK(b==b); // inspection never mutates memory
    auto corrupt=b;
    setPC(corrupt,bank,PcBoxArrayOffset+102*PcBoxStride+32,0xff);
    refresh(corrupt,bank); // sector checksum repaired, mon checksum damaged
    CHECK(validate(corrupt.data(),corrupt.size(),info));
    CHECK(inspectPcCandidate(corrupt.data(),corrupt.size()).status==PcStatus::Corrupt);
    corrupt=b;
    setPC(corrupt,bank,0,14);refresh(corrupt,bank);
    CHECK(inspectPcCandidate(corrupt.data(),corrupt.size()).status==PcStatus::Corrupt);
    corrupt=b;
    setPC(corrupt,bank,PcBoxArrayOffset+102*PcBoxStride+19,0);
    refresh(corrupt,bank);
    CHECK(inspectPcCandidate(corrupt.data(),corrupt.size()).status==PcStatus::Corrupt);
    CHECK(inspectPcCandidate(b.data(),b.size()-1).status==PcStatus::Unsupported);
    CHECK(base==Bytes(std::istreambuf_iterator<char>(std::ifstream(argv[1],std::ios::binary)),{}));
    std::cout<<"PASS Phase18 source-backed PC structural candidate, section crossing, absent PC, corrupt data and fail closed\n";
}
