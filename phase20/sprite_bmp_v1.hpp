// SPDX-License-Identifier: GPL-3.0-or-later
// Strict, read-only 48x48 BGR24 bitmap header parser for owner-local sprites.
// Input is a 54-byte header (pixels are read separately), bottom-up, uncompressed.
// Require exact trailing length and no BMP palette/profile/directory appendices.
// No Pokémon artwork is bundled. Missing/malformed sprites fall back to
// a neutral generated avatar and cannot affect save-editing code.
#pragma once
#include <cstddef>
#include <cstdint>
namespace phase20 { namespace gui {
struct SpriteBmpInfo { unsigned pixelOffset=0,rowBytes=0; };
inline uint16_t bmp16(const uint8_t* data,unsigned p){
    return uint16_t(data[p])|(uint16_t(data[p+1])<<8);
}
inline uint32_t bmp32(const uint8_t* data,unsigned p){
    return uint32_t(bmp16(data,p))|(uint32_t(bmp16(data,p+2))<<16);
}
inline bool validateSpriteBmp(const uint8_t* data,size_t n,SpriteBmpInfo& info){
    info=SpriteBmpInfo{};
    if(!data || n<54 || data[0]!='B' || data[1]!='M')return false;
    if(bmp32(data,14)!=40 || bmp32(data,18)!=48 ||
       bmp32(data,22)!=48 || bmp16(data,26)!=1 ||
       bmp16(data,28)!=24 || bmp32(data,30)!=0)return false;
    const unsigned offset=bmp32(data,10);
    constexpr unsigned rowBytes=144, pixelBytes=rowBytes*48;
    const unsigned fileBytes=bmp32(data,2);
    if(offset<54 || offset>1024 || fileBytes!=offset+pixelBytes ||
       fileBytes>16384u || bmp32(data,6)!=0 ||
       bmp32(data,46)!=0 || bmp32(data,50)!=0)return false;
    const uint32_t declaredPixels=bmp32(data,34);
    if(declaredPixels!=0 && declaredPixels!=pixelBytes)return false;
    info.pixelOffset=offset;info.rowBytes=rowBytes;
    return true;
}
}} // namespace phase20::gui
