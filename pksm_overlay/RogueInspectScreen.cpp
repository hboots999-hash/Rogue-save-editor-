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
    std::unique_ptr<unsigned char[]> readCopiedSave()
    {
        std::unique_ptr<FILE, FileCloser> input(std::fopen(kImportPath, "rb"));
        if (!input) return nullptr;
        auto bytes = std::make_unique<unsigned char[]>(ROGUE_RAW_SAVE_SIZE);
        if (std::fread(bytes.get(), 1, ROGUE_RAW_SAVE_SIZE, input.get()) != ROGUE_RAW_SAVE_SIZE)
            return nullptr;
        // Reject longer files; exact 128 KiB only.
        if (std::fgetc(input.get()) != EOF || std::ferror(input.get())) return nullptr;
        return bytes;
    }
    void drawRow(const std::string& text, unsigned row)
    {
        Gui::text(text, 12, 52 + static_cast<int>(row) * 20, FONT_SIZE_11, COLOR_WHITE,
            TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SQUISH, 376);
    }
}

RogueInspectScreen::RogueInspectScreen()
    : Screen("L/R: page   Up/Down: scroll   B: back")
{
    const auto bytes = readCopiedSave();
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
    status = "CANDIDATE ONLY - ROM NOT VERIFIED";
}

RogueInspectScreen::~RogueInspectScreen() = default;

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
        drawRow("No editing, export or restore available", 6);
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
    Gui::text("Read-only candidate data", 160, 100, FONT_SIZE_14, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
    Gui::text("L/R change page   B returns", 160, 135, FONT_SIZE_11, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
}

void RogueInspectScreen::update(touchPosition*)
{
    const u32 keys = hidKeysDown();
    if (keys & KEY_B) { ScreenStack::requestPop(); return; }
    if (!snapshot) return;
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
