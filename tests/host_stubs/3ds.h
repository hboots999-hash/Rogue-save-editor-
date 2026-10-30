#pragma once
#include <cstdint>
using u32 = uint32_t;
struct touchPosition {};
constexpr u32 KEY_B=1,KEY_L=2,KEY_R=4,KEY_UP=8,KEY_DOWN=16,KEY_A=32,KEY_X=64,KEY_START=128;
inline u32 simulatedKeysDown = 0;
inline u32 hidKeysDown(){const u32 result=simulatedKeysDown;simulatedKeysDown=0;return result;}
