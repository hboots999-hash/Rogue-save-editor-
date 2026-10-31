// PKSM Rogue Phase 10, experimental H168 GBA-VC offline-copy inspector.
// GPL-3.0-or-later. Never opens installed-title archives, AGBSAVE or extdata.
#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <new>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace h168 {
constexpr size_t Size=131072;
constexpr size_t Sector=4096;
struct Bank { bool valid=false; unsigned sector[14]{}; uint32_t counter=0,key=0; };
struct Info { bool valid=false; unsigned active=0,partyCount=0,items=0; uint32_t money=0; Bank banks[2]{}; };
struct Result { bool ok=false; std::string message; unsigned changed=0; };

inline uint16_t r16(const uint8_t* p,size_t i) {
    return uint16_t(p[i])|(uint16_t(p[i+1])<<8);
}
inline uint32_t r32(const uint8_t* p,size_t i){
    return uint32_t(r16(p,i))|(uint32_t(r16(p,i+2))<<16);
}
inline void w16(uint8_t* p,size_t i,uint16_t v){
    p[i]=uint8_t(v);p[i+1]=uint8_t(v>>8);
}
inline void w32(uint8_t* p,size_t i,uint32_t v){
    w16(p,i,uint16_t(v));w16(p,i+2,uint16_t(v>>16));
}
inline uint16_t checksum(const uint8_t* sector){
    uint32_t sum=0;
    for(size_t j=0;j<0xff4;j+=4)sum+=r32(sector,j);
    return uint16_t((sum>>16)+sum);
}
inline bool validMon(const uint8_t* source,size_t start){
    if(start+104>Size)return false;
    const uint32_t key=r32(source,start)^r32(source,start+4);
    uint32_t sum=0;
    for(unsigned j=0;j<48;j+=4) {
        const uint32_t word=r32(source,start+32+j)^key;
        sum+=uint16_t(word); sum+=uint16_t(word>>16);
    }
    return uint16_t(sum)==r16(source,start+28);
}
inline bool inspect(const uint8_t* source,size_t n,Info& out){
    out=Info{};
    if(!source||n!=Size)return false;
    for(unsigned b=0;b<2;b++){
        Bank& bank=out.banks[b];
        bool used[14]={};
        bool ok=true;
        uint32_t counter=0;
        for(unsigned p=0;p<14;p++){
            const size_t start=(b*14u+p)*Sector;
            const uint16_t id=r16(source,start+0xff4);
            const uint32_t c=r32(source,start+0xffc);
            if(id>=14 || used[id] || r32(source,start+0xff8)!=0x08012025u ||
               checksum(source+start)!=r16(source,start+0xff6) ||
               (p && c!=counter)){ok=false;break;}
            used[id]=true;
            bank.sector[id]=static_cast<unsigned>(start);
            counter=c;
        }
        if(!ok)continue;
        const unsigned s1=bank.sector[1];
        const unsigned party=r32(source,s1+0x234);
        if(party>6)continue;
        for(unsigned m=0;m<party;m++)
            if(!validMon(source,s1+0x238+104u*m)){ok=false;break;}
        if(!ok)continue;
        bank.counter=counter;
        bank.key=r32(source,bank.sector[0]+0x4c);
        const uint32_t money=r32(source,s1+0x4a8)^bank.key;
        if(money>999999u)continue;
        bank.valid=true;
    }
    // Strict two-complete-bank requirement, not sufficient for ROM authenticity.
    if(!out.banks[0].valid || !out.banks[1].valid ||
       out.banks[0].counter==out.banks[1].counter)return false;
    out.active=out.banks[1].counter>out.banks[0].counter?1u:0u;
    const Bank& bank=out.banks[out.active];
    const unsigned s1=bank.sector[1];
    out.money=r32(source,s1+0x4a8)^bank.key;
    out.partyCount=r32(source,s1+0x234);
    bool gap=false;
    for(unsigned j=0;j<450;j++){
        const size_t off=s1+0x578+4u*j;
        const uint16_t id=r16(source,off);
        if(!id){gap=true;continue;}
        if(gap)return false; // only a contiguous observed bag layout
        const unsigned qty=r16(source,off+2)^uint16_t(bank.key);
        if(qty<1||qty>999)return false;
        out.items++;
    }
    out.valid=true;
    return true;
}
inline unsigned species(const uint8_t* b,const Info& info,unsigned i){
    if(!b||!info.valid||i>=info.partyCount)return 0;
    const unsigned mon=info.banks[info.active].sector[1]+0x238+i*104u;
    const unsigned pid=r32(b,mon), ot=r32(b,mon+4);
    // Gen III canonical 24 permutations, find which 12-byte block is G.
    static const char* order[24]={
       "GAEM","GAME","GEAM","GEMA","GMAE","GMEA",
       "AGEM","AGME","AEGM","AEMG","AMGE","AMEG",
       "EGAM","EGMA","EAGM","EAMG","EMGA","EMAG",
       "MGAE","MGEA","MAGE","MAEG","MEGA","MEAG"
    };
    unsigned block=0;
    while(block<4&&order[pid%24][block]!='G')block++;
    if(block==4)return 0;
    return uint16_t(r32(b,mon+32+block*12)^(pid^ot));
}
inline bool item(const uint8_t* b,const Info& info,unsigned i,unsigned& id,unsigned& qty){
    if(!b||!info.valid||i>=info.items)return false;
    const Bank& bank=info.banks[info.active];
    const size_t off=bank.sector[1]+0x578+i*4u;
    id=r16(b,off);
    qty=r16(b,off+2)^uint16_t(bank.key);
    return id!=0 && qty>0 && qty<=999;
}
struct FreeFILE{void operator()(FILE* f) const {if(f)fclose(f);}};
inline std::unique_ptr<uint8_t[]> readExact(const char* path){
    if(!path)return {};
    std::unique_ptr<FILE,FreeFILE> in(fopen(path,"rb"));
    if(!in)return {};
    std::unique_ptr<uint8_t[]> b(new(std::nothrow) uint8_t[Size]);
    if(!b || fread(b.get(),1,Size,in.get())!=Size || fgetc(in.get())!=EOF ||
       ferror(in.get()))return {};
    return b;
}
inline bool directory(const char* path){
    if(mkdir(path,0777)!=0 && errno!=EEXIST)return false;
    struct stat st{};
    return stat(path,&st)==0 && S_ISDIR(st.st_mode);
}
inline std::string ioError(const char* operation,int code){
    char text[96];
    std::snprintf(text,sizeof(text),"%s (errno %d)",operation,code);
    return std::string(text);
}
// Preserve every pre-existing file; failures are reported by stage and errno.
// Any partial file is left at its unique name for inspection; never overwritten.
inline bool uniqueWrite(const std::string& folder,const char* prefix,
                        const uint8_t* bytes,std::string& filename,
                        std::string& error){
    error.clear();
    if(!bytes){error="Null source";return false;}
    for(unsigned i=1;i<=99;i++){
        char base[64];
        std::snprintf(base,sizeof(base),"%s%03u.sav",prefix,i);
        const std::string full=folder+"/"+base;
        errno=0;
        int fd=::open(full.c_str(),O_CREAT|O_EXCL|O_WRONLY,0666);
        if(fd<0){
            const int e=errno;
            if(e==EEXIST)continue;
            error=ioError("open",e);return false;
        }
        size_t count=0;
        while(count<Size){
            errno=0;
            const ssize_t n=::write(fd,bytes+count,Size-count);
            if(n<=0){
                const int e=errno;
                error=ioError("write",e);
                (void)::close(fd);return false;
            }
            count+=size_t(n);
        }
        errno=0;
        const int synced=::fsync(fd);
        const int syncError=errno;
        errno=0;
        const int closed=::close(fd);
        const int closeError=errno;
        if(synced!=0){
            error=ioError("fsync",syncError);return false;
        }
        if(closed!=0){
            error=ioError("close",closeError);return false;
        }
        const auto back=readExact(full.c_str());
        if(!back){error="readback open/size";return false;}
        if(std::memcmp(back.get(),bytes,Size)!=0){
            error="readback content mismatch";return false;
        }
        filename=base;
        return true;
    }
    error="all 99 backup names occupied";
    return false;
}
// A deliberately tiny experimental edit surface: +1 to MONEY or an EXISTING item.
// Offline source copy only; caller must perform distinct deliberate A+A confirmation.
inline Result exportIncrement(const uint8_t* source,const char* importPath,
                              const char* root,bool moneyEdit,unsigned itemIndex=0){
    if(!source||!importPath||!root)return {false,"Invalid input",0};
    Info info{};
    if(!inspect(source,Size,info))return {false,"Invalid/unknown save: READ ONLY",0};
    auto fresh=readExact(importPath);
    if(!fresh||memcmp(fresh.get(),source,Size)!=0)return {false,"Import changed: BLOCKED",0};
    const Bank& bank=info.banks[info.active];
    const unsigned s1=bank.sector[1];
    size_t field=0;
    uint32_t next=0,old=0;
    unsigned fieldWidth=0;
    if(moneyEdit){
        if(info.money>=999999)return {false,"Money already at limit",0};
        field=s1+0x4a8;
        old=info.money;next=old+1;fieldWidth=4;
    }else{
        unsigned id=0,qty=0;
        if(!item(source,info,itemIndex,id,qty)||qty>=999)
            return {false,"Item not supported/at limit",0};
        field=s1+0x578+itemIndex*4u+2;
        old=qty;next=old+1;fieldWidth=2;
    }
    std::unique_ptr<uint8_t[]> draft(new(std::nothrow) uint8_t[Size]);
    if(!draft)return {false,"Memory failure",0};
    memcpy(draft.get(),source,Size);
    if(moneyEdit)w32(draft.get(),field,next^bank.key);
    else w16(draft.get(),field,uint16_t(next)^uint16_t(bank.key));
    const unsigned ck=s1+0xff6;
    w16(draft.get(),ck,checksum(draft.get()+s1));
    unsigned changed=0;
    for(size_t j=0;j<Size;j++){
        if(draft[j]==source[j])continue;
        if(!((j>=field && j<field+fieldWidth) || j==ck || j==ck+1))
            return {false,"Unexpected write footprint: BLOCKED",0};
        changed++;
    }
    if(changed==0||changed>6)return {false,"Invalid edit difference",0};
    Info after{};
    if(!inspect(draft.get(),Size,after) || after.active!=info.active ||
       after.banks[0].counter!=info.banks[0].counter ||
       after.banks[1].counter!=info.banks[1].counter ||
       after.partyCount!=info.partyCount || after.items!=info.items ||
       (moneyEdit && after.money!=next))
        return {false,"Independent draft readback failed",0};
    if(!moneyEdit){
        unsigned id0=0,oldqty=0,id1=0,newqty=0;
        if(!item(source,info,itemIndex,id0,oldqty) ||
           !item(draft.get(),after,itemIndex,id1,newqty) ||
           id0!=id1 || oldqty!=old || newqty!=next)
            return {false,"Item draft mismatch",0};
    }
    // Recheck imported bytes immediately BEFORE and AFTER creating backup.
    fresh=readExact(importPath);
    if(!fresh||memcmp(fresh.get(),source,Size)!=0)return {false,"Stale before backup",0};
    if(!directory(root))return {false,"App folder unavailable",0};
    const std::string backups=std::string(root)+"/backups";
    if(!directory(backups.c_str()))return {false,"Backup folder unavailable",0};
    std::string bk,backupFailure;
    if(!uniqueWrite(backups,"import_",source,bk,backupFailure))
        return {false,"BACKUP: "+backupFailure,0};
    fresh=readExact(importPath);
    if(!fresh||memcmp(fresh.get(),source,Size)!=0)
        return {false,"Stale after backup: no draft",0};
    const std::string drafts=std::string(root)+"/drafts";
    if(!directory(drafts.c_str()))return {false,"Draft folder unavailable",0};
    std::string df,draftFailure;
    if(!uniqueWrite(drafts,"h168_",draft.get(),df,draftFailure))
        return {false,"DRAFT: "+draftFailure,0};
    return {true,"OFFLINE draft: "+df,changed};
}
} // namespace h168
