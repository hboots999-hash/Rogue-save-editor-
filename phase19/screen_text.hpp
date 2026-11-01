// Phase19 allocation-free line wrapping for both 3DS screens and host tests.
// Keeps hard newlines, wraps on spaces when possible, and splits long words.
#pragma once
#include <cstddef>

namespace phase19 {
template <typename Emit>
inline void forEachWrappedLine(const char* raw,unsigned width,Emit emit) {
    if(!raw || !width) return;
    const char* p=raw;
    while(*p) {
        if(*p=='\n'){emit(p,0u);++p;continue;}
        const char* stop=p;
        while(*stop && *stop!='\n' &&
              static_cast<unsigned>(stop-p)<width)++stop;
        const char* cut=stop;
        if(*stop && *stop!='\n') {
            const char* boundary=stop;
            while(boundary>p && boundary[-1]!=' ')--boundary;
            if(boundary>p)cut=boundary;
        }
        const char* last=cut;
        while(last>p && last[-1]==' ')--last;
        emit(p,static_cast<unsigned>(last-p));
        p=cut;
        while(*p==' ')++p;
        if(*p=='\n')++p;
    }
}

// Capped renderer for 3DS: never emit more than maxLines. Reserves the
// last row for an explicit ellipsis when there is more material than fits.
// A malformed multi-KiB error cannot push A/B return instructions off-screen.
// Processing is O(width * maxLines), without heap memory or recursion.
template <typename Emit>
inline void forEachBoundedLine(const char* raw,unsigned width,unsigned maxLines,
                               Emit emit) {
    if(!raw || !width || !maxLines)return;
    const char* p=raw;
    unsigned lines=0;
    while(*p && lines<maxLines) {
        const char* begin=p;
        const char* cut=p;
        unsigned len=0;
        if(*p=='\n') {
            ++p;
        } else {
            while(*cut && *cut!='\n' && len<width) {
                ++cut;++len;
            }
            if(*cut && *cut!='\n') {
                const char* word=cut;
                while(word>begin && word[-1]!=' ')--word;
                if(word>begin)cut=word;
            }
            const char* end=cut;
            while(end>begin && end[-1]==' ')--end;
            len=static_cast<unsigned>(end-begin);
            p=cut;
            while(*p==' ')++p;
            if(*p=='\n')++p;
        }
        if(lines+1u==maxLines && *p) {
            constexpr char marker[]="[MORE TEXT OMITTED]";
            const unsigned markerLen=sizeof(marker)-1u;
            emit(marker,markerLen<width?markerLen:width);
            return;
        }
        emit(begin,len);
        ++lines;
    }
}
} // namespace phase19
