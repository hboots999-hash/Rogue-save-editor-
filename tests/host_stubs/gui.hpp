#pragma once
#include <string>
struct PKSM_Color { PKSM_Color(int,int,int,int) {} };
#define COLOR_WHITE PKSM_Color(255,255,255,255)
constexpr int FONT_SIZE_11=11,FONT_SIZE_14=14;
enum class TextPosX { LEFT,CENTER };
enum class TextPosY {TOP};
enum class TextWidthAction {SQUISH};
namespace Gui {
inline void exitMainLoop() {}
inline void text(const std::string&,float,float,int,PKSM_Color,TextPosX,TextPosY,TextWidthAction=TextWidthAction::SQUISH,float=0) {}
inline void drawSolidRect(float,float,float,float,PKSM_Color) {}
inline void backgroundBottom(bool) {}
}
