// Phase 18 deterministic non-playable regression suite, no personal saves.
#include "offline_engine.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
using namespace rogue18;
#define CHECK(x) do { if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n"; return 1;} }while(0)

static Bytes fixture(){
    Bytes b(h168::Size,0);
    for(unsigned bank=0;bank<2;bank++){
        // Rotate physical sector positions; section 0 is NOT at offset 0.
        const unsigned rotation=bank?5u:3u;
        const uint32_t key=bank?0xf00dbabeu:0xa5a5c33cu;
        const unsigned s0=(bank*14+(0+rotation)%14)*h168::Sector;
        const unsigned s1=(bank*14+(1+rotation)%14)*h168::Sector;
        h168::w32(b.data(),s0+0x4c,key);
        h168::w32(b.data(),s1+0x234,2);
        h168::w32(b.data(),s1+0x4a8,(40000+bank)^key);
        for(unsigned i=0;i<2;i++){
            const size_t base=s1+0x238+i*104;
            const uint32_t pid=i?17u:1u; // EGM A / GAME permutations
            const uint32_t ot=i?0xdeadbeefu:0x13572468u;
            h168::w32(b.data(),base,pid);
            h168::w32(b.data(),base+4,ot);
            b[base+84]=50;
            b[base+100]=0xa7; // preserve 104-byte extension
            b[base+103]=0x19;
            std::array<uint8_t,48> p{};
            h168::w16(p.data(),0,uint16_t(i?676:25)); // expanded species allowed
            // 11-bit species, a held item occupying 10+1 separate bits.
            uint32_t growth0=(i?676u:25u)|(uint32_t(99)<<11);
            h168::w32(p.data(),0,growth0);
            h168::w16(p.data(),12,33);
            h168::w16(p.data(),14,45);
            p[20]=15;p[21]=25;
            p[24]=4;p[25]=252;p[27]=252;
            uint32_t packed=0;
            for(unsigned v=0;v<6;v++)packed|=uint32_t(10+v)<<(5*v);
            if(i)packed|=uint32_t(1)<<31; // Gigantamax bit, NOT ability
            h168::w32(p.data(),40,packed);
            if(i)h168::w32(p.data(),44,uint32_t(2)<<29); // hidden ability index
            encode(b.data(),base,p);
        }
        const uint16_t ids[]={113,1,5};
        const uint16_t quantities[]={10,2,12};
        for(unsigned j=0;j<3;j++){
            const unsigned off=s1+0x578+4*j;
            h168::w16(b.data(),off,ids[j]);
            h168::w16(b.data(),off+2,quantities[j]^uint16_t(key));
        }
        // Finalize each footer AFTER the encrypted party, bag and money.
        for(unsigned p=0;p<14;p++){
            const unsigned physical=bank*14+p;
            uint8_t* sector=b.data()+physical*h168::Sector;
            const unsigned section=(p+14-rotation)%14;
            h168::w16(sector,0xff4,uint16_t(section));
            h168::w16(sector,0xff6,h168::checksum(sector));
            h168::w32(sector,0xff8,0x08012025);
            h168::w32(sector,0xffc,40+bank);
        }
    }
    return b;
}
static Request monRequest(const Bytes& b,unsigned idx,Kind kind){
    h168::Info info{};
    assert(validate(b.data(),b.size(),info));
    Mon mon{};
    assert(partyAt(b.data(),b.size(),info,idx,mon));
    Request q{};
    q.kind=kind;q.slot=idx;q.expectedCounter=info.banks[info.active].counter;
    q.expectedPid=mon.pid;q.expectedOt=mon.ot;
    q.expectedSpecies=mon.species;q.expectedPokemonChecksum=mon.checksum;
    return q;
}
static bool sameInactive(const Bytes& original,const Bytes& edited){
    // Fixture: bank 0 is the older bank; trailing 16KiB untouched as well.
    return std::memcmp(original.data(),edited.data(),14*h168::Sector)==0 &&
           std::memcmp(original.data()+28*h168::Sector,
                       edited.data()+28*h168::Sector,
                       h168::Size-28*h168::Sector)==0;
}
int main(int argc,char** argv){
    const Bytes b=fixture();
    h168::Info info{};
    CHECK(validate(b.data(),b.size(),info));
    CHECK(info.active==1 && info.money==40001 && info.items==3);
    CHECK(info.partyCount==2 && info.banks[1].sector[0]%h168::Sector==0);
    Mon mon0{},mon1{};
    CHECK(partyAt(b.data(),b.size(),info,0,mon0));
    CHECK(partyAt(b.data(),b.size(),info,1,mon1));
    CHECK(mon0.species==25 && mon1.species==676 && mon0.pid%24!=mon1.pid%24);
    CHECK(mon0.ivs[0]==10 && mon0.ivs[5]==15 && mon1.moves[1]==45);
    CHECK(mon1.natureFromPid==17);
    CHECK(mon0.heldItem==99 && mon1.heldItem==99);
    CHECK(mon1.abilitySlot==2 && mon1.gigantamaxFactor==1);
    // Every one of the 24 Gen III substructure orders must preserve the
    // same canonical fields, 104-byte extension, and checksums after editing.
    for(unsigned permutation=0;permutation<24;permutation++){
        Bytes varied=b;
        const size_t start=info.banks[info.active].sector[1]+0x238;
        const auto clean=plaintext(varied.data(),start);
        h168::w32(varied.data(),start,permutation);
        encode(varied.data(),start,clean);
        const size_t sec=info.banks[info.active].sector[1];
        h168::w16(varied.data(),sec+0xff6,h168::checksum(varied.data()+sec));
        h168::Info checked{};
        CHECK(validate(varied.data(),varied.size(),checked));
        Mon decoded{};
        CHECK(partyAt(varied.data(),varied.size(),checked,0,decoded));
        CHECK(decoded.pid==permutation && decoded.species==25);
        CHECK(decoded.moves[0]==33 && decoded.ivs[0]==10 && decoded.evs[1]==252);
        Request req=monRequest(varied,0,Kind::IVs);
        req.stats={1,2,3,4,5,6};
        auto p=propose(varied.data(),varied.size(),req);
        CHECK(p.ok && sameInactive(varied,p.candidate));
        CHECK(verifyExact(varied.data(),varied.size(),p.candidate.data(),
                          p.candidate.size(),req));
        CHECK(p.candidate[start+100]==0xa7 && p.candidate[start+103]==0x19);
    }

    Request q{};
    q.kind=Kind::Money;q.expectedCounter=41;
    q.expectedMoney=40001;q.money=999999;
    auto money=propose(b.data(),b.size(),q);
    CHECK(money.ok && money.changedOffsets.size()<=6 && sameInactive(b,money.candidate));
    CHECK(verifyExact(b.data(),b.size(),money.candidate.data(),money.candidate.size(),q));
    h168::Info after{};
    CHECK(validate(money.candidate.data(),money.candidate.size(),after));
    CHECK(after.money==999999);
    Request back=q;back.expectedMoney=999999;back.money=40001;
    auto undone=propose(money.candidate.data(),h168::Size,back);
    CHECK(undone.ok && undone.candidate==b);
    q.expectedCounter=40;
    CHECK(!propose(b.data(),b.size(),q).ok);
    q.expectedCounter=41;
    q.money=40001;
    CHECK(!propose(b.data(),b.size(),q).ok);

    q={};q.kind=Kind::ExistingQuantity;q.expectedCounter=41;
    q.slot=0;q.expectedItemId=113;q.expectedQuantity=10;q.quantity=999;
    auto stack=propose(b.data(),b.size(),q);
    CHECK(stack.ok && stack.changedOffsets.size()<=4 && sameInactive(b,stack.candidate));
    CHECK(validate(stack.candidate.data(),stack.candidate.size(),after));
    unsigned id=0,qty=0;
    CHECK(h168::item(stack.candidate.data(),after,0,id,qty) && id==113 && qty==999);
    back=q;back.expectedQuantity=999;back.quantity=10;
    undone=propose(stack.candidate.data(),stack.candidate.size(),back);
    CHECK(undone.ok && undone.candidate==b);
    q.expectedItemId=999;CHECK(!propose(b.data(),b.size(),q).ok);
    q.expectedItemId=113;q.quantity=0;CHECK(!propose(b.data(),b.size(),q).ok);
    q.quantity=1000;CHECK(!propose(b.data(),b.size(),q).ok);

    q=monRequest(b,0,Kind::IVs);q.stats={31,30,29,28,27,26};
    auto iv=propose(b.data(),b.size(),q);
    CHECK(iv.ok && sameInactive(b,iv.candidate));
    CHECK(verifyExact(b.data(),b.size(),iv.candidate.data(),iv.candidate.size(),q));
    CHECK(validate(iv.candidate.data(),iv.candidate.size(),after));
    Mon changed{};
    CHECK(partyAt(iv.candidate.data(),iv.candidate.size(),after,0,changed));
    CHECK(changed.ivs==q.stats && changed.evs==mon0.evs);
    CHECK(changed.pid==mon0.pid && changed.abilitySlot==mon0.abilitySlot);
    CHECK(changed.checksum!=mon0.checksum);
    back=monRequest(iv.candidate,0,Kind::IVs);back.stats=mon0.ivs;
    undone=propose(iv.candidate.data(),iv.candidate.size(),back);
    CHECK(undone.ok && undone.candidate==b);
    q.stats[0]=32;CHECK(!propose(b.data(),b.size(),q).ok);
    q.stats[0]=31;q.expectedPokemonChecksum++;
    CHECK(!propose(b.data(),b.size(),q).ok);

    q=monRequest(b,1,Kind::EVs);q.stats={0,0,0,0,252,252};
    auto ev=propose(b.data(),b.size(),q);
    CHECK(ev.ok && sameInactive(b,ev.candidate));
    CHECK(validate(ev.candidate.data(),ev.candidate.size(),after));
    CHECK(partyAt(ev.candidate.data(),ev.candidate.size(),after,1,changed));
    CHECK(changed.evs==q.stats && changed.ivs==mon1.ivs);
    back=monRequest(ev.candidate,1,Kind::EVs);back.stats=mon1.evs;
    undone=propose(ev.candidate.data(),ev.candidate.size(),back);
    CHECK(undone.ok && undone.candidate==b);
    q.stats={252,252,252,0,0,0};CHECK(!propose(b.data(),b.size(),q).ok);
    q.stats={253,0,0,0,0,0};CHECK(!propose(b.data(),b.size(),q).ok);

    q=monRequest(b,1,Kind::Move);
    q.moveSlot=0;q.expectedMoveId=33;q.expectedPP=15;
    q.moveId=85;q.pp=12;
    CHECK(!propose(b.data(),b.size(),q).ok); // arbitrary Rogue move ID unsafe
    Evidence moves{};moves.verifiedMoveIds={85};
    auto move=propose(b.data(),b.size(),q,moves);
    CHECK(move.ok && sameInactive(b,move.candidate));
    CHECK(validate(move.candidate.data(),move.candidate.size(),after));
    CHECK(partyAt(move.candidate.data(),move.candidate.size(),after,1,changed));
    CHECK(changed.moves[0]==85 && changed.pp[0]==12);
    CHECK(changed.moves[1]==45 && changed.pp[1]==25);
    CHECK(verifyExact(b.data(),b.size(),move.candidate.data(),move.candidate.size(),q,moves));
    back=monRequest(move.candidate,1,Kind::Move);
    back.moveSlot=0;back.expectedMoveId=85;back.expectedPP=12;
    back.moveId=33;back.pp=15;moves.verifiedMoveIds.push_back(33);
    undone=propose(move.candidate.data(),move.candidate.size(),back,moves);
    CHECK(undone.ok && undone.candidate==b);
    q.pp=65;CHECK(!propose(b.data(),b.size(),q,moves).ok);
    q.pp=12;q.expectedMoveId=100;
    CHECK(!propose(b.data(),b.size(),q,moves).ok);
    // Same move, PP-only edit requires no guessed ID allowlist.
    q=monRequest(b,0,Kind::Move);
    q.moveSlot=1;q.expectedMoveId=45;q.expectedPP=25;q.moveId=45;q.pp=24;
    CHECK(propose(b.data(),b.size(),q).ok);

    q=monRequest(b,0,Kind::AbilitySlot);q.abilitySlot=1;
    CHECK(!propose(b.data(),b.size(),q).ok);
    Evidence ability{};ability.verifiedAbilitySlot=true;
    auto ab=propose(b.data(),b.size(),q,ability);
    CHECK(ab.ok && sameInactive(b,ab.candidate));
    CHECK(validate(ab.candidate.data(),ab.candidate.size(),after));
    CHECK(partyAt(ab.candidate.data(),ab.candidate.size(),after,0,changed));
    CHECK(changed.abilitySlot==1 && changed.ivs==mon0.ivs);
    back=monRequest(ab.candidate,0,Kind::AbilitySlot);back.abilitySlot=0;
    undone=propose(ab.candidate.data(),ab.candidate.size(),back,ability);
    CHECK(undone.ok && undone.candidate==b);
    q.abilitySlot=3;CHECK(!propose(b.data(),b.size(),q,ability).ok);
    // Ability slot mutation must never alter Gigantamax flag on mon1.
    q=monRequest(b,1,Kind::AbilitySlot);q.abilitySlot=1;
    ab=propose(b.data(),b.size(),q,ability);
    CHECK(ab.ok);
    CHECK(validate(ab.candidate.data(),ab.candidate.size(),after));
    CHECK(partyAt(ab.candidate.data(),ab.candidate.size(),after,1,changed));
    CHECK(changed.abilitySlot==1 && changed.gigantamaxFactor==1);
    CHECK(changed.heldItem==99);
    q=monRequest(b,0,Kind::Nature);q.expectedNature=mon0.effectiveNature;
    q.nature=20;
    CHECK(!propose(b.data(),b.size(),q,ability).ok);
    ability.verifiedNatureModifier=true;
    auto nature=propose(b.data(),b.size(),q,ability);
    CHECK(nature.ok && sameInactive(b,nature.candidate));
    CHECK(validate(nature.candidate.data(),nature.candidate.size(),after));
    CHECK(partyAt(nature.candidate.data(),nature.candidate.size(),after,0,changed));
    CHECK(changed.effectiveNature==20 && changed.natureFromPid==mon0.natureFromPid);
    CHECK(changed.pid==mon0.pid && changed.ot==mon0.ot);
    CHECK(changed.ivs==mon0.ivs && changed.abilitySlot==mon0.abilitySlot);
    CHECK(changed.checksum==mon0.checksum); // header-only nature edit
    back=monRequest(nature.candidate,0,Kind::Nature);
    back.expectedNature=20;back.nature=mon0.effectiveNature;
    undone=propose(nature.candidate.data(),nature.candidate.size(),back,ability);
    CHECK(undone.ok && undone.candidate==b);
    q.expectedNature=99;CHECK(!propose(b.data(),b.size(),q,ability).ok);
    q.expectedNature=mon0.effectiveNature;q.nature=25;
    CHECK(!propose(b.data(),b.size(),q,ability).ok);

    q={};q.kind=Kind::InsertItem;q.expectedCounter=41;
    q.slot=3;q.itemId=99;q.quantity=500;
    CHECK(!propose(b.data(),b.size(),q).ok);
    Evidence insertion{};insertion.verifiedBagInsertion=true;
    insertion.verifiedInsertItemIds={99};
    // EX v2.1 geometry: start at 80 slots, 50 reserved for key items.
    CHECK(capacityFromUpgrades(0)==80 && normalCapacityFromUpgrades(0)==30);
    CHECK(capacityFromUpgrades(37)==450 && normalCapacityFromUpgrades(37)==400);
    CHECK(capacityFromUpgrades(200)==450);
    insertion.verifiedUpgradeCount=0;
    insertion.verifiedItemPockets={{113,1},{1,3},{5,7},{99,9}};
    CHECK(insertionCompatible(insertion.verifiedItemPockets,
                              std::vector<uint16_t>{113,1,5},99,0));
    CHECK(!insertionCompatible(insertion.verifiedItemPockets,
                               std::vector<uint16_t>{5,113,1},99,0));

    auto inserted=propose(b.data(),b.size(),q,insertion);
    CHECK(inserted.ok && sameInactive(b,inserted.candidate));
    CHECK(validate(inserted.candidate.data(),inserted.candidate.size(),after));
    CHECK(after.items==4);
    CHECK(h168::item(inserted.candidate.data(),after,3,id,qty) && id==99 && qty==500);
    CHECK(verifyExact(b.data(),b.size(),inserted.candidate.data(),
                      inserted.candidate.size(),q,insertion));
    q.itemId=113;CHECK(!propose(b.data(),b.size(),q,insertion).ok);
    insertion.verifiedInsertItemIds.push_back(113);
    CHECK(!propose(b.data(),b.size(),q,insertion).ok); // duplicate
    q.itemId=99;q.slot=4;CHECK(!propose(b.data(),b.size(),q,insertion).ok);
    q.slot=3;
    insertion.verifiedUpgradeCount=255;
    CHECK(!propose(b.data(),b.size(),q,insertion).ok);
    insertion.verifiedUpgradeCount=0;
    insertion.verifiedItemPockets={{113,1},{1,3},{5,9},{99,1}};
    CHECK(!propose(b.data(),b.size(),q,insertion).ok);


    const unsigned s1=info.banks[1].sector[1];
    auto corrupt=b;
    // Flip one protected byte in every physical sector of both banks.
    // All 28 must be rejected despite the active party being otherwise valid.
    for(unsigned physical=0;physical<28;physical++){
        auto invalid=b;
        const size_t off=physical*h168::Sector+64;
        invalid[off]^=0x5a;
        h168::Info check{};
        CHECK(!validate(invalid.data(),invalid.size(),check));
    }
    // An attacker can repair section checksums but not silently change the
    // Pokémon ciphertext without also invalidating its 16-bit checksum.
    for(unsigned partySlot=0;partySlot<2;partySlot++){
        auto cipher=b;
        const size_t mon=s1+0x238+104*partySlot;
        cipher[mon+32+partySlot*8]^=1;
        h168::w16(cipher.data(),s1+0xff6,h168::checksum(cipher.data()+s1));
        h168::Info check{};
        CHECK(!validate(cipher.data(),cipher.size(),check));
    }
    // Different XOR key but a repaired section 0 alone invalidates bag/money.
    {
        auto wrongKey=b;
        const size_t sec0=info.banks[info.active].sector[0];
        wrongKey[sec0+0x4c+3]^=0x80; // force decoded money out of range
        h168::w16(wrongKey.data(),sec0+0xff6,
                   h168::checksum(wrongKey.data()+sec0));
        h168::Info check{};
        CHECK(!validate(wrongKey.data(),wrongKey.size(),check));
    }
    corrupt[s1+0x238+32]^=0xff; // mon payload, sector + mon checksum broken
    CHECK(!propose(corrupt.data(),corrupt.size(),Request{},moves).ok);
    corrupt=b;corrupt[info.banks[0].sector[1]+0x4a8]^=0x80;
    CHECK(!validate(corrupt.data(),corrupt.size(),after));
    corrupt=b;h168::w32(corrupt.data(),info.banks[1].sector[0]+0xffc,999);
    CHECK(!validate(corrupt.data(),corrupt.size(),after));
    CHECK(!validate(b.data(),h168::Size-1,after));
    // Tamper with a valid draft and recompute its sector checksum: STILL
    // fail the exact-difference/whole-buffer proof.
    corrupt=money.candidate;
    corrupt[s1+0x300]^=1;
    h168::w16(corrupt.data(),s1+0xff6,h168::checksum(corrupt.data()+s1));
    Request correct{};correct.kind=Kind::Money;correct.expectedCounter=41;
    correct.expectedMoney=40001;correct.money=999999;
    CHECK(validate(corrupt.data(),corrupt.size(),after));
    CHECK(!verifyExact(b.data(),b.size(),corrupt.data(),corrupt.size(),correct));
    CHECK(b==fixture()); // input never mutated
    if(argc>1){
        std::ofstream out(argv[1],std::ios::binary|std::ios::trunc);
        CHECK(out.good());
        out.write(reinterpret_cast<const char*>(b.data()),b.size());
        CHECK(out.good());
    }
    std::cout<<"PASS Phase18: rotated H168 banks / 104-byte party / exact diff / checksums / IV / EV / move+PP / money / bag / guarded insertion+ability / rejected nature / corruption / rollback\n";
}
