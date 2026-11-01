// Phase18 v3 batch exporter storage faults, restart and no-overwrite proofs.
// Host-only injected I/O faults on generated NON-PLAYABLE saves.
#ifndef ROGUE18_TEST_IO_FAULTS
#error "Host fault tests must opt in to ROGUE18_TEST_IO_FAULTS"
#endif
#include "multi_export_v3.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
using namespace rogue18;
#define CHECK(x) do{if(!(x)){std::cerr<<"V3 SD FAILURE line "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
static Bytes readBytes(const fs::path& p){
    std::ifstream in(p,std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in),{});
}
static bool writeBytes(const fs::path& p,const Bytes& b){
    std::ofstream out(p,std::ios::binary|std::ios::trunc);
    out.write(reinterpret_cast<const char*>(b.data()),b.size());
    out.close();return out.good();
}
int main(int argc,char** argv){
    CHECK(argc==2);
    const Bytes original=readBytes(argv[1]);
    CHECK(original.size()==raw_v1::Size);
    const auto snap=v2::inspect(original.data(),original.size());
    CHECK(snap.data.valid);
    v3::Batch batch{};CHECK(v3::start(snap,batch));
    CHECK(v3::append(snap,batch,Kind::Money,0,0,123456));
    CHECK(v3::append(snap,batch,Kind::ExistingQuantity,0,0,999));
    CHECK(v3::append(snap,batch,Kind::IVs,0,0,31));
    CHECK(v3::append(snap,batch,Kind::Move,0,1,0));
    const auto prepared=v3::preview(original.data(),original.size(),batch);
    CHECK(prepared.ok && v3::verify(original.data(),original.size(),
           prepared.candidate.data(),prepared.candidate.size(),batch));
    const fs::path root=fs::temp_directory_path()/"pksm18_v3_sd_faults";
    const std::array<raw_v1::IoFault,6> faults={{
        raw_v1::IoFault::OpenNoSpace,raw_v1::IoFault::WriteNoSpace,
        raw_v1::IoFault::PartialThenNoSpace,raw_v1::IoFault::FlushFailure,
        raw_v1::IoFault::CloseFailure,raw_v1::IoFault::ReadbackFailure
    }};
    unsigned tested=0;
    for(const bool backupFailure:{true,false}){
        for(unsigned fault=0;fault<faults.size();fault++){
            fs::remove_all(root);
            fs::create_directories(root/"imports");
            const auto source=root/"imports"/"00000001.sav";
            CHECK(writeBytes(source,original));
            const std::string basename=backupFailure?
                "phase18_original_001.sav":"phase18_draft_001.sav";
            raw_v1::ioFault={faults[fault],basename,0};
            const auto failed=v3::exportCopiedBatch(original.data(),original.size(),
                                                     source.c_str(),root.c_str(),batch);
            raw_v1::ioFault={};
            CHECK(!failed.ok && readBytes(source)==original);
            const bool created=fault!=0;
            const auto uncertain=root/(backupFailure?"backups":"drafts")/basename;
            CHECK(fs::exists(uncertain)==created);
            if(backupFailure){
                CHECK(failed.draftName.empty());
                CHECK(failed.backupName==(created?basename:std::string{}));
                CHECK(!fs::exists(root/"drafts"));
            }else{
                CHECK(failed.backupName=="phase18_original_001.sav");
                CHECK(failed.draftName==(created?basename:std::string{}));
                CHECK(readBytes(root/"backups"/failed.backupName)==original);
            }
            if(created)CHECK(failed.reason.find(basename)!=std::string::npos);
            const auto retained=created?readBytes(uncertain):Bytes{};
            if(created && fault==1)CHECK(retained.empty());
            if(created && fault==2)CHECK(retained.size()==raw_v1::Size/2);
            if(created && fault>=3)CHECK(retained.size()==raw_v1::Size);
            // A new process with no persisted IoFault state must never
            // truncate an uncertain output; it allocates the next name.
            const auto retried=v3::exportCopiedBatch(original.data(),original.size(),
                                                     source.c_str(),root.c_str(),batch);
            CHECK(retried.ok && retried.changed==prepared.changedOffsets.size());
            CHECK(readBytes(source)==original);
            CHECK(readBytes(root/"backups"/retried.backupName)==original);
            CHECK(readBytes(root/"drafts"/retried.draftName)==prepared.candidate);
            if(created)CHECK(readBytes(uncertain)==retained);
            if(backupFailure && created)
                CHECK(retried.backupName=="phase18_original_002.sav");
            if(!backupFailure && created)
                CHECK(retried.draftName=="phase18_draft_002.sav");
            tested++;
        }
    }
    fs::remove_all(root);
    std::cout<<"PASS V3 SD batch: "<<tested<<" write/fault/restart scenarios,"
             <<" original untouched, failed output named, immutable backups and unique drafts\n";
}
