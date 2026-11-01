// SPDX-License-Identifier: GPL-3.0-or-later
// Phase16 host-only synthetic recovery journal rehearsal, mock MAC not real CMAC.
#include "rehearsal.hpp"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <fcntl.h>
#include <unistd.h>

namespace fs=std::filesystem;
using phase14::Bytes;
using phase16::Code;
struct FakeDevice {
    Bytes live;
    unsigned reads=0,macCalls=0,failRead=0,failMac=0,changeRead=0;
    bool unstable=false;
    explicit FakeDevice(Bytes initial):live(std::move(initial)){}
    bool read(Bytes& b){
        ++reads;
        if(reads==failRead)return false;
        b=live;
        if(reads==changeRead)b.back()^=1;
        return true;
    }
    bool calculate(const phase14::Digest& d,phase14::Mac& mac){
        ++macCalls;
        if(macCalls==failMac)return false;
        const auto digest=phase14::hash(d.data(),d.size());
        std::copy_n(digest.begin(),mac.size(),mac.begin());
        if(unstable)mac[0]^=static_cast<uint8_t>(macCalls);
        return true;
    }
};
static Bytes sourceContainer(const Bytes& raw){
    Bytes container(rogue_live::ContainerMin+128,0x5a);
    for(size_t off:{size_t(0),size_t(rogue_live::SlotSize)}){
        std::fill_n(container.begin()+off,rogue_live::HeaderSize,0);
        std::memcpy(container.data()+off,".SAV",4);
        h168::w32(container.data(),off+0x34,off?48:49);
        h168::w32(container.data(),off+0x38,uint32_t(rogue_live::ExpectedTitle));
        h168::w32(container.data(),off+0x3c,uint32_t(rogue_live::ExpectedTitle>>32));
        std::fill_n(container.begin()+off+0x40,16,0x43);
        h168::w32(container.data(),off+0x50,rogue_live::HeaderSize);
        h168::w32(container.data(),off+0x54,rogue_live::RawSize);
        std::copy(raw.begin(),raw.end(),container.begin()+off+rogue_live::HeaderSize);
        FakeDevice mock(container);
        phase14::Mac mac{};
        assert(phase14::stableMac(mock,phase14::slotHash(container,off),mac));
        std::copy(mac.begin(),mac.end(),container.begin()+off+0x10);
    }
    return container;
}

class DiskStore {
    fs::path root;
    static phase16::State status(const fs::path& p){
        std::error_code ec;
        const bool exists=fs::exists(p,ec);
        if(ec)return phase16::State::Error;
        if(!exists)return phase16::State::Missing;
        return fs::is_regular_file(p,ec)&&!ec?
                phase16::State::Present:phase16::State::Error;
    }
    static bool readExact(const fs::path& p,Bytes& out){
        out.clear();
        std::ifstream in(p,std::ios::binary);
        if(!in)return false;
        Bytes b((std::istreambuf_iterator<char>(in)),{});
        if(in.bad())return false;
        out=std::move(b);
        return true;
    }
    static bool uniqueWrite(const fs::path& path,const Bytes& b){
        const int fd=::open(path.c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);
        if(fd<0)return false;
        size_t written=0;
        bool ok=true;
        while(written<b.size()){
            const ssize_t count=::write(fd,b.data()+written,b.size()-written);
            if(count<=0){ok=false;break;}
            written+=size_t(count);
        }
        if(::fsync(fd)!=0)ok=false;
        if(::close(fd)!=0)ok=false;
        Bytes back;
        return ok&&readExact(path,back)&&back==b;
    }
public:
    bool failBackup=false,failJournal=false;
    explicit DiskStore(fs::path path):root(std::move(path)){
        fs::create_directories(root);
    }
    phase16::State backupState()const{return status(root/"original.bin");}
    phase16::State journalState()const{return status(root/"rehearsal.rec");}
    bool createBackup(const Bytes& b)const{
        return !failBackup&&uniqueWrite(root/"original.bin",b);
    }
    bool createJournal(const Bytes& b)const{
        return !failJournal&&uniqueWrite(root/"rehearsal.rec",b);
    }
    bool readBackup(Bytes& b)const{return readExact(root/"original.bin",b);}
    bool readJournal(Bytes& b)const{return readExact(root/"rehearsal.rec",b);}
    const fs::path& directory()const{return root;}
};
static void replaceBytes(const fs::path& path,Bytes b){
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    assert(out);
    out.write(reinterpret_cast<const char*>(b.data()),b.size());
    assert(out.good());
}

int main(int argc,char** argv){
    assert(argc==2);
    static_assert(!phase14::LiveWritesEnabled,"Forbidden game writes");
    std::ifstream in(argv[1],std::ios::binary);
    const Bytes raw((std::istreambuf_iterator<char>(in)),{});
    assert(raw.size()==h168::Size);
    const Bytes original=sourceContainer(raw);
    assert(original.size()==263168);
    char tmp[]="/tmp/phase16-rehearsal-XXXXXX";
    const char* temp=mkdtemp(tmp);assert(temp);
    const fs::path root=temp;
    unsigned serial=0;
    auto store=[&]{return DiskStore(root/("test-"+std::to_string(++serial)));};

    {
        auto disk=store();
        FakeDevice device(original);
        const auto result=phase16::createRehearsal(device,disk);
        assert(result.code==Code::Created && result.liveSafe);
        assert(result.originalSize==263168);
        assert(result.journalSize==526400);
        assert(result.currentCounter==49);
        assert(disk.backupState()==phase16::State::Present);
        assert(disk.journalState()==phase16::State::Present);
        Bytes saved,record;assert(disk.readBackup(saved)&&saved==original);
        assert(disk.readJournal(record)&&record.size()==526400);
        phase14::Plan plan{};assert(phase14::decodeJournal(record,plan));
        assert(plan.before==original && plan.after!=original);
        assert(plan.slots.target==rogue_live::SlotSize);
        assert(plan.after.size()==original.size());
        // The previously current, authenticated slot is untouched.
        assert(std::equal(original.begin(),original.begin()+rogue_live::SlotSize,
                           plan.after.begin()));
        FakeDevice restarted(original);
        DiskStore reopened(disk.directory());
        const auto verify=phase16::verifyAfterRestart(restarted,reopened);
        assert(verify.code==Code::Verified&&verify.liveSafe&&
               verify.journalSize==526400);
        // Files are immutable: second create is BLOCKED, no overwrite.
        assert(phase16::createRehearsal(restarted,reopened).code==Code::AlreadyPresent);
        Bytes still;assert(reopened.readBackup(still)&&still==original);
        // A later game save is detected and is never overwritten.
        FakeDevice changed(original);changed.live.back()^=1;
        assert(phase16::verifyAfterRestart(changed,reopened).code==Code::LiveNowDifferent);
        // Bit-flipped journal fails SHA-256 record check.
        auto broken=record;broken[1000]^=1;
        replaceBytes(reopened.directory()/"rehearsal.rec",broken);
        assert(phase16::verifyAfterRestart(restarted,reopened).code==Code::JournalInvalid);
        // Truncated journal fails.
        broken.pop_back();
        replaceBytes(reopened.directory()/"rehearsal.rec",broken);
        assert(phase16::verifyAfterRestart(restarted,reopened).code==Code::JournalInvalid);
        // Exact full journal but wrong original container fails.
        replaceBytes(reopened.directory()/"rehearsal.rec",record);
        saved.back()^=1;
        replaceBytes(reopened.directory()/"original.bin",saved);
        assert(phase16::verifyAfterRestart(restarted,reopened).code==Code::BackupMismatch);
    }
    {
        auto disk=store();FakeDevice device(original);
        assert(phase16::verifyAfterRestart(device,disk).code==Code::MissingRecord);
    }
    for(unsigned fail=1;fail<=2;fail++){
        auto disk=store();FakeDevice device(original);device.failRead=fail;
        const Code expect=fail==1?Code::FirstReadError:Code::SecondReadError;
        assert(phase16::createRehearsal(device,disk).code==expect);
        assert(disk.backupState()==phase16::State::Missing);
        assert(disk.journalState()==phase16::State::Missing);
    }
    {
        auto disk=store();FakeDevice device(original);device.changeRead=2;
        assert(phase16::createRehearsal(device,disk).code==Code::LiveChanged);
        assert(disk.backupState()==phase16::State::Missing);
    }
    {
        auto disk=store();FakeDevice device(original);device.failMac=1;
        assert(phase16::createRehearsal(device,disk).code==Code::LiveAuthenticationFailed);
        assert(disk.backupState()==phase16::State::Missing);
    }
    {
        auto disk=store();FakeDevice device(original);device.unstable=true;
        assert(phase16::createRehearsal(device,disk).code==Code::LiveAuthenticationFailed);
    }
    {
        auto disk=store();FakeDevice device(original);disk.failBackup=true;
        assert(phase16::createRehearsal(device,disk).code==Code::BackupWriteFailed);
        assert(disk.journalState()==phase16::State::Missing);
    }
    {
        auto disk=store();FakeDevice device(original);disk.failJournal=true;
        assert(phase16::createRehearsal(device,disk).code==Code::JournalWriteFailed);
        assert(disk.backupState()==phase16::State::Present);
        assert(phase16::createRehearsal(device,disk).code==Code::PartialRecord);
        assert(phase16::verifyAfterRestart(device,disk).code==Code::PartialRecord);
    }
    {
        auto disk=store();FakeDevice device(original);device.failRead=3;
        assert(phase16::createRehearsal(device,disk).code==Code::RecheckReadError);
        assert(disk.backupState()==phase16::State::Present);
        assert(disk.journalState()==phase16::State::Present);
    }
    {
        auto disk=store();FakeDevice device(original);device.changeRead=3;
        assert(phase16::createRehearsal(device,disk).code==Code::LiveChangedAfterJournal);
        assert(disk.backupState()==phase16::State::Present);
        assert(disk.journalState()==phase16::State::Present);
    }
    {
        auto disk=store();FakeDevice device(original);device.reads=0;
        assert(phase16::createRehearsal(device,disk).code==Code::Created);
        FakeDevice failing(original);failing.failRead=1;
        assert(phase16::verifyAfterRestart(failing,disk).code==Code::RecheckReadError);
    }
    {
        auto disk=store();FakeDevice device(original);
        fs::create_directory(disk.directory()/"original.bin");
        assert(phase16::createRehearsal(device,disk).code==Code::StorageError);
    }
    // Defensive checks: no source mutation or installed-title writes in harness.
    assert(original==sourceContainer(raw));
    fs::remove_all(root);
    std::cout<<"PASS Phase16: independent full-container backup + authentic-format journal;\n"
             <<"restart/reopen, tamper/truncation, unique filenames, stale reads and fail-closed errors.\n"
             <<"MOCK MAC ONLY: physical signing/restore not established.\n";
}
