#pragma once

#include "inc.hpp"

#include <SFML/Graphics.hpp>


const int GRAB_KEY_NONE = -1;
const int GRAB_KEY_UNUSED = 0;
const int GRAB_KEY_TASK = 1;
const int GRAB_KEY_TASKBUTTON = 2;
const int GRAB_KEY_MENUBAR = 3;
const int GRAB_KEY_SCROLLBAR = 4;


struct MouseState
{
	enum class ButtonState { Up, Pressed, Held, Released };

	sf::Vector2i gpos;
	sf::Vector2i pos;
	sf::Vector2i delta;

	sf::Vector2f winscale;

	int grabbed_key = GRAB_KEY_NONE;		// what type of thing has the mouse grabbed?
	int grabbed_id = -1;		// what has the mouse grabbed?
	int grabbed_id_sub = -1;	// in case we're not really sure

	ButtonState buttons[3];


	MouseState()
	{
		buttons[0] = ButtonState::Up;
		buttons[1] = ButtonState::Up;
		buttons[2] = ButtonState::Up;
	}


	void Update(sf::Vector2i, sf::RenderWindow*);
};