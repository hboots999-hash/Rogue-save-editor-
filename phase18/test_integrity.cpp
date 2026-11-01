// Independent Phase18 regression of SHA-256 snapshot locks, source layout,
// empty-party handling and modular save-counter rollover. Synthetic only.
#include "editor_model.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace rogue18;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL integrity "<<__LINE__<<": "<<#x<<"\n";return 1;}}while(0)
static std::string hexDigest(const Digest256& d){
    static const char* h="0123456789abcdef";
    std::string out;
    for(uint8_t b:d){out+=h[b>>4];out+=h[b&15];}
    return out;
}
static void setBankCounter(Bytes& b,unsigned bank,uint32_t counter){
    for(unsigned p=0;p<14;p++)h168::w32(b.data(),
        (bank*14+p)*h168::Sector+0xffc,counter);
}
int main(int argc,char** argv){
    CHECK(argc==2);
    const auto* empty=reinterpret_cast<const uint8_t*>("");
    const auto* abc=reinterpret_cast<const uint8_t*>("abc");
    CHECK(hexDigest(sourceDigest(empty,0))==
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(hexDigest(sourceDigest(abc,3))==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(!hasDigest(Digest256{}));
    CHECK(layout::PartyStart+layout::PartyMax*layout::PartyStride==layout::Money);
    CHECK(layout::Money==0x4a8 && layout::BagArray==0x578);
    CHECK(layout::BagUpgradeCandidate==0x4ad);
    std::ifstream in(argv[1],std::ios::binary);
    Bytes raw(std::istreambuf_iterator<char>(in),{});
    CHECK(raw.size()==h168::Size);
    const auto view=inspectReadOnly(raw.data(),raw.size());
    CHECK(view.valid && view.party.size()==2 && view.money==40001);
    CHECK(hasDigest(view.sourceSha256) && view.sourceSha256==sourceDigest(raw.data(),raw.size()));
    CHECK(view.bagUpgradeCandidate==0);
    Request money{};
    CHECK(beginRequest(view,Kind::Money,0,money));
    CHECK(money.expectedDigest==view.sourceSha256);
    money.money=40002;
    CHECK(propose(raw.data(),raw.size(),money).ok);
    // Counter and money are unchanged, but a tail byte differs. Only the
    // SHA-256 snapshot guard catches this otherwise valid stale copy.
    Bytes changed=raw;
    changed.back()^=0x80;
    h168::Info v{};
    CHECK(validate(changed.data(),changed.size(),v));
    CHECK(v.money==view.money && v.banks[v.active].counter==view.activeCounter);
    CHECK(!propose(changed.data(),changed.size(),money).ok);
    CHECK(!verifyExact(changed.data(),changed.size(),
          raw.data(),raw.size(),money));
    CHECK(propose(raw.data(),raw.size(),money).ok);

    // UINT32_MAX -> zero is a valid adjacent rollover; both banks exist.
    changed=raw;
    setBankCounter(changed,0,0xffffffffu);
    setBankCounter(changed,1,0u);
    CHECK(validate(changed.data(),changed.size(),v));
    CHECK(v.active==1 && v.money==40001 && v.items==3);
    auto wrapView=inspectReadOnly(changed.data(),changed.size());
    CHECK(wrapView.valid && wrapView.activeCounter==0 && wrapView.inactiveCounter==0xffffffffu);
    CHECK(beginRequest(wrapView,Kind::Money,0,money));
    money.money=50000;
    auto result=propose(changed.data(),changed.size(),money);
    CHECK(result.ok);
    CHECK(validate(result.candidate.data(),result.candidate.size(),v));
    CHECK(v.active==1 && v.money==50000);
    CHECK(verifyExact(changed.data(),changed.size(),
                      result.candidate.data(),result.candidate.size(),money));

    // Reverse rollover: bank 0 is now the successor to bank 1.
    changed=raw;
    setBankCounter(changed,0,0u);
    setBankCounter(changed,1,0xffffffffu);
    CHECK(validate(changed.data(),changed.size(),v));
    CHECK(v.active==0 && v.money==40000);
    wrapView=inspectReadOnly(changed.data(),changed.size());
    CHECK(wrapView.valid && wrapView.activeBank==0 && wrapView.activeCounter==0);
    CHECK(beginRequest(wrapView,Kind::Money,0,money));
    money.money=50000;
    CHECK(propose(changed.data(),changed.size(),money).ok);

    // Counter gaps, ambiguous equality and truncation all fail closed.
    changed=raw;
    setBankCounter(changed,0,42);
    setBankCounter(changed,1,44);
    CHECK(!validate(changed.data(),changed.size(),v));
    setBankCounter(changed,1,42);
    CHECK(!validate(changed.data(),changed.size(),v));
    CHECK(!validate(raw.data(),raw.size()-1,v));

    // A legitimate no-Pokémon party is not grounds to disable money editing.
    changed=raw;
    CHECK(validate(changed.data(),changed.size(),v));
    for(unsigned b=0;b<2;b++){
        const auto sector=v.banks[b].sector[1];
        h168::w32(changed.data(),sector+layout::PartyCount,0);
        h168::w16(changed.data(),sector+0xff6,
                   h168::checksum(changed.data()+sector));
    }
    CHECK(validate(changed.data(),changed.size(),v));
    CHECK(v.partyCount==0);
    const auto noParty=inspectReadOnly(changed.data(),changed.size());
    CHECK(noParty.valid && noParty.party.empty());
    CHECK(!beginRequest(noParty,Kind::IVs,0,money));
    CHECK(beginRequest(noParty,Kind::Money,0,money));
    money.money=40002;
    CHECK(propose(changed.data(),changed.size(),money).ok);
    std::cout<<"PASS Phase18 SHA256 known vectors, no-stale-copy, money+bag layout, counter rollover, no-party, ambiguous counters\n";
}
