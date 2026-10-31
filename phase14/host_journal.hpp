// SPDX-License-Identifier: GPL-3.0-or-later
// POSIX disposable-file test backend ONLY. Not a native SD durability claim.
#pragma once
#ifdef __3DS__
#error Host journal must not be used as a native recovery backend
#endif
#include "journal_codec.hpp"
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace phase14 {
inline bool readFile(const std::string& path,Bytes& out,size_t max=2*MaxContainer+64) {
    out.clear();int fd=::open(path.c_str(),O_RDONLY|O_NOFOLLOW);
    if(fd<0)return false;
    struct stat s{};
    if(fstat(fd,&s)!=0 || !S_ISREG(s.st_mode) || s.st_size<0 || uint64_t(s.st_size)>max){::close(fd);return false;}
    out.resize(size_t(s.st_size));size_t done=0;
    while(done<out.size()) {ssize_t n=::read(fd,out.data()+done,out.size()-done);
        if(n<0 && errno==EINTR)continue;
        if(n<=0){::close(fd);out.clear();return false;}done+=size_t(n);}
    uint8_t extra;const auto tail=::read(fd,&extra,1);const auto closed=::close(fd);
    return tail==0 && closed==0;
}
inline bool exclusiveFile(const std::string& path,const Bytes& b) {
    int fd=::open(path.c_str(),O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW,0600);
    if(fd<0)return false;
    size_t done=0;bool ok=true;
    while(done<b.size()) {ssize_t n=::write(fd,b.data()+done,b.size()-done);
        if(n<0 && errno==EINTR)continue;
        if(n<=0){ok=false;break;}done+=size_t(n);}
    if(::fsync(fd)!=0)ok=false;
    if(::close(fd)!=0)ok=false;
    const auto slash=path.find_last_of('/');
    const auto dir=slash==std::string::npos?".":path.substr(0,slash);
    int parent=::open(dir.c_str(),O_RDONLY|O_DIRECTORY);
    if(parent<0)return false;
    if(::fsync(parent)!=0)ok=false;
    if(::close(parent)!=0)ok=false;
    Bytes readback;return ok && readFile(path,readback) && readback==b;
}
// Dedicated test directory per transaction. Never replace or delete a record.
class HostJournal {
    std::string path;
public:
    explicit HostJournal(std::string p):path(std::move(p)){}
    bool unresolved() const {
        // Even completed transactions require a NEW directory/transaction.
        // Cannot silently run again against an old backup or broken record.
        struct stat s{};if(::lstat(path.c_str(),&s)==0)return true;
        return errno!=ENOENT;
    }
    bool persist(const Plan& p){const auto b=encodeJournal(p);return !b.empty()&&exclusiveFile(path,b);}
    bool verifyJournal(const Plan& p)const{Bytes b;return readFile(path,b)&&b==encodeJournal(p);}
    bool load(Plan& p)const{Bytes b;return readFile(path,b)&&decodeJournal(b,p);}
    bool finish(Status status) {
        if(status!=Status::Restored && status!=Status::RolledBack)return false;
        // Separate immutable receipts allow an explicit later rollback.
        Bytes b{uint8_t(status)};
        const auto name=path+(status==Status::Restored?".restored":".rolledback");
        Bytes old;if(readFile(name,old))return old==b;
        return exclusiveFile(name,b);
    }
};
}
