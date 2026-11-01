#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
// Phase19/Phase18 integration test: generated non-playable 128KiB H168 copy.
// Reuse Phase18's existing deterministic fixture, not a private game save.
#define main phase18_regression_reference_main
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "phase18/test_offline_engine.cpp"
#pragma GCC diagnostic pop
#undef main
#include "ui.hpp"
#include "read_only_import.hpp"
// Host-only comparison against the Phase18 heap-backed versioned API.
// NEVER include this in 3DS native main: PC research uses a 37 KiB stack frame.
#include "phase18/editor_model.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace phase19;
#if defined(__linux__)
// GNU cookie-based SD read fault simulation: claims an exact 128-KiB file
// during seek/ftell, then returns early EOF or EIO during the read.
struct FaultyCopy {
    const uint8_t* bytes;
    size_t available;
    size_t position=0;
    bool ioError=false;
};
static ssize_t faultyRead(void* context,char* dest,size_t requested){
    auto& s=*static_cast<FaultyCopy*>(context);
    if(s.ioError && s.position>=s.available){
        errno=EIO;
        return -1;
    }
    const size_t remaining=s.position<s.available?s.available-s.position:0u;
    const size_t n=std::min(requested,remaining);
    if(n){
        std::memcpy(dest,s.bytes+s.position,n);
        s.position+=n;
    }
    return static_cast<ssize_t>(n);
}
static int faultySeek(void* context,off64_t* offset,int whence){
    auto& s=*static_cast<FaultyCopy*>(context);
    off64_t position=0;
    if(whence==SEEK_SET) position=*offset;
    else if(whence==SEEK_CUR) position=static_cast<off64_t>(s.position)+*offset;
    else if(whence==SEEK_END) position=static_cast<off64_t>(h168::Size)+*offset;
    else return -1;
    if(position<0 || position>static_cast<off64_t>(h168::Size))return -1;
    s.position=static_cast<size_t>(position);
    *offset=position;
    return 0;
}
static FILE* faultyFile(FaultyCopy& data){
    cookie_io_functions_t callbacks{};
    callbacks.read=&faultyRead;
    callbacks.seek=&faultySeek;
    FILE* file=fopencookie(&data,"rb",callbacks);
    if(file)std::setvbuf(file,nullptr,_IONBF,0);
    return file;
}
#endif
int main(){
    const auto bytes=fixture(); // generated rotated dual-bank H168 synthetic
    assert(bytes.size()==h168::Size);
    static Demo live{};
    populateMock(live);
    std::string reason;
    assert(inspectCopiedBuffer(bytes.data(),bytes.size(),live,reason));
    assert(!live.synthetic);
    assert(live.sourceModelApi==rogue18::ModelApiVersion);
    static_assert(Phase18ReadModelApiV1==rogue18::ModelApiVersion,
                  "Native decoder and reviewed Phase18 SaveView differ");
    const auto model=rogue18::inspectReadOnly(bytes.data(),bytes.size(),false);
    assert(model.valid && model.confidence==rogue18::DataConfidence::StructurallyValidated);
    assert(model.apiVersion==Phase18ReadModelApiV1);
    assert(model.sourceSha256==live.sourceSha256);
    assert(model.activeCounter==live.activeCounter && model.money==live.money);
    assert(model.party.size()==live.partyCount && model.items.size()==live.bagCount);
    for(unsigned i=0;i<live.partyCount;i++) {
        const auto& mon=model.party[i].value;
        assert(mon.species==live.party[i].species);
        assert(mon.abilitySlot==live.party[i].ability);
        assert(mon.effectiveNature==live.party[i].nature);
        for(unsigned j=0;j<6;j++) {
            assert(mon.ivs[j]==live.party[i].iv[j]);
            assert(mon.evs[j]==live.party[i].ev[j]);
        }
        for(unsigned j=0;j<4;j++) {
            assert(mon.moves[j]==live.party[i].moves[j]);
            assert(mon.pp[j]==live.party[i].pp[j]);
        }
    }
    for(unsigned i=0;i<live.bagCount;i++) {
        assert(model.items[i].itemId==live.bag[i].id);
        assert(model.items[i].quantity==live.bag[i].quantity);
    }
    assert(model.capabilities.copiedSaveOnly);
    assert(!model.capabilities.liveWrite && !model.capabilities.restore);
    assert(!model.capabilities.pcEdit && !model.capabilities.insertItem);
    assert(!model.capabilities.arbitraryMoveId && !model.capabilities.nature);

    assert(rogue18::hasDigest(live.sourceSha256));
    assert(live.sourceSha256==rogue18::sourceDigest(bytes.data(),bytes.size()));
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

    // Valid early-game save with no Pokémon in party: ACCEPT as EMPTY.
    auto partyEmpty=bytes;
    h168::Info originalInfo{};
    assert(rogue18::validate(bytes.data(),bytes.size(),originalInfo));
    for(unsigned bank=0;bank<2;bank++){
        const size_t sector=originalInfo.banks[bank].sector[1];
        h168::w32(partyEmpty.data(),sector+0x234,0);
        h168::w16(partyEmpty.data(),sector+0xff6,
                   h168::checksum(partyEmpty.data()+sector));
    }
    static Demo zeroParty{};
    assert(inspectCopiedBuffer(partyEmpty.data(),partyEmpty.size(),zeroParty,reason));
    assert(!zeroParty.synthetic && zeroParty.partyStatus==Availability::Empty);
    assert(zeroParty.partyCount==0 && zeroParty.bagCount==3);
    State emptyPartyUI(zeroParty);
    press(emptyPartyUI,Key::A);
    assert(emptyPartyUI.screen==Screen::Party && count(emptyPartyUI)==0);
    press(emptyPartyUI,Key::A);
    assert(emptyPartyUI.screen==Screen::Notice);
    press(emptyPartyUI,Key::B);
    assert(emptyPartyUI.screen==Screen::Party);

    // Valid empty bag: ACCEPT as EMPTY while retaining the actual party.
    auto bagEmpty=bytes;
    for(unsigned bank=0;bank<2;bank++){
        const size_t sector=originalInfo.banks[bank].sector[1];
        std::memset(bagEmpty.data()+sector+0x578,0,450*4);
        h168::w16(bagEmpty.data(),sector+0xff6,
                   h168::checksum(bagEmpty.data()+sector));
    }
    static Demo zeroBag{};
    assert(inspectCopiedBuffer(bagEmpty.data(),bagEmpty.size(),zeroBag,reason));
    assert(!zeroBag.synthetic && zeroBag.bagStatus==Availability::Empty);
    assert(zeroBag.bagCount==0 && zeroBag.partyCount==2);

    // Both empty must also pass; reject neither because of missing inventory.
    auto bothEmpty=partyEmpty;
    for(unsigned bank=0;bank<2;bank++){
        const size_t sector=originalInfo.banks[bank].sector[1];
        std::memset(bothEmpty.data()+sector+0x578,0,450*4);
        h168::w16(bothEmpty.data(),sector+0xff6,
                   h168::checksum(bothEmpty.data()+sector));
    }
    static Demo zeroBoth{};
    assert(inspectCopiedBuffer(bothEmpty.data(),bothEmpty.size(),zeroBoth,reason));
    assert(zeroBoth.partyStatus==Availability::Empty);
    assert(zeroBoth.bagStatus==Availability::Empty);
    assert(zeroBoth.money==40001);

    // Exact real FILE* reader: open generated disposable host-only file,
    // verify parser did not rewrite even one byte of the input stream.
    std::unique_ptr<FILE, int(*)(FILE*)> stream(std::tmpfile(),&std::fclose);
    assert(stream);
    assert(std::fwrite(bytes.data(),1,bytes.size(),stream.get())==bytes.size());
    assert(std::fflush(stream.get())==0);
    static Demo fromStream{};
    assert(inspectReadOnlyStream(stream.get(),fromStream,reason));
    assert(fromStream.money==40001 && fromStream.bag[0].id==113);
    assert(std::fseek(stream.get(),0,SEEK_SET)==0);
    std::vector<uint8_t> stillOriginal(bytes.size());
    assert(std::fread(stillOriginal.data(),1,stillOriginal.size(),stream.get())==bytes.size());
    assert(stillOriginal==bytes);

    // Stream reader must reject an exact-size corrupted copy and keep view.
    std::unique_ptr<FILE, int(*)(FILE*)> damagedStream(std::tmpfile(),&std::fclose);
    assert(damagedStream);
    assert(std::fwrite(damaged.data(),1,h168::Size,damagedStream.get())==h168::Size);
    assert(std::fflush(damagedStream.get())==0);
    // The earlier "damaged" buffer was resized to 263168, but its first
    // 128KiB still contain the deliberately modified sector checksum.
    assert(!inspectReadOnlyStream(damagedStream.get(),fromStream,reason));
    assert(fromStream.money==40001 && fromStream.party[0].species==25);
    std::unique_ptr<FILE, int(*)(FILE*)> wrappedStream(std::tmpfile(),&std::fclose);
    assert(wrappedStream);
    assert(std::fwrite(damaged.data(),1,damaged.size(),wrappedStream.get())==damaged.size());
    assert(std::fflush(wrappedStream.get())==0);
    assert(!inspectReadOnlyStream(wrappedStream.get(),fromStream,reason));
    assert(reason.find("263168")!=std::string::npos);
    assert(fromStream.money==40001 && fromStream.party[0].species==25);

    // All 28 journalled sector payload checksums must fail closed if changed.
    // The final 4 physical 4-KiB sectors are not covered by this format's
    // two historical save banks; do not claim full-file authentication.
    for(unsigned sector=0;sector<28;sector++){
        auto singleFlip=bytes;
        singleFlip[sector*h168::Sector+0x350]^=0x01;
        assert(!inspectCopiedBuffer(singleFlip.data(),singleFlip.size(),fromStream,reason));
        assert(fromStream.money==40001 && fromStream.party[0].species==25);
    }

    // Phase18 current parser must accept modulo-u32 rollover adjacency.
    auto rollover=bytes;
    for(unsigned bank=0;bank<2;bank++)
      for(unsigned physical=0;physical<14;physical++){
        const auto sector=(bank*14u+physical)*h168::Sector;
        h168::w32(rollover.data(),sector+0xffc,
                  bank==0?0xffffffffu:0u);
      }
    static Demo rolls{};
    assert(inspectCopiedBuffer(rollover.data(),rollover.size(),rolls,reason));
    assert(rolls.money==40001 && rolls.activeCounter==0u);
    assert(rolls.sourceSha256!=live.sourceSha256);
    // A stale/nonadjacent source copy is never silently selected.
    auto gapped=bytes;
    for(unsigned i=0;i<14;i++){
        const auto off=(14u+i)*h168::Sector;
        h168::w32(gapped.data(),off+0xffc,60u);
    }
    assert(!inspectCopiedBuffer(gapped.data(),gapped.size(),rolls,reason));
    assert(rolls.activeCounter==0u &&
           rolls.sourceSha256==rogue18::sourceDigest(rollover.data(),rollover.size()));

    // Recompute sector checksums to probe semantic field checks beyond
    // simple checksum rejection; any invalid Pokémon remains blocked.
    const size_t activeS1=originalInfo.banks[originalInfo.active].sector[1];
    const size_t monAt=activeS1+0x238u;
    auto badAbility=bytes;
    auto canonical=rogue18::plaintext(badAbility.data(),monAt);
    uint32_t misc=h168::r32(canonical.data(),44u);
    misc=(misc&~(uint32_t(3)<<29))|(uint32_t(3)<<29);
    h168::w32(canonical.data(),44u,misc);
    rogue18::encode(badAbility.data(),monAt,canonical);
    h168::w16(badAbility.data(),activeS1+0xff6,
               h168::checksum(badAbility.data()+activeS1));
    assert(!inspectCopiedBuffer(badAbility.data(),badAbility.size(),rolls,reason));
    assert(rolls.money==40001 && rolls.activeCounter==0u);

    auto badEV=bytes;
    canonical=rogue18::plaintext(badEV.data(),monAt);
    canonical[24]=253;
    rogue18::encode(badEV.data(),monAt,canonical);
    h168::w16(badEV.data(),activeS1+0xff6,
               h168::checksum(badEV.data()+activeS1));
    assert(!inspectCopiedBuffer(badEV.data(),badEV.size(),rolls,reason));

    auto badPP=bytes;
    canonical=rogue18::plaintext(badPP.data(),monAt);
    canonical[20]=65; // parser may decode, but UI enforces PP<=64
    rogue18::encode(badPP.data(),monAt,canonical);
    h168::w16(badPP.data(),activeS1+0xff6,
               h168::checksum(badPP.data()+activeS1));
    assert(!inspectCopiedBuffer(badPP.data(),badPP.size(),rolls,reason));
    assert(rolls.sourceSha256==rogue18::sourceDigest(rollover.data(),rollover.size()));

    assert(!inspectCopiedBuffer(nullptr,h168::Size,rolls,reason));
    assert(!inspectCopiedBuffer(bytes.data(),0,rolls,reason));
    assert(!inspectCopiedBuffer(bytes.data(),h168::Size-1,rolls,reason));
    assert(rolls.activeCounter==0);
#if defined(__linux__)
    FaultyCopy premature{bytes.data(),h168::Size/2u,0u,false};
    std::unique_ptr<FILE, int(*)(FILE*)> prematureFile(faultyFile(premature),&std::fclose);
    assert(prematureFile);
    assert(!inspectReadOnlyStream(prematureFile.get(),rolls,reason));
    assert(reason.find("full")!=std::string::npos);
    assert(rolls.activeCounter==0u);

    FaultyCopy diskError{bytes.data(),4096u,0u,true};
    std::unique_ptr<FILE, int(*)(FILE*)> errorFile(faultyFile(diskError),&std::fclose);
    assert(errorFile);
    assert(!inspectReadOnlyStream(errorFile.get(),rolls,reason));
    assert(reason.find("full")!=std::string::npos);
    assert(rolls.sourceSha256==rogue18::sourceDigest(rollover.data(),rollover.size()));
#endif

    std::cout<<"PASS Phase19: pinned Phase18 H168 copied-data read only bridge, no writes\n";
}
