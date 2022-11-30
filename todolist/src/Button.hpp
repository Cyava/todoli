#pragma once

#include "inc.hpp"
#include "FadeColor.hpp"
#include "Mnemosyne.hpp"


struct Button
{
	enum class State { Up, Down, Clicked, Released };
	State state = State::Up;

	enum class ScaleMode { Linear, Quadratic };


	std::string id;

	sf::Texture* icon = nullptr;
	sf::Vector2f pos;

	ScaleMode scalemode = ScaleMode::Linear;
	float radius = 16.f;
	float scale = 0.f;
	float pscale = 0.f;
	float scalespeed = .1f;

	bool shown = false;
	bool held = false;


	Button();
	Button(std::string);
	Button(std::string, std::string);

	void Draw(sf::RenderWindow*);

	bool Init();

	void SetPosition(sf::Vector2f);
	void SetTexture(std::string);
	void SetVisible(bool);

	void Update();

	bool Visible();
};


struct ButtonGroup
{
	std::vector<Button> buttons;


	ButtonGroup() {}

	void SetVisible(bool);

	void Update();
};
