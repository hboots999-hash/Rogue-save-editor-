// Offline file-stage tests only: generated non-playable synthetic fixture.
#include "offline_export.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
namespace fs=std::filesystem;
using namespace rogue18;
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n";return 1;}}while(0)
static Bytes read(const fs::path& path){
    std::ifstream f(path,std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(f),{});
}
static void put(const fs::path& p,const Bytes& b){
    std::ofstream out(p,std::ios::binary|std::ios::trunc);
    out.write(reinterpret_cast<const char*>(b.data()),b.size());
    out.close();
    assert(out.good());
}
int main(int argc,char** argv){
    CHECK(argc==2);
    Bytes original=read(argv[1]);
    CHECK(original.size()==h168::Size);
    h168::Info info{};
    CHECK(validate(original.data(),original.size(),info));
    const fs::path root=fs::temp_directory_path()/"phase18_offline_stage_test";
    fs::remove_all(root);
    fs::create_directories(root/"imports");
    const auto imp=root/"imports/00000001.sav";
    put(imp,original);
    Request q{};q.kind=Kind::Money;
    q.expectedCounter=info.banks[info.active].counter;
    q.expectedMoney=info.money;q.money=60000;
    q.expectedDigest=sourceDigest(original.data(),original.size());
    auto output=exportCopied(original.data(),original.size(),
                             imp.c_str(),root.c_str(),q);
    CHECK(output.ok && output.changed>0);
    CHECK(output.backupName=="phase18_original_001.sav");
    CHECK(output.draftName=="phase18_draft_001.sav");
    CHECK(read(imp)==original);
    CHECK(read(root/"backups"/output.backupName)==original);
    auto draft=read(root/"drafts"/output.draftName);
    CHECK(verifyExact(original.data(),original.size(),
                      draft.data(),draft.size(),q));
    CHECK(validate(draft.data(),draft.size(),info)&&info.money==60000);
    // Each export creates NEW immutable backup/draft, never overwrites.
    output=exportCopied(original.data(),original.size(),imp.c_str(),root.c_str(),q);
    CHECK(output.ok && output.backupName=="phase18_original_002.sav");
    CHECK(output.draftName=="phase18_draft_002.sav");
    CHECK(read(root/"backups/phase18_original_001.sav")==original);
    CHECK(read(root/"drafts/phase18_draft_001.sav")==draft);
    CHECK(read(imp)==original);
    // No arbitrary export destination / permission to modify a game save.
    CHECK(!exportCopied(original.data(),original.size(),
                        (root/"backups/phase18_original_001.sav").c_str(),
                        root.c_str(),q).ok);
    CHECK(!exportCopied(original.data(),original.size(),
                        imp.c_str(),(root/"../phase18_offline_stage_test").c_str(),q).ok);
    // The imported bytes must match the captured expected original EXACTLY.
    auto stale=original;stale.back()^=1;
    CHECK(!exportCopied(stale.data(),stale.size(),imp.c_str(),root.c_str(),q).ok);
    stale=original;stale[info.banks[info.active].sector[1]+0x4a8]^=1;
    CHECK(!exportCopied(stale.data(),stale.size(),imp.c_str(),root.c_str(),q).ok);
    CHECK(!fs::exists(root/"drafts/phase18_draft_003.sav"));
    // Existing partial backup names are preserved; no replacement.
    put(root/"backups/phase18_original_003.sav",Bytes{1,2,3});
    output=exportCopied(original.data(),original.size(),imp.c_str(),root.c_str(),q);
    CHECK(output.ok && output.backupName=="phase18_original_004.sav");
    CHECK(read(root/"backups/phase18_original_003.sav")==Bytes({1,2,3}));
    // No-op, mismatched counter, unverified features must create no files.
    q.money=q.expectedMoney;
    CHECK(!exportCopied(original.data(),original.size(),imp.c_str(),root.c_str(),q).ok);
    q.kind=Kind::Nature;
    CHECK(!exportCopied(original.data(),original.size(),imp.c_str(),root.c_str(),q).ok);
    CHECK(!fs::exists(root/"drafts/phase18_draft_004.sav"));
    CHECK(read(imp)==original);
    fs::remove_all(root);
    std::cout<<"PASS Phase18 SD import: untouched source, immutable backups, fresh readback, exact draft, stale/unsupported/path rejection\n";
}
