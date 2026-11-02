// Actual production header test: NO synthetic macro and no ROM/save files.
// The fixture mutator must not be declared by the release build.
#if defined(ROGUE18_TEST_MOVE_V2) || defined(ROGUE18_SYNTHETIC_MOVE_TEST)
#error "Release-lock test must not compile with synthetic editor symbols"
#endif
#include "move_queue_v4.hpp"
#include <iostream>
#include <type_traits>
using namespace rogue18;
int main(){
    static_assert(!move_v2::ApplyApproved,"Release Apply unexpectedly unlocked");
    static_assert(move_v2::ContractVersion==2,"Unreviewed move contract version");
    static_assert(move_queue_v4::ContractVersion==4,"Unreviewed queue ABI");
    static_assert(move_v2::releaseBlockers.size()==6,"Missing release blocker");
    move_v2::Catalog untrusted{};
    move_v2::Request req{};
    if(move_v2::authenticate(untrusted))return 1;
    if(move_v2::choose(untrusted,req,53))return 2;
    if(move_v2::preview(nullptr,0,untrusted,req).ok)return 3;
    if(move_v2::verify(nullptr,0,nullptr,0,untrusted,req))return 4;
    if(move_v2::mayRestoreInstalledGame(req))return 5;
    move_queue_v4::Queue queue{};
    queue.replacements.push_back(req);
    if(move_queue_v4::preview(nullptr,0,queue).ok)return 6;
    if(move_queue_v4::mayWriteInstalledGame(queue))return 7;
    if(move_queue_v4::verifyStaging(nullptr,0,untrusted,queue))return 8;
    std::cout<<"PASS production headers: Apply/verify/export rejected without synthetic macro\n";
}
