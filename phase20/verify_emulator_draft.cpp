// SPDX-License-Identifier: GPL-3.0-or-later
// Inspect disposable Azahar CI artifacts only. NEVER use real-game save input.
// This host-only tool independently decodes and checks one money +1 draft.
#include "phase18/integration_v2.hpp"
#include "phase18/multi_export_v3.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
using Bytes=std::vector<uint8_t>;
static Bytes readFile(const char* path) {
    std::ifstream f(path,std::ios::binary);
    if(!f)return {};
    return Bytes(std::istreambuf_iterator<char>(f),{});
}
int main(int argc,char** argv){
    if(argc!=4 && argc!=6) {
        std::cerr<<"usage: verifier original.sav backup.sav draft.sav [batch_backup.sav batch_draft.sav]\\n";
        return 2;
    }
    const auto original=readFile(argv[1]);
    const auto backup=readFile(argv[2]);
    const auto draft=readFile(argv[3]);
    if(original.size()!=rogue18::raw_v1::Size ||
       backup!=original || draft.size()!=original.size()) {
        std::cerr<<"Invalid or overwritten synthetic backup/draft\n";return 1;
    }
    const auto source=rogue18::v2::inspect(original.data(),original.size());
    const auto result=rogue18::v2::inspect(draft.data(),draft.size());
    if(!source.data.valid || !result.data.valid ||
       source.data.money!=40001u || result.data.money!=40002u ||
       source.data.party.size()!=result.data.party.size() ||
       source.data.items.size()!=result.data.items.size()) {
        std::cerr<<"Decoded synthetic money/party/bag validation failed\n";return 1;
    }
    rogue18::Request req{};
    if(!rogue18::v2::begin(source,rogue18::Kind::Money,0,req)){
        std::cerr<<"Could not begin source-locked v2 request\n";return 1;
    }
    req.money=40002;
    if(!rogue18::v2::verify(original.data(),original.size(),
                            draft.data(),draft.size(),req)) {
        std::cerr<<"Draft contains unexpected bytes or invalid checksums\n";
        return 1;
    }
    if(rogue18::v2::mayRestoreInstalledGame(req))return 1;
    if(argc==6){
        const auto v3backup=readFile(argv[4]);
        const auto v3draft=readFile(argv[5]);
        if(v3backup!=original || v3draft.size()!=original.size()) {
            std::cerr<<"Missing or modified v3 offline batch backup/draft\n";
            return 1;
        }
        rogue18::v3::Batch batch{};
        if(!rogue18::v3::start(source,batch) ||
           !rogue18::v3::append(source,batch,rogue18::Kind::Money,0,0,40002u) ||
           !rogue18::v3::append(source,batch,rogue18::Kind::ExistingQuantity,
                                  0,0,source.data.items[0].quantity+1u) ||
           !rogue18::v3::verify(original.data(),original.size(),
                               v3draft.data(),v3draft.size(),batch) ||
           rogue18::v3::mayWriteInstalledGame(batch)) {
            std::cerr<<"Native 3DS UI batch failed independent v3 whole-save proof\n";
            return 1;
        }
        const auto view=rogue18::v2::inspect(v3draft.data(),v3draft.size());
        if(!view.data.valid || view.data.money!=40002u ||
           view.data.items[0].quantity!=source.data.items[0].quantity+1u) {
            std::cerr<<"Native 3DS UI batch decoded values invalid\n";return 1;
        }
        std::cout<<"PASS Azahar multi-edit: money+1 and item stack+1 in ONE"
                    " 131072-byte SD draft, original/backup unchanged\n";
    }
    std::cout<<"PASS Azahar money +1: immutable original, exact backup, "
                "verified draft bytes, no installed-game write authority\n";
}
