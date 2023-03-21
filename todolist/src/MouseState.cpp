#include "MouseState.hpp"


void MouseState::Update(sf::Vector2i _pos, sf::RenderWindow* window)
{
	//cout << _pos.x << ", " << _pos.y << endl;
	_pos.x /= winscale.x;
	_pos.y /= winscale.y;

	delta.x = _pos.x - gpos.x;
	delta.y = _pos.y - gpos.y;

	gpos = _pos;
	pos = sf::Vector2i(_pos.x - window->getPosition().x, _pos.y - window->getPosition().y);


	for (int i = 0; i < 3; i++)
	{
		if (buttons[i] == ButtonState::Pressed)
		{
			buttons[i] = ButtonState::Held;
		}
		else if (buttons[i] == ButtonState::Released)
		{
			buttons[i] = ButtonState::Up;

			grabbed_key = GRAB_KEY_NONE;
			grabbed_id = -1;
		}
	}
}