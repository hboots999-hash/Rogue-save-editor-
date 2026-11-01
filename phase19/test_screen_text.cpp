// Phase19 portable 3DS console wrapping + paging tests.
#include "screen_text.hpp"
#include "ui.hpp"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
using namespace phase19;
int main(){
    auto wrap=[](const char* s,unsigned width){
        std::vector<std::string> lines;
        forEachWrappedLine(s,width,[&](const char* p,unsigned n){lines.emplace_back(p,n);});
        for(const auto& line:lines)assert(line.size()<=width);
        return lines;
    };
    auto a=wrap("INVALID/UNVERIFIED: unchanged value or unsupported move ID.",31);
    assert(a.size()>=2 && a.front()=="INVALID/UNVERIFIED: unchanged");
    auto b=wrap("NO SAVE / WRONG GAME\nKeep original save safe.\nCheck the installed title ID.",31);
    assert(b.size()==3 && b[0]=="NO SAVE / WRONG GAME" && b[1]=="Keep original save safe.");
    auto c=wrap("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",31);
    assert(c.size()==2 && c[0].size()==31 && c[1].size()==21);
    auto d=wrap("\nShort   spaces\n",10);
    assert(d.size()>=2 && d.front().empty());
    auto e=wrap("",31);assert(e.empty());
    auto f=wrap(nullptr,31);assert(f.empty());
    assert(firstVisible(0)==0 && lastVisible(0,0)==0);
    assert(firstVisible(5)==0 && lastVisible(5,19)==6);
    assert(firstVisible(6)==6 && lastVisible(6,19)==12);
    assert(firstVisible(18)==18 && lastVisible(18,19)==19);
    assert(firstVisible(449)==444 && lastVisible(449,450)==450);
    // Error messages and export filenames must never scroll off the 3DS
    // console and hide A/B recovery prompts.
    auto capped=[](const char* message,unsigned width,unsigned height){
        std::vector<std::string> result;
        forEachBoundedLine(message,width,height,[&](const char* p,unsigned n){
            result.emplace_back(p,n);
        });
        for(const auto& line:result)assert(line.size()<=width);
        return result;
    };
    std::string lengthy(12000,'X');
    auto longLines=capped(lengthy.c_str(),31,9);
    assert(longLines.size()==9 && longLines.back()=="[MORE TEXT OMITTED]");
    auto exact=capped("1234567890",5,2);
    assert(exact.size()==2 && exact[0]=="12345" && exact[1]=="67890");
    auto truncated=capped("first second third fourth fifth sixth",8,2);
    assert(truncated.size()==2 && truncated.back()=="[MORE TE");
    auto newline=capped("A\nB\nC\nD",31,3);
    assert(newline.size()==3 && newline.back()=="[MORE TEXT OMITTED]");
    assert(capped("anything",31,0).empty());
    assert(capped("anything",0,6).empty());
    assert(capped(nullptr,31,6).empty());
    assert(firstVisible(0xffffffffu)==4294967292u);
    assert(lastVisible(0xffffffffu,450u)==450u);
    std::cout<<"PASS 3DS console status text wrapping and safe pagination (450 items)\n";
}
