// SPDX-License-Identifier: GPL-3.0-or-later
// Phase17 read-only diagnostic has no L-dependent controls.
// Preserve START+SELECT Rosalina in both press orders.
// This is intentionally independent of Phase19/stock PKSM UI headers.
#pragma once
namespace phase17 {
class StartExitGate {
    bool pending_=false;
    bool combo_=false;
public:
    bool tick(bool startDown,bool startUp,
              bool selectDown,bool selectHeld) {
        if(startDown) {
            pending_=true;
            combo_=selectDown || selectHeld;
        }
        if(pending_ && (selectDown || selectHeld))combo_=true;
        if(startUp) {
            const bool exit=pending_ && !combo_ &&
                            !selectDown && !selectHeld;
            pending_=false;combo_=false;
            return exit;
        }
        return false;
    }
    bool pending() const {return pending_;}
};
} // namespace phase17
