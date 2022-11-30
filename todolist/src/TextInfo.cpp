#include "TextInfo.hpp"



std::vector<std::string> TextInfo::StringToLines(std::string str, std::string font, int fontsize, int maxwidth)
{
	sf::Text strtext;
	strtext.setFont(Mnemosyne::GetFont(font));
	strtext.setCharacterSize(fontsize);

	std::vector<std::string> strlines;
	std::string substr = str;
	size_t subi = 0;
	size_t last_space = 0;
	int cwide = 0;

	for (;;)
	{
		if (subi >= substr.size())
		{
			strlines.push_back(substr);
			break;
		}

		if (substr[subi] == '\n')
		{
			if (subi == 0)
			{
				strlines.push_back("");
				substr = substr.substr(1);
				subi = 0;
				cwide = 0;
				last_space = 0;

				continue;
			}

			strlines.push_back(substr.substr(0, subi));
			substr = substr.substr(subi + 1);
			subi = 0;
			cwide = 0;
			last_space = 0;

			continue;
		}
#if 0
		else if (subdesc[subi] == '<')
		{
			++subi;
			continue;
		}
#endif

		if (substr[subi] == ' ') last_space = subi;
		strtext.setString(substr.substr(subi, 1));
		cwide += strtext.findCharacterPos(1).x;

		//desctext.setString(subdesc.substr(0, subi));
		//std::cout << "width: " << cwide << " / " << desctext.findCharacterPos(subi).x << std::endl;

		if (cwide > maxwidth)
		{
			if (last_space > 0)
			{
				subi = last_space;
			}
			else
			{
				//--subi;
			}
			strlines.push_back(substr.substr(0, subi));
			substr = substr.substr(subi + 1);
			subi = 0;
			cwide = 0;
			last_space = 0;

			continue;
		}

		++subi;
	}

	return strlines;
}
