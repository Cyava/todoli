#pragma once

#include "inc.hpp"
#include <SFML/Graphics.hpp>

// Resource manager class.
// Named after the Greek goddess of memory and remembrance.
class Mnemosyne
{
private:
	static bool _init;

	static std::string dir_assets_base;
	static std::string dir_audio;
	static std::string dir_definition;
	static std::string dir_font;
	static std::string dir_shader;
	static std::string dir_stage;
	static std::string dir_texture;

public:
	static std::map<std::string, sf::Font> fonts;
	static std::map<std::string, sf::Image> images;
	static std::map<std::string, sf::Texture> textures;


	static void Init();

	static std::string AssetsDirectory() { return dir_assets_base; }
	static std::string AudioDirectory() { return dir_assets_base + dir_audio; }
	static std::string DefinitionDirectory() { return dir_assets_base + dir_definition; }
	static std::string FontDirectory() { return dir_assets_base + dir_font; }
	static std::string ShaderDirectory() { return dir_assets_base + dir_shader; }
	static std::string StageDirectory() { return dir_assets_base + dir_stage; }
	static std::string TextureDirectory() { return dir_assets_base + dir_texture; }

	static sf::Font& GetFont(std::string);
	static sf::Image& GetImage(std::string);
	static sf::Texture& GetTexture(std::string);


	static bool LoadImage(std::string, std::string = "");

	/*
		let's do something fancy where there's a map of name -> path,
		then there'll be a load function that knows how to load an
		image just by name (using the path). Maybe?
	*/
};
