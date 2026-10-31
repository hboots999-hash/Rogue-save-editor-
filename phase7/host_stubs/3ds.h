#pragma once
#include <cstdint>
using u32=uint32_t;
struct touchPosition {};
constexpr u32 KEY_B=1,KEY_L=2,KEY_R=4,KEY_UP=8,KEY_DOWN=16,KEY_X=32,KEY_Y=64,KEY_A=128,KEY_LEFT=256,KEY_RIGHT=512;
inline u32& stub_key_state(){static u32 key=0;return key;}
inline u32 hidKeysDown(){const u32 key=stub_key_state();stub_key_state()=0;return key;}
