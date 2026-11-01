#pragma once
#include <string>
#include <vector>
struct PKSM_Color{PKSM_Color(int,int,int,int){}};
#define COLOR_WHITE PKSM_Color(255,255,255,255)
constexpr int FONT_SIZE_11=11,FONT_SIZE_14=14;
enum class TextPosX{LEFT,CENTER};
enum class TextPosY{TOP};
enum class TextWidthAction{SQUISH};
namespace Gui {
inline std::vector<std::string>& captured_text(){static std::vector<std::string> text;return text;}
inline void text(const std::string& s,float,float,int,PKSM_Color,TextPosX,TextPosY,TextWidthAction=TextWidthAction::SQUISH,float=0){captured_text().push_back(s);}
inline void drawSolidRect(float,float,float,float,PKSM_Color){}
inline void backgroundBottom(bool){}
}
