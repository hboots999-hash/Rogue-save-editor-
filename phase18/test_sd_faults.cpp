// SD-only transport failure/restart tests on NON-PLAYABLE synthetic copies.
// These test faults only exist when ROGUE18_TEST_IO_FAULTS is set on HOST.
#ifndef ROGUE18_TEST_IO_FAULTS
#error "Compile this test with -DROGUE18_TEST_IO_FAULTS"
#endif
#include "offline_export.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
namespace fs=std::filesystem;
using namespace rogue18;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAULT FAILURE "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
static Bytes readBytes(const fs::path& path){
    std::ifstream in(path,std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in),{});
}
static bool writeBytes(const fs::path& path,const Bytes& bytes){
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    out.close();return out.good();
}
static std::string nameFor(bool backup,unsigned n){
    char suffix[48]{};
    std::snprintf(suffix,sizeof(suffix),backup?"phase18_original_%03u.sav":
                                         "phase18_draft_%03u.sav",n);
    return suffix;
}
int main(int argc,char** argv){
    CHECK(argc==2);
    const auto original=readBytes(argv[1]);
    CHECK(original.size()==raw_v1::Size);
    const auto view=v2::inspect(original.data(),original.size());
    CHECK(view.data.valid);
    Request req{};
    CHECK(v2::begin(view,Kind::Money,0,req));
    req.money=123456;
    const auto prepared=v2::preview(original.data(),original.size(),req);
    CHECK(prepared.ok);
    const fs::path root=fs::temp_directory_path()/"pksm18_sd_faults";
    const std::array<raw_v1::IoFault,6> faults={{
        raw_v1::IoFault::OpenNoSpace,raw_v1::IoFault::WriteNoSpace,
        raw_v1::IoFault::PartialThenNoSpace,raw_v1::IoFault::FlushFailure,
        raw_v1::IoFault::CloseFailure,raw_v1::IoFault::ReadbackFailure
    }};
    unsigned failures=0;
    for(const bool faultBackup:{true,false}){
        for(unsigned phase=0;phase<faults.size();phase++){
            fs::remove_all(root);
            fs::create_directories(root/"imports");
            const auto imported=root/"imports/00000001.sav";
            CHECK(writeBytes(imported,original));
            raw_v1::ioFault={faults[phase],
                faultBackup?"phase18_original_001.sav":"phase18_draft_001.sav",0};
            const auto bad=exportCopied(original.data(),original.size(),
                                        imported.c_str(),root.c_str(),req);
            raw_v1::ioFault={};
            CHECK(!bad.ok && bad.draftName.empty());
            CHECK(readBytes(imported)==original);
            const auto broken=root/(faultBackup?"backups":"drafts")/
                              nameFor(faultBackup,1);
            const bool created=(phase!=0);
            CHECK(fs::exists(broken)==created);
            Bytes retained{};
            if(created){
                retained=readBytes(broken);
                if(phase==1)CHECK(retained.empty());
                if(phase==2)CHECK(retained.size()==raw_v1::Size/2);
                if(phase>=3)CHECK(retained.size()==raw_v1::Size);
            }
            if(!faultBackup){
                CHECK(readBytes(root/"backups"/nameFor(true,1))==original);
            }else CHECK(!fs::exists(root/"drafts"/nameFor(false,1)));
            // New session/process: no fault configuration, no deletion and
            // no re-use of an uncertain partial/completed SD output.
            const auto resumed=exportCopied(original.data(),original.size(),
                                            imported.c_str(),root.c_str(),req);
            CHECK(resumed.ok && resumed.changed==prepared.changedOffsets.size());
            CHECK(resumed.backupName==nameFor(true,faultBackup && created?2:(!faultBackup?2:1)));
            CHECK(resumed.draftName==nameFor(false,!faultBackup && created?2:1));
            CHECK(readBytes(root/"backups"/resumed.backupName)==original);
            CHECK(readBytes(root/"drafts"/resumed.draftName)==prepared.candidate);
            CHECK(readBytes(imported)==original);
            if(created)CHECK(readBytes(broken)==retained); // never overwrite
            failures++;
        }
    }
    // Missing SD import, short import, and inaccessible intermediate paths
    // must never create an approved draft.
    fs::remove_all(root);
    fs::create_directories(root/"imports");
    const auto imported=root/"imports/00000001.sav";
    CHECK(!exportCopied(original.data(),original.size(),
                        imported.c_str(),root.c_str(),req).ok);
    CHECK(writeBytes(imported,Bytes{1,2,3}));
    CHECK(!exportCopied(original.data(),original.size(),
                        imported.c_str(),root.c_str(),req).ok);
    CHECK(writeBytes(imported,original));
    fs::create_directories(root/"backups");
    for(unsigned i=1;i<=99;i++){
        const auto name=root/"backups"/nameFor(true,i);
        CHECK(writeBytes(name,Bytes{42}));
    }
    CHECK(!exportCopied(original.data(),original.size(),
                        imported.c_str(),root.c_str(),req).ok);
    CHECK(readBytes(root/"backups"/nameFor(true,99))==Bytes{42});
    CHECK(readBytes(imported)==original);
    fs::remove_all(root);
    std::cout<<"PASS Phase18 host SD fault test: "<<failures
             <<" backup/draft faults, storage full, partial, flush, close, readback,"
             <<" exact restart/unique-name preservation and exhausted 99 names\n";
}
