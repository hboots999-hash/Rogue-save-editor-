// Phase19 START+SELECT / Rosalina-safe exit gate, GPL-3.0-or-later.
// Pure input model: standalone START exits *on release*. A START/SELECT
// chord never exits the app, regardless of key press order.
#pragma once
namespace phase19 {
struct StartExitGate {
    bool armed=false;
    bool chord=false;
    bool tick(bool startDown,bool startUp,bool selectDown,bool selectHeld) {
        if(startDown) {
            armed=true;
            chord=selectDown||selectHeld;
        }
        if(armed && (selectDown||selectHeld))chord=true;
        const bool exit=startUp && armed && !chord;
        if(startUp){armed=false;chord=false;}
        return exit;
    }
};
} // namespace phase19
