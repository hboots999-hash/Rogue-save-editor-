/*
 * PKSM Rogue Edition experimental read-only inspection screen.
 * Copyright (C) 2026 contributors to PKSM Rogue Edition.
 * GPL-3.0-or-later; see upstream PKSM licence and notices.
 * NO GAME STORAGE OR SAVE WRITES IN THIS MODULE.
 */
#include "RogueInspectScreen.hpp"
#include "ScreenStack.hpp"
#include "gui.hpp"
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

    void drawRow(const std::string& text, unsigned row)
    {
        Gui::text(text, 12, 52 + static_cast<int>(row) * 20, FONT_SIZE_11, COLOR_WHITE,
            TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SQUISH, 376);
    }
}

RogueInspectScreen::RogueInspectScreen()
    : Screen("L/R: page   Up/Down: scroll   B: exit")
{
    const auto bytes = readCopiedSave(kImportPath);
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
    status = "CANDIDATE ONLY - ROM NOT VERIFIED";
}

RogueInspectScreen::~RogueInspectScreen() = default;


void RogueInspectScreen::stageDraft()
{
    if (!snapshot || !originalRaw) return;
    draftRaw = std::make_unique<uint8_t[]>(ROGUE_RAW_SAVE_SIZE);
    stagedChangedBytes = 0;
    const rogue_edit_status result = rogue_make_offline_draft(
        originalRaw.get(), ROGUE_RAW_SAVE_SIZE, &edits,
        draftRaw.get(), ROGUE_RAW_SAVE_SIZE, &stagedChangedBytes);
    if (result == ROGUE_EDIT_OK && stagedChangedBytes > 0)
    {
        awaitingExport = true;
        status = "Preview OK - START again exports COPY";
    }
    else
    {
        awaitingExport = false;
        draftRaw.reset();
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
        return true;
    }
    status = "No unused export filename";
    return false;
}



void RogueInspectScreen::drawTop() const
{
    Gui::drawSolidRect(0, 0, 400, 240, PKSM_Color(15, 22, 89, 255));
    Gui::text("PKSM ROGUE - READ ONLY", 200, 12, FONT_SIZE_14, COLOR_WHITE,
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
            drawRow("#" + std::to_string(index+1) + " Species ID " +
                std::to_string(p.species_id) + "  item ID " + std::to_string(p.held_item_id), i);
        }
    }
    else
    {
        for (unsigned i=0; i<kRowsPerPage; ++i)
        {
            unsigned index = firstRow+i;
            rogue_item_candidate item{};
            if (!rogue_snapshot_item(snapshot.get(), index, &item)) break;
            drawRow("Slot " + std::to_string(item.slot_index) + "  ID " +
                std::to_string(item.item_id) + "  x" + std::to_string(item.quantity), i);
        }
    }
}

void RogueInspectScreen::drawBottom() const
{
    Gui::backgroundBottom(true);
    Gui::text("Candidate only - not ROM verified", 160, 90, FONT_SIZE_11, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
    Gui::text("L/R: page  X: item x999  A: money max", 160, 125, FONT_SIZE_11, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
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
        edits.change_money = !edits.change_money;
        edits.money = 999999;
        awaitingExport = false;
        draftRaw.reset();
        status = edits.change_money ? "Money 999999 staged (not saved)" : "Money edit cancelled";
        return;
    }
    if ((keys & KEY_X) && page == Page::Bag)
    {
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
    const unsigned current = static_cast<unsigned>(page);
    if (keys & KEY_R) { page = static_cast<Page>((current + 1) % 3); firstRow = 0; }
    else if (keys & KEY_L) { page = static_cast<Page>((current + 2) % 3); firstRow = 0; }
    else if (keys & KEY_DOWN)
    {
        const unsigned count = page == Page::Party ? preview.party_count :
            (page == Page::Bag ? preview.occupied_bag_slots : 0);
        if (count > kRowsPerPage && firstRow + kRowsPerPage < count &&
            firstRow < kMaxBagPages) ++firstRow;
    }
    else if (keys & KEY_UP && firstRow > 0) --firstRow;
}
