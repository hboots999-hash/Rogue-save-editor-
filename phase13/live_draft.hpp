/* Phase 13: direct LIVE read -> unchanged SD backup -> bounded edited SD
 * candidate; no privileged NAND or installed-game writes. GPL-3.0-or-later.
 */
#pragma once
#include "live_backup.hpp"
#include "restore_preflight.hpp"
#include <cstring>
#include <string>
namespace rogue_restore {
struct Receipt {
    bool ok=false;
    std::string message;
    std::string backupName,draftName;
    Validation editProof{};
    Receipt(bool v,std::string msg,std::string b={},std::string d={},Validation p={})
        : ok(v),message(std::move(msg)),backupName(std::move(b)),draftName(std::move(d)),editProof(std::move(p)) {}

};
template<typename Reader>
Receipt stageFromLive(Reader&& read,const char* root,
                      Operation operation,unsigned itemIndex=0){
    if(!root)return {false,"No root configured"};
    // backupVerified performs three fresh reads and confirms bytes across
    // read-only live snapshots before and after writing the backup on SD.
    const auto backup=rogue_live_backup::backupVerified(read,root);
    if(!backup.ok)return {false,"LIVE backup: "+backup.message,backup.name};
    const std::string backupPath=std::string(root)+"/backups/"+backup.name;
    auto saved=h168::readExact(backupPath.c_str());
    if(!saved)return {false,"Missing verified live backup",backup.name};
    auto latest=read();
    if(!latest.ok||!latest.save||
       std::memcmp(latest.save.get(),saved.get(),h168::Size)!=0)
        return {false,"Live source changed after backup: BLOCKED",backup.name};
    Validation proof{};
    auto draft=prepare(saved.get(),operation,itemIndex,proof);
    if(!draft||!proof.ok)
        return {false,"Cannot prepare: "+proof.reason,backup.name};
    if(!h168::directory(root))return {false,"SD root inaccessible",backup.name};
    const std::string folder=std::string(root)+"/drafts";
    if(!h168::directory(folder.c_str()))
        return {false,"SD drafts folder unavailable",backup.name};
    std::string name,error;
    if(!h168::uniqueWrite(folder,"liveedit_",draft.get(),name,error))
        return {false,"SD draft write: "+error,backup.name};
    const auto verified=h168::readExact((folder+"/"+name).c_str());
    if(!verified||
       std::memcmp(verified.get(),draft.get(),h168::Size)!=0||
       !validate(saved.get(),verified.get(),operation,itemIndex).ok)
        return {false,"SD draft readback FAILED",backup.name,name};
    auto end=read();
    if(!end.ok||!end.save||
       std::memcmp(end.save.get(),saved.get(),h168::Size)!=0)
        return {false,"Draft saved; live game changed since: DO NOT RESTORE",
                backup.name,name,proof};
    return {true,"SD DRAFT OK: "+name+" (game untouched)",backup.name,name,proof};
}
} // namespace rogue_restore
