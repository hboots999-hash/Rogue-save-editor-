/*
 * PKSM Rogue Edition experimental read-only inspection screen.
 * Copyright (C) 2026 contributors to PKSM Rogue Edition.
 * GPL-3.0-or-later; see upstream PKSM licence and notices.
 * NO GAME STORAGE OR SAVE WRITES IN THIS MODULE.
 */
#ifndef ROGUE_INSPECT_SCREEN_HPP
#define ROGUE_INSPECT_SCREEN_HPP
#include "Screen.hpp"
#include "rogue_bridge.h"
#include <memory>
#include <string>

class RogueInspectScreen : public Screen
{
public:
    RogueInspectScreen();
    ~RogueInspectScreen() override;
    void drawTop() const override;
    void drawBottom() const override;
    void update(touchPosition* touch) override;

private:
    using SnapshotHandle = std::unique_ptr<rogue_snapshot, decltype(&rogue_snapshot_close)>;
    SnapshotHandle snapshot{nullptr, &rogue_snapshot_close};
    rogue_preview preview{};
    std::string status;
    enum class Page { Summary, Party, Bag } page = Page::Summary;
    unsigned firstRow = 0;
};
#endif
