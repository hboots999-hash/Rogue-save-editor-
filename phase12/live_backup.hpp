/* Phase 12: no installed-title WRITES. Copy one current live save into a new,
 * independently verified SD-only backup. GPL-3.0-or-later.
 * The reader callback returns a fresh snapshot each time; no cached live
 * pointers or gameplay save archives are ever used as a write target.
 */
#pragma once
#include "offline.hpp"
#include <cstring>
#include <string>
namespace rogue_live_backup {
struct Receipt {
    bool ok=false;
    std::string message;
    std::string name;
};
template<typename Reader>
Receipt backupVerified(Reader&& read,const char* root){
    if(!root)return {false,"No backup root",{}};
    auto first=read();
    if(!first.ok || !first.save)
        return {false,"LIVE first read: "+first.stage,{}};
    h168::Info firstInfo{};
    if(!h168::inspect(first.save.get(),h168::Size,firstInfo))
        return {false,"LIVE first data invalid",{}};
    auto second=read();
    if(!second.ok || !second.save)
        return {false,"LIVE second read: "+second.stage,{}};
    h168::Info secondInfo{};
    if(!h168::inspect(second.save.get(),h168::Size,secondInfo) ||
       std::memcmp(second.save.get(),first.save.get(),h168::Size)!=0)
        return {false,"LIVE changed before backup: BLOCKED",{}};
    if(!h168::directory(root))return {false,"Backup root unavailable",{}};
    const std::string folder=std::string(root)+"/backups";
    if(!h168::directory(folder.c_str()))
        return {false,"Backup folder unavailable",{}};
    std::string name,error;
    if(!h168::uniqueWrite(folder,"live_",first.save.get(),name,error))
        return {false,"LIVE SD backup: "+error,{}};
    // Read back directly from the new file, even though uniqueWrite already
    // did its own verification, so the receipt never covers an absent file.
    const auto saved=h168::readExact((folder+"/"+name).c_str());
    if(!saved || std::memcmp(saved.get(),first.save.get(),h168::Size)!=0)
        return {false,"LIVE SD backup readback mismatch",name};
    auto third=read();
    if(!third.ok || !third.save)
        return {false,"Backup saved; LIVE recheck failed",name};
    if(std::memcmp(third.save.get(),first.save.get(),h168::Size)!=0)
        return {false,"Backup saved; LIVE changed since read",name};
    return {true,"LIVE backup OK: "+name,name};
}
} // namespace rogue_live_backup
