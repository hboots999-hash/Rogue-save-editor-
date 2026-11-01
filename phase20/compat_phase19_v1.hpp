// Phase20 compatibility adapter for Phase19 snapshots expecting global h168.
// Scoped exclusively to READ-ONLY Phase20 TUs; NEVER include Phase14/17 internals.
// Phase18 now owns its separate lightweight parser namespace rogue18::raw_v1.
// Remove after Phase19 adopts explicit rogue18::raw_v1 types throughout.
#pragma once
#include "phase18/offline_engine.hpp"
namespace h168 = rogue18::raw_v1;
