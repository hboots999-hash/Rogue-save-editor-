#include "rosalina_input.hpp"
#include <cassert>
#include <iostream>
using namespace phase19;
int main() {
    StartExitGate g{};
    // Standalone START triggers only after release.
    assert(!g.tick(true,false,false,false));
    assert(!g.tick(false,false,false,false));
    assert(g.tick(false,true,false,false));
    assert(!g.tick(false,true,false,false));
    // Simultaneous START+SELECT is reserved for Rosalina.
    assert(!g.tick(true,false,true,true));
    assert(!g.tick(false,false,false,true));
    assert(!g.tick(false,true,false,true));
    // START pressed first, SELECT second: must never exit.
    assert(!g.tick(true,false,false,false));
    assert(!g.tick(false,false,true,true));
    assert(!g.tick(false,false,false,false));
    assert(!g.tick(false,true,false,false));
    // SELECT pressed first, then START: also must never exit.
    assert(!g.tick(false,false,true,true));
    assert(!g.tick(true,false,false,true));
    assert(!g.tick(false,true,false,false));
    // After an intercepted chord, standalone START still works.
    assert(!g.tick(true,false,false,false));
    assert(g.tick(false,true,false,false));
    // Select pressed on same frame START releases remains protected.
    assert(!g.tick(true,false,false,false));
    assert(!g.tick(false,true,true,true));
    std::cout<<"PASS Phase19 Rosalina START+SELECT chord never exits; standalone START release exits\n";
}
