// SPDX-License-Identifier: GPL-3.0-or-later
// Optional read-only exact-ROM move catalogue provider for Phase20.
// Generated 14,040-byte user-owned metadata is read from SDMC, never ROM
// bytes, never installed-title archives. No move-replacement Apply permission.
#pragma once
#include "phase18/move_replacement_v2.hpp"
#include "move_browser.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace phase20 {
constexpr const char* LocalMoveCatalogPath=
    "sdmc:/3ds/PKSM-Rogue/catalogs/exact_rom_move_metadata.bin";
struct LocalMoveProvider {
    std::array<uint8_t,rogue18::move_v2::PackedBytes> packed{};
    std::array<phase19::MoveRecord,935> rows{};
    phase19::MoveCatalog catalog{};
    bool loaded=false;

    bool read(const char* path=LocalMoveCatalogPath) {
        loaded=false;
        catalog=phase19::MoveCatalog{};
        // Fixed-size reader; fail closed for missing, short and oversize data.
        // Never fall back to arbitrary ROM/installed-title reads.
        if(!path)return false;
        std::FILE* f=std::fopen(path,"rb");
        if(!f)return false;
        const size_t n=std::fread(packed.data(),1,packed.size(),f);
        const int extra=std::fgetc(f);
        const int err=std::ferror(f);
        const int close=std::fclose(f);
        if(n!=packed.size() || extra!=EOF || err || close!=0)return false;
        const rogue18::move_v2::Catalog db{
            packed.data(),packed.size(),rogue18::move_v2::RomSha};
        if(!rogue18::move_v2::authenticate(db))return false;
        unsigned shown=0;
        for(unsigned id=1;id<rogue18::move_v2::RecordCount;id++){
            const uint8_t* raw=packed.data()+id*rogue18::move_v2::PackedRowBytes;
            phase19::MoveRecord m{};
            m.id=static_cast<uint16_t>(id);
            const bool ordinary=id<=848 && id!=165;
            const bool hasName=rogue18::move_v2::textKnown(raw);
            rogue18::move_v2::Move candidate{};
            if(ordinary && rogue18::move_v2::lookupAuthenticated(
                                         db,static_cast<uint16_t>(id),candidate)) {
                m.evidence=phase19::MoveEvidence::Candidate;
                m.basePP=candidate.basePP;
            }else if(!ordinary){
                m.evidence=id==165 || id>883?
                    phase19::MoveEvidence::Special:phase19::MoveEvidence::Dummy;
            }else if(raw[13]!=raw[14]){
                m.evidence=phase19::MoveEvidence::ConflictingPP;
            }else{
                m.evidence=phase19::MoveEvidence::Unknown;
            }
            if(hasName) {
                rogue18::move_v2::Move spelling{};
                spelling.id=static_cast<uint16_t>(id);
                std::memcpy(spelling.rawName,raw,rogue18::move_v2::NameBytes);
                char name[14]{};
                if(rogue18::move_v2::displayName(spelling,name))
                    std::snprintf(m.name,sizeof(m.name),"%s",name);
            }
            if(!m.name[0])std::snprintf(m.name,sizeof(m.name),
                                      "Unavailable ID %u",id);
            rows[shown++]=m;
        }
        catalog.version=phase19::MoveCatalogApi;
        catalog.origin=phase19::CatalogOrigin::ExactRomReviewed;
        catalog.count=shown;
        catalog.records=rows.data();
        catalog.romSha256=rogue18::move_v2::RomSha;
        loaded=phase19::validMoveCatalog(catalog);
        return loaded;
    }
    const phase19::MoveCatalog* verifiedMetadata() const {
        return loaded?&catalog:nullptr;
    }
};
static_assert(!rogue18::move_v2::ApplyApproved,
              "Production move replacement must remain disabled");
} // namespace phase20
