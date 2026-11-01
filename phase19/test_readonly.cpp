// Phase19/Phase18 integration test: generated non-playable 128KiB H168 copy.
// Reuse Phase18's existing deterministic fixture, not a private game save.
#define main phase18_regression_reference_main
#include "phase18/test_offline_engine.cpp"
#undef main
#include "read_only_import.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace phase19;
int main(){
    const auto bytes=fixture(); // generated rotated dual-bank H168 synthetic
    assert(bytes.size()==h168::Size);
    static Demo live{};
    populateMock(live);
    std::string reason;
    assert(inspectCopiedBuffer(bytes.data(),bytes.size(),live,reason));
    assert(!live.synthetic);
    assert(live.moneyStatus==Availability::Ready && live.money==40001);
    assert(live.activeCounter==41);
    assert(live.partyStatus==Availability::Ready && live.partyCount==2);
    assert(live.party[0].species==25 && live.party[1].species==676);
    assert(live.party[0].level==0 && live.party[1].level==0);
    assert(std::strcmp(live.party[0].name,"Species 25")==0);
    assert(live.party[0].iv[0]==10 && live.party[0].iv[5]==15);
    assert(live.party[0].ev[1]==252 && live.party[1].ev[3]==252);
    assert(live.party[0].moves[0]==33 && live.party[0].pp[0]==15);
    assert(live.party[1].ability==2); // hidden ability slot, NOT Gmax flag
    assert(live.pcStatus==Availability::Unsupported && live.pcBoxes==0);
    assert(live.bagStatus==Availability::Ready && live.bagCount==3);
    assert(live.bag[0].id==113 && live.bag[0].quantity==10);
    assert(std::strcmp(live.bag[0].name,"Item 113")==0);
    State ui(live);
    press(ui,Key::A); assert(ui.screen==Screen::Party);
    press(ui,Key::A); assert(ui.screen==Screen::Pokemon);
    press(ui,Key::A); assert(ui.screen==Screen::Notice);
    assert(std::strstr(ui.notice,"READ ONLY")!=nullptr);
    press(ui,Key::B);press(ui,Key::B);press(ui,Key::B);
    assert(ui.screen==Screen::Home);
    ui.homeRow=3;press(ui,Key::A);press(ui,Key::A);
    assert(ui.screen==Screen::Notice); // money changes prohibited
    press(ui,Key::B);press(ui,Key::B);
    ui.homeRow=2;press(ui,Key::A);press(ui,Key::A);
    assert(ui.screen==Screen::Notice); // bag changes prohibited
    press(ui,Key::B);press(ui,Key::B);
    ui.homeRow=1;press(ui,Key::A);assert(count(ui)==0);
    press(ui,Key::A);assert(ui.screen==Screen::Notice); // PC not decoded
    press(ui,Key::B);press(ui,Key::B);
    ui.homeRow=6;press(ui,Key::A);assert(ui.screen==Screen::Import);
    press(ui,Key::B);assert(ui.screen==Screen::Home);

    // No side effect or fallback to mock data on invalid file.
    static Demo baseline{};
    populateMock(baseline);
    const unsigned original=baseline.money;
    assert(!inspectCopiedBuffer(bytes.data(),bytes.size()-1,baseline,reason));
    assert(baseline.synthetic && baseline.money==original);
    assert(reason.find("128 KiB")!=std::string::npos);
    auto damaged=bytes;damaged[100]=0xFF;
    assert(!inspectCopiedBuffer(damaged.data(),damaged.size(),baseline,reason));
    assert(baseline.synthetic && baseline.money==original);
    // A wrapped GBA VC container cannot pass this fixed unwrapped parser.
    damaged.resize(263168);
    assert(!inspectCopiedBuffer(damaged.data(),damaged.size(),baseline,reason));
    assert(baseline.synthetic && baseline.money==original);
    assert(live.money==40001 && live.party[0].species==25);
    std::cout<<"PASS Phase19: pinned Phase18 H168 copied-data read only bridge, no writes\n";
}
