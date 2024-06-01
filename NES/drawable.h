#pragma once
#include "defs.h"

namespace sf { class RenderWindow; }

class cDrawable {
private:
	u32 x;
	u32 y;
	u32 width;
	u32 height;

public:
	cDrawable(u32 x, u32 y, u32 width, u32 height) 
		: x{x}, y{y}, width{width}, height{height}
	{ }

	~cDrawable() = default;

	virtual void draw(sf::RenderWindow *win) = 0;
};