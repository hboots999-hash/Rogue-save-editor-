#include "editor_model.hpp"
#include <fstream>
#include <iostream>
using namespace rogue18;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL MODEL line "<<__LINE__<<": "<<#x<<"\n";return 1;}}while(0)
int main(int argc,char** argv){
    CHECK(argc==2);
    std::ifstream input(argv[1],std::ios::binary);
    Bytes raw(std::istreambuf_iterator<char>(input),{});
    CHECK(raw.size()==h168::Size);
    const auto view=inspectReadOnly(raw.data(),raw.size(),true);
    CHECK(view.valid && view.apiVersion==1);
    CHECK(view.confidence==DataConfidence::StructurallyValidated);
    CHECK(view.activeCounter==41 && view.inactiveCounter==40);
    CHECK(view.money==40001 && view.party.size()==2 && view.items.size()==3);
    CHECK(view.party[0].value.species==25 && view.party[0].value.heldItem==99);
    CHECK(view.pc.status==PcStatus::UnconfirmedEmpty);
    CHECK(!view.capabilities.liveWrite && !view.capabilities.restore);
    CHECK(!view.capabilities.pcEdit && !view.capabilities.nature);
    CHECK(!view.capabilities.arbitraryMoveId && !view.capabilities.insertItem);
    Request req{};
    CHECK(beginRequest(view,Kind::Money,0,req));
    CHECK(req.expectedCounter==41 && req.expectedMoney==40001);
    req.money=40002;
    auto cand=propose(raw.data(),raw.size(),req);
    CHECK(cand.ok);
    CHECK(!beginRequest(view,Kind::Money,1,req));
    CHECK(beginRequest(view,Kind::ExistingQuantity,0,req));
    CHECK(req.expectedItemId==113 && req.expectedQuantity==10);
    req.quantity=11;
    CHECK(propose(raw.data(),raw.size(),req).ok);
    CHECK(!beginRequest(view,Kind::ExistingQuantity,3,req));
    CHECK(beginRequest(view,Kind::IVs,1,req));
    CHECK(req.expectedSpecies==676);
    req.stats={31,31,31,31,31,31};
    CHECK(propose(raw.data(),raw.size(),req).ok);
    CHECK(beginRequest(view,Kind::Move,0,req));
    CHECK(req.expectedMoveId==33 && req.moveId==33 && req.pp==15);
    req.pp=14;
    CHECK(propose(raw.data(),raw.size(),req).ok);
    CHECK(beginRequest(view,Kind::Nature,0,req));
    req.nature=11;
    CHECK(!propose(raw.data(),raw.size(),req).ok);
    Evidence verified{};
    verified.verifiedNatureModifier=true;
    CHECK(propose(raw.data(),raw.size(),req,verified).ok);
    auto bad=raw;bad[0]^=1;
    auto unknown=inspectReadOnly(bad.data(),bad.size());
    CHECK(!unknown.valid && !beginRequest(unknown,Kind::Money,0,req));
    CHECK(!inspectReadOnly(raw.data(),raw.size()-1).valid);
    CHECK(raw.size()==h168::Size);
    std::cout<<"PASS Phase18 -> Phase19 versioned read-only snapshot, permission flags, guarded request factory, no live writes\n";
}
