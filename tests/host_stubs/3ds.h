#pragma once
#include <cstdint>
using u32 = uint32_t;
struct touchPosition {};
constexpr u32 KEY_B=1,KEY_L=2,KEY_R=4,KEY_UP=8,KEY_DOWN=16;
inline u32 hidKeysDown(){return 0;}
