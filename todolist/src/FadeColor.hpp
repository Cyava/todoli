#pragma once


#include "inc.hpp"
#include <SFML/Graphics/Color.hpp>


struct FadeColor
{
	enum class Mode { Linear, Quadratic };


	float rgba[4] = { 0.f, 0.f, 0.f, 255.f };

	int previous[4] = { 0, 0, 0, 255 };
	int targets[4] = { 0, 0, 0, 255 };

	Mode mode = Mode::Linear;
	float speed = .02f;


	FadeColor() {}
	FadeColor(int r, int g, int b, int a = 255);


	sf::Color Color();

	bool SetRGB(int r, int g, int b, int a = 255);
	bool SetRGB(std::vector<int>);

	void Update();
};