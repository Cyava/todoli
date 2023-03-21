#pragma once


#include "inc.hpp"
#include "Mnemosyne.hpp"


struct TextInfo
{
	std::string str;


	static std::vector<std::string> StringToLines(std::string, std::string font, int fontsize = 16, int maxwidth = -1);

	static float TextHeight(sf::Text);
	static float TextHeadspace(sf::Text);
	static float TextTop(sf::Text);

	static float StandardTextHeight(sf::Text);
	static float StandardTextHeadspace(sf::Text);
	static float StandardTextTop(sf::Text);
};