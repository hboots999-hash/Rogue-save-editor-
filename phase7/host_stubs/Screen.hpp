#pragma once
#include <string>
#include "3ds.h"
class Screen {public: explicit Screen(const std::string& title = {}){(void)title;} virtual ~Screen()=default; virtual void drawTop() const=0; virtual void drawBottom() const=0; virtual void update(touchPosition*)=0;};
