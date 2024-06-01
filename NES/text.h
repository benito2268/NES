#pragma once
#pragma warning(disable : 4996)

#include "defs.h"
#include "drawable.h"
#include <string>
#include <SFML/Graphics.hpp>

class cTextBox : public cDrawable {
private:
	u32 fontsize;
	std::string str;

	sf::Font *font;
	sf::Text *text;

public:
	cTextBox(u32, u32, u32, std::string);
	~cTextBox();

	void draw(sf::RenderWindow *win) override;
	void set_text(std::string);
	void set_color(sf::Color c);
};