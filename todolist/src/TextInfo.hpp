#pragma once


#include "inc.hpp"
#include "Mnemosyne.hpp"


struct TextInfo
{
	std::string str;


	static std::vector<std::string> StringToLines(std::string, std::string font, int fontsize = 16, int maxwidth = -1);
};