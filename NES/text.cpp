#include "text.h"
#include <iostream>

cTextBox::cTextBox(u32 x, u32 y, u32 fontsize, std::string str)
	: cDrawable(x, y, 0, 0), fontsize{fontsize}, str{str}
{
	font = new sf::Font();
	text = new sf::Text();

	if(!font->loadFromFile("C:\\Users\\benst\\Desktop\\font2.ttf")) {
		std::cerr << "error: could not load font resource \n";
		std::exit(1);
	}

	text->setFont(*font);
	text->setString(str);
	text->setPosition(x, y);
	text->setCharacterSize(fontsize);
	text->setColor(sf::Color::White);
}

cTextBox::~cTextBox() {
	delete text;
}

void cTextBox::draw(sf::RenderWindow *win) {
	win->draw(*text);
}

void cTextBox::set_text(std::string newtext) {
	text->setString(newtext);
}

void cTextBox::set_color(sf::Color c) {
	text->setColor(c);
}