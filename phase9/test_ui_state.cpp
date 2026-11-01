// Synthetic-only host UI contract. Never opens any save or game archive.
#include "ui_state.hpp"
#include <cassert>
#include <iostream>
using namespace rogue_ui;

int main() {
    State s;
    const auto key = [&](Key k, bool supported = true) { press(s, k, 2, 9, supported); };
    assert(s.page == Page::Summary && s.mode == Mode::List);
    key(Key::Open, false);
    key(Key::Stage, false);
    assert(s.mode == Mode::List);
    key(Key::NextPage);
    assert(s.page == Page::Party && s.selected == 0);
    key(Key::Down); assert(s.selected == 1);
    key(Key::Down); assert(s.selected == 1); // bound to party count
    key(Key::Open); assert(s.mode == Mode::Detail);
    key(Key::Down); assert(s.selected == 1); // details do not move list
    key(Key::Back); assert(s.mode == Mode::List);
    key(Key::Stage); assert(s.mode == Mode::ConfirmOne);
    key(Key::NextPage); assert(s.page == Page::Party); // modal is sticky
    key(Key::Open); assert(s.mode == Mode::ConfirmTwo);
    key(Key::Back); assert(s.mode == Mode::List); // cancellation
    key(Key::Stage);
    key(Key::Open);
    key(Key::Open);
    assert(s.mode == Mode::Blocked); // no save-write transition exists
    key(Key::Open); assert(s.mode == Mode::Blocked);
    key(Key::Back); assert(s.mode == Mode::List);
    key(Key::NextPage); assert(s.page == Page::Bag && s.selected == 0);
    for (unsigned i = 0; i < 30; ++i) key(Key::Down);
    assert(s.selected == 8);
    key(Key::PrevPage); assert(s.page == Page::Party && s.selected == 0);
    key(Key::PrevPage); assert(s.page == Page::Summary);
    key(Key::Open); assert(s.mode == Mode::Detail);
    key(Key::Stage); assert(s.mode == Mode::ConfirmOne);
    key(Key::Exit); assert(s.exitRequested);
    key(Key::Open); assert(s.mode == Mode::ConfirmOne);
    assert(visibleCount(s, 1000, 1000) == 0);
    State capped; capped.page = Page::Party;
    assert(visibleCount(capped, 1000, 1000) == 6);
    capped.page = Page::Bag;
    assert(visibleCount(capped, 1000, 1000) == 450);
    std::cout << "PASS Phase 9 navigation: missing copy, bounds, details, cancel, double-confirm blocked, exit\n";
}
