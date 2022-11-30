#include "Mnemosyne.hpp"

using namespace std;


bool Mnemosyne::_init = false;

std::string Mnemosyne::dir_assets_base = "";
std::string Mnemosyne::dir_audio = "";
std::string Mnemosyne::dir_definition = "";
std::string Mnemosyne::dir_font = "";
std::string Mnemosyne::dir_shader = "";
std::string Mnemosyne::dir_stage = "";
std::string Mnemosyne::dir_texture = "";

std::map<std::string, sf::Font> Mnemosyne::fonts;
std::map<std::string, sf::Image> Mnemosyne::images;
std::map<std::string, sf::Texture> Mnemosyne::textures;



void Mnemosyne::Init()
{
	if (_init) return;

	cout << "[ Mnemosyne : Initializing ]" << endl;

	dir_assets_base = "./assets/";
	dir_audio = "audio/";
	dir_definition = "definition/";
	dir_font = "font/";
	dir_shader = "shader/";
	dir_stage = "stage/";
	dir_texture = "texture/";

	images.clear();
	textures.clear();

	images["n/a"].loadFromFile(dir_assets_base + dir_texture + "_na.png");
	textures["n/a"].loadFromImage(images["n/a"]);

	_init = true;
}



sf::Font& Mnemosyne::GetFont(std::string name)
{
	if (!_init) Mnemosyne::Init();

	if (fonts.find(name) == fonts.end())
	{
		std::vector<std::string> exts({ "otf", "ttf" });
		for (int i = 0; i < exts.size(); i++)
		{
			if (fonts[name].loadFromFile(dir_assets_base + dir_font + name + "." + exts[i]))
			{
				break;
			}
			else
			{
			}
		}
	}

	return fonts[name];
}


sf::Image& Mnemosyne::GetImage(std::string name)
{
	if (!_init) Mnemosyne::Init();

	//check if not here
	return images[name];
}

sf::Texture& Mnemosyne::GetTexture(std::string name)
{
	if (!_init) Mnemosyne::Init();

	if (textures.find(name) == textures.end())
	{
		if (images.find(name) == images.end())
		{
			if (!Mnemosyne::LoadImage(name))
			{
				textures[name] = textures["n/a"];
				return textures[name];
			}
		}

		textures[name].loadFromImage(images[name]);
		textures[name].setSmooth(true);

		bool repeating = false;
		if (name.substr(0, 12) == "environment/") repeating = true;
		textures[name].setRepeated(repeating);
	}

	return textures[name];
}


bool Mnemosyne::LoadImage(std::string name, std::string path)
{
	if (!_init) Mnemosyne::Init();
	if (path == "") path = name + ".png";

	if (dir_texture != "texture/")
	{
		dir_texture = "texture/";
		cout << "Repairing texture path" << endl;
	}

	sf::Image img;
	bool success = img.loadFromFile(dir_assets_base + dir_texture + path);
	cout << "Loading image: '" << dir_assets_base << dir_texture << path << "'" << endl;

	if (success)
	{
		images[name] = img;
	}

	return success;
}