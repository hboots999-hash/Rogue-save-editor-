/*
 * PKSM Rogue Edition experimental offline save editor screen.
 * Copyright (C) 2026 contributors to PKSM Rogue Edition.
 * GPL-3.0-or-later; see upstream PKSM licence and notices.
 * Only opt-in imported/copied saves are used; no installed-game save writes.
 */
#include "RogueInspectScreen.hpp"
#include "ScreenStack.hpp"
#include "gui.hpp"
#include "candidate_profile.hpp"
#include "observed_profile.hpp"
#include <3ds.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <memory>
#include <string>

namespace {
    constexpr char kImportPath[] = "sdmc:/3ds/PKSM-Rogue/imports/00000001.sav";
    constexpr unsigned kRowsPerPage = 8;
    constexpr unsigned kMaxBagPages = 512;
    struct FileCloser { void operator()(FILE* p) const { if(p) std::fclose(p); } };

    // A separate, opt-in SD copy is opened ONLY with rb. No other
    // game save archive (or PKSM's regular Sav3 code) is touched.
    std::unique_ptr<unsigned char[]> readCopiedSave(const char* filename)
    {
        std::unique_ptr<FILE, FileCloser> input(std::fopen(filename, "rb"));
        if (!input) return nullptr;
        auto bytes = std::make_unique<unsigned char[]>(ROGUE_RAW_SAVE_SIZE);
        if (std::fread(bytes.get(), 1, ROGUE_RAW_SAVE_SIZE, input.get()) != ROGUE_RAW_SAVE_SIZE)
            return nullptr;
        // Reject longer files; exact 128 KiB only.
        if (std::fgetc(input.get()) != EOF || std::ferror(input.get())) return nullptr;
        return bytes;
    }

    bool ensureDir(const char* name)
    {
        if (::mkdir(name, 0777) == 0) return true;
        struct stat st{};
        return errno == EEXIST && ::stat(name, &st) == 0 && S_ISDIR(st.st_mode);
    }

    // Only creates new, separate files. Never opens existing game saves for writing.
    bool createVerifiedCopy(const std::string& path, const uint8_t* source)
    {
        int flags = O_WRONLY | O_CREAT | O_EXCL;
#ifdef O_NOFOLLOW
        flags |= O_NOFOLLOW;
#endif
        const int fd = ::open(path.c_str(), flags, 0600);
        if (fd < 0) return false;
        bool okay = true;
        std::size_t offset = 0;
        while (offset < ROGUE_RAW_SAVE_SIZE)
        {
            const ssize_t wrote = ::write(fd, source + offset, ROGUE_RAW_SAVE_SIZE - offset);
            if (wrote < 0 && errno == EINTR) continue;
            if (wrote <= 0) { okay = false; break; }
            offset += static_cast<std::size_t>(wrote);
        }
        if (okay && ::fsync(fd) != 0) okay = false;
        if (::close(fd) != 0) okay = false;
        if (okay)
        {
            const auto readback = readCopiedSave(path.c_str());
            okay = readback && std::memcmp(readback.get(), source, ROGUE_RAW_SAVE_SIZE) == 0;
        }
        if (!okay)
        {
            // We own this pathname because O_EXCL succeeded. Never clean up
            // files that we merely attempted to create without ownership.
            (void)::unlink(path.c_str());
        }
        return okay;
    }


    // IDs and names from Pokabbie/pokeemerald-rogue EX 2.1.2
    // include/constants/rogue_hub.h, commit 518b7b4d. Display only.
    const char* const kAreaNames[11] = {
        "Town Square", "Adventure Entrance", "Labs", "Safari Zone",
        "Challenge Frontier", "Berry Field", "Home", "Marts",
        "Ride Training", "Day Care", "Market"
    };
    const char* const kUpgradeNames[48] = {
        "School", "Tutors", "Random starter", "Adventure replay",
        "Unique Mon Lab", "Unique slots I", "Unique slots II",
        "Rare rarity", "Epic rarity", "Exotic rarity", "Reserved rarity",
        "Home lower floor", "Reserved upper floor", "Home styles",
        "Berry brewing", "Reserved brewing", "Berry field I",
        "Berry field II", "Berry field III", "Berry yield I",
        "Berry yield II", "Berry yield III", "Legends Cave",
        "General stock", "Poke Ball stock", "More Poke Balls",
        "TMs", "More TMs", "Battle enhancers", "Held items",
        "Day Care capacity I", "Day Care capacity II", "Breeder",
        "Seen Dex breeder", "Reserved breeder", "Day Care phone I",
        "Tea Shop", "Exp Share I", "Exp Share II", "Exp Share III",
        "Pokeblock blenders", "Bakery", "Treat Shop", "More treats",
        "Market bank", "Day Care phone II", "Day Care phone III",
        "Unique typings"
    };

    void drawRow(const std::string& text, unsigned row)
    {
        Gui::text(text, 12, 52 + static_cast<int>(row) * 20, FONT_SIZE_11, COLOR_WHITE,
            TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SQUISH, 376);
    }
}

RogueInspectScreen::RogueInspectScreen()
    : Screen("L/R: page   Up/Down: scroll   B: exit")
{
    auto bytes = readCopiedSave(kImportPath);
    if (!bytes)
    {
        status = "Missing, unreadable or wrong-size SD copy";
        return;
    }
    snapshot.reset(rogue_snapshot_open(bytes.get(), ROGUE_RAW_SAVE_SIZE));
    if (!snapshot || !rogue_snapshot_summary(snapshot.get(), &preview))
    {
        status = "Unsupported or invalid candidate save";
        snapshot.reset();
        return;
    }
    originalRaw = std::move(bytes);
    hubStatus = rogue_probe_hub(originalRaw.get(), ROGUE_RAW_SAVE_SIZE, &hubPreview);
    status = "CANDIDATE ONLY - ROM NOT VERIFIED";
}

RogueInspectScreen::~RogueInspectScreen() = default;


void RogueInspectScreen::stageDraft()
{
    if (!snapshot || !originalRaw) return;
    draftRaw = std::make_unique<uint8_t[]>(ROGUE_RAW_SAVE_SIZE);
    stagedChangedBytes = 0;
    const int result = masterBallInsertActive
        ? int(rogue_make_master_ball_draft(
            originalRaw.get(), ROGUE_RAW_SAVE_SIZE,
            draftRaw.get(), ROGUE_RAW_SAVE_SIZE, &stagedChangedBytes))
        : pokemonEditActive
        ? int(rogue_make_pokemon_stats_draft(
            originalRaw.get(), ROGUE_RAW_SAVE_SIZE, &pokemonEdit,
            draftRaw.get(), ROGUE_RAW_SAVE_SIZE, &stagedChangedBytes))
        : int(rogue_make_offline_draft(
            originalRaw.get(), ROGUE_RAW_SAVE_SIZE, &edits,
            draftRaw.get(), ROGUE_RAW_SAVE_SIZE, &stagedChangedBytes));
    if (result == 0 && stagedChangedBytes > 0)
    {
        awaitingExport = true;
        status = "Preview OK - START again exports COPY";
    }
    else
    {
        awaitingExport = false;
        draftRaw.reset();
        if (masterBallInsertActive && result == ROGUE_BALL_INSERT_ALREADY_OWNED)
            status = "Master Ball owned: select its slot + X";
        else if (masterBallInsertActive && result == ROGUE_BALL_INSERT_NO_CAPACITY)
            status = "Bag has no verified spare capacity";
        else if (masterBallInsertActive && result == ROGUE_BALL_INSERT_BAG_UNSAFE)
            status = "Bag order/pocket unsafe: no draft";
        else if (masterBallInsertActive && result == ROGUE_BALL_INSERT_UNSUPPORTED_VERSION)
            status = "Insert supported only on EX 2.1.2";
        else
            status = "Draft refused (code " + std::to_string(int(result)) + ")";
    }
}

bool RogueInspectScreen::exportVerified()
{
    awaitingExport = false;
    if (!originalRaw || !draftRaw || !snapshot || !stagedChangedBytes)
        return false;
    const auto latest = readCopiedSave(kImportPath);
    if (!latest || std::memcmp(originalRaw.get(), latest.get(), ROGUE_RAW_SAVE_SIZE) != 0)
    {
        status = "Source changed! No files exported";
        return false;
    }
    constexpr char originalDir[] = "sdmc:/3ds/PKSM-Rogue/originals";
    constexpr char draftDir[] = "sdmc:/3ds/PKSM-Rogue/drafts";
    if (!ensureDir(originalDir) || !ensureDir(draftDir))
    {
        status = "Could not create original/draft folders";
        return false;
    }
    for (unsigned i = 0; i < 1000; ++i)
    {
        char name[64];
        std::snprintf(name, sizeof(name), "%lld_%03u.sav",
            static_cast<long long>(std::time(nullptr)), i);
        const std::string originalPath = std::string(originalDir) + "/" + name;
        const std::string draftPath = std::string(draftDir) + "/" + name;
        struct stat st{};
        if (::stat(originalPath.c_str(), &st) == 0 ||
            ::stat(draftPath.c_str(), &st) == 0) continue;
        if (!createVerifiedCopy(originalPath, originalRaw.get()))
        {
            status = "Original backup failed; no draft";
            return false;
        }
        if (!createVerifiedCopy(draftPath, draftRaw.get()))
        {
            status = "Backup safe; draft export failed";
            return false;
        }
        status = "Saved original + draft with readback";
        draftRaw.reset();
        edits = rogue_draft_request{};
        pokemonEdit = rogue_pokemon_stats_request{};
        pokemonEditActive = false;
        masterBallInsertActive = false;
        return true;
    }
    status = "No unused export filename";
    return false;
}



void RogueInspectScreen::drawTop() const
{
    Gui::drawSolidRect(0, 0, 400, 240, PKSM_Color(15, 22, 89, 255));
    Gui::text("PKSM ROGUE - OFFLINE EDITOR", 200, 12, FONT_SIZE_14, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
    Gui::text(status, 200, 34, FONT_SIZE_11, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP, TextWidthAction::SQUISH, 390);
    if (!snapshot) return;
    if (page == Page::Summary)
    {
        drawRow("Candidate marker: " + std::to_string(preview.serialized_marker), 0);
        drawRow("Money: " + std::to_string(preview.money), 1);
        drawRow("Save counter: " + std::to_string(preview.save_counter), 2);
        drawRow("Party size: " + std::to_string(preview.party_count), 3);
        drawRow("Occupied bag slots: " + std::to_string(preview.occupied_bag_slots), 4);
        drawRow("A: stage 999999 money (offline draft)", 5);
        drawRow("START: preview / confirm export", 6);
        drawRow("NEVER writes installed game save", 7);
    }
    else if (page == Page::Party)
    {
        for (unsigned i=0; i<kRowsPerPage; ++i)
        {
            unsigned index = firstRow+i;
            rogue_party_candidate p{};
            if (!rogue_snapshot_party(snapshot.get(), index, &p)) break;
            drawRow((index == firstRow ? "> " : "  ") + std::string("#") + std::to_string(index+1) + " Species ID " +
                std::to_string(p.species_id) + "  item ID " + std::to_string(p.held_item_id), i);
        }
    }
    else if (page == Page::Bag)
    {
        for (unsigned i=0; i<kRowsPerPage; ++i)
        {
            unsigned index = firstRow+i;
            rogue_item_candidate item{};
            if (!rogue_snapshot_item(snapshot.get(), index, &item)) break;
            const std::string candidateName =
                item.item_id == 113 ? "Rare Candy*" :
                (item.item_id == 4 ? "Master Ball*" : "ID " + std::to_string(item.item_id));
            drawRow("Slot " + std::to_string(item.slot_index) + "  " +
                candidateName + "  x" + std::to_string(item.quantity), i);
        }
    }
    else if (page == Page::Hub)
    {
        if (hubStatus != ROGUE_HUB_OK)
        {
            drawRow("Hub layout not verified - display refused", 0);
            drawRow("No progression flags are editable", 1);
        }
        else
        {
            for (unsigned i = 0; i < kRowsPerPage; ++i)
            {
                const unsigned index = firstRow + i;
                if (index < 11)
                {
                    drawRow(std::string("Area ") + std::to_string(index) + ": " +
                        kAreaNames[index] +
                        (hubPreview.area_built[index] ? " [BUILT]" : " [LOCKED]"), i);
                }
                else if (index < 59)
                {
                    const unsigned upgrade = index - 11;
                    drawRow(std::string("Upg ") + std::to_string(upgrade) + ": " +
                        kUpgradeNames[upgrade] +
                        (hubPreview.upgrade_owned[upgrade] ? " [YES]" : " [NO]"), i);
                }
            }
        }
    }
}

void RogueInspectScreen::drawBottom() const
{
    Gui::backgroundBottom(true);
    Gui::text("Candidate only - not ROM verified", 160, 90, FONT_SIZE_11, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
    Gui::text(page == Page::Party ? "X: IVs31  Y: 252Atk/252Spd" :
        (page == Page::Bag ? "X: item x999  Y: add Master Ball x999" :
        "A: money 999999"), 160, 125, FONT_SIZE_11, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
    if (page == Page::Hub)
    {
        const std::string info = hubStatus == ROGUE_HUB_OK ?
            ("Areas: " + std::to_string(hubPreview.built_count) +
             "/11   Upgrades: " + std::to_string(hubPreview.upgrades_count) + "/48") :
            "Hub parser refused unknown layout";
        Gui::text(info, 160, 160, FONT_SIZE_11, COLOR_WHITE,
            TextPosX::CENTER, TextPosY::TOP);
        Gui::text("VIEW ONLY. Unlock editing disabled.", 160, 195,
            FONT_SIZE_11, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    }
    if (page == Page::Party && snapshot)
    {
        rogue_party_candidate p{};
        if (rogue_snapshot_party(snapshot.get(), firstRow, &p))
        {
            const auto statLine = [](const uint8_t* a) -> std::string {
                return std::to_string(a[0]) + "/" + std::to_string(a[1]) + "/" +
                    std::to_string(a[2]) + "/" + std::to_string(a[3]) + "/" +
                    std::to_string(a[4]) + "/" + std::to_string(a[5]);
            };
            Gui::text("IV HP/Atk/Def/Spd/SpA/SpD", 160, 155, FONT_SIZE_11, COLOR_WHITE,
                TextPosX::CENTER, TextPosY::TOP, TextWidthAction::SQUISH, 310);
            Gui::text(statLine(p.ivs), 160, 173, FONT_SIZE_11, COLOR_WHITE,
                TextPosX::CENTER, TextPosY::TOP);
            Gui::text("EV " + statLine(p.evs), 160, 191, FONT_SIZE_11, COLOR_WHITE,
                TextPosX::CENTER, TextPosY::TOP);
        }
    }
}

void RogueInspectScreen::update(touchPosition*)
{
    const u32 keys = hidKeysDown();
    if (keys & KEY_B)
    {
        if (awaitingExport) { awaitingExport = false; draftRaw.reset(); status = "Export cancelled"; }
        else Gui::exitMainLoop();
        return;
    }
    if (!snapshot || !originalRaw) return;

    if (keys & KEY_START)
    {
        if (awaitingExport) (void)exportVerified();
        else stageDraft();
        return;
    }

    if (keys & KEY_A)
    {
        pokemonEditActive = false;
        pokemonEdit = rogue_pokemon_stats_request{};
        masterBallInsertActive = false;
        edits.change_money = !edits.change_money;
        edits.money = 999999;
        awaitingExport = false;
        draftRaw.reset();
        status = edits.change_money ? "Money 999999 staged (not saved)" : "Money edit cancelled";
        return;
    }
    if ((keys & KEY_X) && page == Page::Bag)
    {
        pokemonEditActive = false;
        pokemonEdit = rogue_pokemon_stats_request{};
        masterBallInsertActive = false;
        rogue_item_candidate item{};
        if (rogue_snapshot_item(snapshot.get(), firstRow, &item))
        {
            if (edits.change_quantity && edits.item_slot == item.slot_index)
                edits.change_quantity = 0;
            else
            {
                edits.change_quantity = 1;
                edits.item_slot = item.slot_index;
                edits.expected_item_id = item.item_id;
                edits.quantity = 999;
            }
            awaitingExport = false;
            draftRaw.reset();
            status = edits.change_quantity ? "Existing item x999 staged" : "Item edit cancelled";
        }
        return;
    }
    if ((keys & KEY_Y) && page == Page::Bag)
    {
        pokemonEdit = rogue_pokemon_stats_request{};
        pokemonEditActive = false;
        edits = rogue_draft_request{};
        masterBallInsertActive = !masterBallInsertActive;
        awaitingExport = false;
        draftRaw.reset();
        status = masterBallInsertActive ?
            "Add 999 Master Balls staged (offline)" : "Master Ball insert cancelled";
        return;
    }
    if ((keys & (KEY_X | KEY_Y)) && page == Page::Party)
    {
        rogue_party_candidate party{};
        if (!rogue_snapshot_party(snapshot.get(), firstRow, &party))
            return;
        const auto candidate = rogue::inspectObserved(originalRaw.get(), ROGUE_RAW_SAVE_SIZE);
        if (candidate.status != rogue::ObservedStatus::Candidate)
        {
            status = "Party candidate invalid - no draft";
            return;
        }
        const auto base = std::size_t(candidate.selected) * 14 * rogue::SectorBytes;
        const auto slot = rogue::validateSlot(originalRaw.get() + base,
            14 * rogue::SectorBytes, rogue::kCandidateSectorSizes);
        if (!slot.valid)
        {
            status = "Party sector invalid - no draft";
            return;
        }
        const auto mon = originalRaw.get() + base +
            slot.physical[1] * rogue::SectorBytes + 0x238 + firstRow * 0x68;
        const uint32_t personality = uint32_t(mon[0]) | (uint32_t(mon[1]) << 8) |
            (uint32_t(mon[2]) << 16) | (uint32_t(mon[3]) << 24);
        edits = rogue_draft_request{};
        masterBallInsertActive = false;
        pokemonEdit = rogue_pokemon_stats_request{};
        pokemonEdit.party_index = static_cast<uint8_t>(firstRow);
        pokemonEdit.expected_species = party.species_id;
        pokemonEdit.expected_personality = personality;
        if (keys & KEY_X)
        {
            pokemonEdit.change_ivs = 1;
            for (auto& iv : pokemonEdit.ivs) iv = 31;
            status = "All IVs 31 staged (offline only)";
        }
        else
        {
            pokemonEdit.change_evs = 1;
            pokemonEdit.evs[0] = 4;
            pokemonEdit.evs[1] = 252;
            pokemonEdit.evs[3] = 252;
            status = "Atk252 Speed252 HP4 staged";
        }
        pokemonEditActive = true;
        awaitingExport = false;
        draftRaw.reset();
        return;
    }
    const unsigned current = static_cast<unsigned>(page);
    if (keys & KEY_R) { page = static_cast<Page>((current + 1) % 4); firstRow = 0; }
    else if (keys & KEY_L) { page = static_cast<Page>((current + 3) % 4); firstRow = 0; }
    else if (keys & KEY_DOWN)
    {
        const unsigned count = page == Page::Party ? preview.party_count :
            (page == Page::Bag ? preview.occupied_bag_slots :
            (page == Page::Hub && hubStatus == ROGUE_HUB_OK ? 59 : 0));
        if (firstRow + 1 < count && firstRow < kMaxBagPages) ++firstRow;
    }
    else if (keys & KEY_UP && firstRow > 0) --firstRow;
}
