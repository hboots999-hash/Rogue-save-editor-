/*
 * PKSM Rogue Edition experimental offline-save editor screen.
 * Copyright (C) 2026 contributors to PKSM Rogue Edition.
 * GPL-3.0-or-later; see upstream PKSM licence and notices.
 * Only separately imported and exported SD save copies; no installed-game writes.
 */
#ifndef ROGUE_INSPECT_SCREEN_HPP
#define ROGUE_INSPECT_SCREEN_HPP
#include "Screen.hpp"
#include "rogue_bridge.h"
#include "rogue_edit.h"
#include "rogue_pokemon_edit.h"
#include "rogue_ball_insert.h"
#include "rogue_hub_probe.h"
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
    rogue_hub_report hubPreview{};
    rogue_hub_status hubStatus = ROGUE_HUB_BAD_SOURCE;
    std::unique_ptr<uint8_t[]> originalRaw;
    std::unique_ptr<uint8_t[]> draftRaw;
    rogue_draft_request edits{};
    rogue_pokemon_stats_request pokemonEdit{};
    bool pokemonEditActive = false;
    bool masterBallInsertActive = false;
    size_t stagedChangedBytes = 0;
    bool awaitingExport = false;
    bool exportVerified();
    void stageDraft();
    std::string status;
    enum class Page { Summary, Party, Bag, Hub } page = Page::Summary;
    unsigned firstRow = 0;
};
#endif
