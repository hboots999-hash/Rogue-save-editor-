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
} // namespace phase19
