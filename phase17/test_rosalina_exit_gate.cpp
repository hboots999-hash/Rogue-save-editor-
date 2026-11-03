// SPDX-License-Identifier: GPL-3.0-or-later
// L-free Phase17 diagnostic control; no real 3DS hardware or save data.
#include "rosalina_exit_gate.hpp"
#include <cassert>
#include <iostream>
using phase17::StartExitGate;
int main() {
    StartExitGate a{};
    assert(!a.tick(true,false,false,false)); // START down
    assert(a.pending());
    assert(!a.tick(false,false,false,false)); // held
    assert(a.tick(false,true,false,false)); // START release -> exit
    assert(!a.pending());
    StartExitGate together{};
    assert(!together.tick(true,false,true,true)); // START+SELECT together
    assert(!together.tick(false,true,false,true)); // START released
    assert(!together.tick(false,false,false,false));
    StartExitGate selectFirst{};
    assert(!selectFirst.tick(false,false,true,true)); // SELECT first
    assert(!selectFirst.tick(true,false,false,true)); // START next
    assert(!selectFirst.tick(false,true,false,false)); // SELECT released first
    StartExitGate startFirst{};
    assert(!startFirst.tick(true,false,false,false)); // START first
    assert(!startFirst.tick(false,false,true,true)); // then SELECT
    assert(!startFirst.tick(false,false,false,false)); // SELECT released
    assert(!startFirst.tick(false,true,false,false)); // no exit!
    StartExitGate selectOnly{};
    assert(!selectOnly.tick(false,false,true,true));
    assert(!selectOnly.tick(false,false,false,false));
    assert(!selectOnly.pending());
    // After a Rosalina combo, a NEW independent START alone may exit.
    assert(!startFirst.tick(true,false,false,false));
    assert(startFirst.tick(false,true,false,false));
    std::cout<<"PASS Phase17 L-free START/SELECT: solo release exits; "
              <<"all Rosalina orders preserved.\n";
}
