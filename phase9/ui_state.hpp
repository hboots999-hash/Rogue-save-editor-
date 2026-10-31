// PKSM Rogue Phase 9: platform-independent navigation contract.
// Intentionally contains no file operations and no save-writing API.
#pragma once
#include <cstdint>

namespace rogue_ui {
enum class Page : uint8_t { Summary, Party, Bag };
enum class Mode : uint8_t { List, Detail, ConfirmOne, ConfirmTwo, Blocked };
enum class Key : uint8_t { NextPage, PrevPage, Up, Down, Open, Back, Stage, Exit };

struct State {
    Page page = Page::Summary;
    Mode mode = Mode::List;
    unsigned selected = 0;
    bool exitRequested = false;
};

inline unsigned visibleCount(const State& s, unsigned partyCount, unsigned bagCount) {
    if (s.page == Page::Party) return partyCount < 6 ? partyCount : 6;
    if (s.page == Page::Bag) return bagCount < 450 ? bagCount : 450;
    return 0;
}

// Confirmation is a deliberately blocked rehearsal until the exact ROM profile
// has been validated. In particular, no key transition can write a file.
inline void press(State& s, Key key, unsigned partyCount, unsigned bagCount, bool supportedCopy) {
    if (key == Key::Exit) { s.exitRequested = true; return; }
    if (s.exitRequested) return;
    if (s.mode == Mode::ConfirmOne || s.mode == Mode::ConfirmTwo) {
        if (key == Key::Back) s.mode = Mode::List;
        else if (key == Key::Open)
            s.mode = s.mode == Mode::ConfirmOne ? Mode::ConfirmTwo : Mode::Blocked;
        return;
    }
    if (s.mode == Mode::Blocked) {
        if (key == Key::Back) s.mode = Mode::List;
        return;
    }
    if (s.mode == Mode::Detail) {
        if (key == Key::Back) s.mode = Mode::List;
        else if (key == Key::Stage && supportedCopy) s.mode = Mode::ConfirmOne;
        return;
    }
    if (key == Key::NextPage || key == Key::PrevPage) {
        unsigned page = static_cast<unsigned>(s.page);
        page = (page + (key == Key::NextPage ? 1u : 2u)) % 3u;
        s.page = static_cast<Page>(page);
        s.selected = 0;
    } else if (key == Key::Down) {
        const unsigned count = visibleCount(s, partyCount, bagCount);
        if (count > 0 && s.selected + 1 < count) ++s.selected;
    } else if (key == Key::Up) {
        if (s.selected > 0) --s.selected;
    } else if (key == Key::Open && supportedCopy) {
        if (s.page == Page::Summary || visibleCount(s, partyCount, bagCount) != 0)
            s.mode = Mode::Detail;
    } else if (key == Key::Stage && supportedCopy) {
        // The confirmation path explicitly ends in Blocked, never a draft.
        s.mode = Mode::ConfirmOne;
    }
}
} // namespace rogue_ui
