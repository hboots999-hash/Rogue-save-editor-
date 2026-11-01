// V3 SD failure/restart regression. All files are nonplayable host fixtures.
#ifndef ROGUE18_TEST_IO_FAULTS
#error "Build v3 storage fault tests with host-only injection"
#endif
#include "multi_export_v3.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
using namespace rogue18;
#define CHECK(x) do{if(!(x)){std::cerr<<"V3 SD FAILURE "<<__LINE__<<" "<<#x<<"\n";return 1;}}while(0)
static Bytes read(const fs::path& path){
    std::ifstream f(path,std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(f),{});
}
static void write(const fs::path& path,const Bytes& b){
    std::ofstream f(path,std::ios::binary|std::ios::trunc);
    f.write(reinterpret_cast<const char*>(b.data()),b.size());
    f.close();
}
int main(int argc,char** argv){
    CHECK(argc==2);
    const auto bytes=read(argv[1]);
    CHECK(bytes.size()==raw_v1::Size);
    const auto view=v2::inspect(bytes.data(),bytes.size());
    CHECK(view.data.valid);
    v3::Batch batch{};
    CHECK(v3::start(view,batch));
    CHECK(v3::append(view,batch,Kind::Money,0,0,60000));
    CHECK(v3::append(view,batch,Kind::ExistingQuantity,0,0,40));
    CHECK(v3::append(view,batch,Kind::IVs,1,0,31));
    const auto expected=v3::preview(bytes.data(),bytes.size(),batch);
    CHECK(expected.ok);
    const fs::path root=fs::temp_directory_path()/"phase18_v3_faults";
    const std::array<raw_v1::IoFault,6> faults={{
        raw_v1::IoFault::OpenNoSpace,raw_v1::IoFault::WriteNoSpace,
        raw_v1::IoFault::PartialThenNoSpace,raw_v1::IoFault::FlushFailure,
        raw_v1::IoFault::CloseFailure,raw_v1::IoFault::ReadbackFailure
    }};
    unsigned scenarios=0;
    for(bool backupFault:{true,false}){
        for(unsigned i=0;i<faults.size();i++){
            fs::remove_all(root);
            fs::create_directories(root/"imports");
            const auto imp=root/"imports/00000001.sav";
            write(imp,bytes);
            const std::string target=backupFault?"phase18_original_001.sav":
                                                 "phase18_draft_001.sav";
            raw_v1::ioFault={faults[i],target,0};
            const auto bad=v3::exportCopiedBatch(bytes.data(),bytes.size(),
                                                  imp.c_str(),root.c_str(),batch);
            raw_v1::ioFault={};
            CHECK(!bad.ok);
            CHECK(read(imp)==bytes);
            const auto path=root/(backupFault?"backups":"drafts")/target;
            const bool created=i!=0;
            CHECK(fs::exists(path)==created);
            if(created){
                CHECK(bad.reason.find(target)!=std::string::npos);
                if(backupFault)CHECK(bad.backupName==target);
                else CHECK(bad.draftName==target);
            }
            if(backupFault)CHECK(!fs::exists(root/"drafts/phase18_draft_001.sav"));
            else CHECK(read(root/"backups/phase18_original_001.sav")==bytes);
            Bytes beforeFail{};
            if(created)beforeFail=read(path);
            if(i==1 && created)CHECK(beforeFail.empty());
            if(i==2 && created)CHECK(beforeFail.size()==raw_v1::Size/2);
            // A subsequent invocation must allocate a new unoccupied
            // basename; the interrupted output is never overwritten.
            const auto retry=v3::exportCopiedBatch(bytes.data(),bytes.size(),
                                                    imp.c_str(),root.c_str(),batch);
            CHECK(retry.ok);
            CHECK(read(root/"backups"/retry.backupName)==bytes);
            CHECK(read(root/"drafts"/retry.draftName)==expected.candidate);
            CHECK(read(imp)==bytes);
            if(created)CHECK(read(path)==beforeFail);
            if(backupFault && created)CHECK(retry.backupName!=target);
            if(!backupFault && created)CHECK(retry.draftName!=target);
            scenarios++;
        }
    }
    // Both replays must keep the earliest approved files intact.
    fs::remove_all(root);
    std::cout<<"PASS v3 SD faults "<<scenarios
             <<" failure/restart combinations, full and partial writes, source preservation\n";
}
