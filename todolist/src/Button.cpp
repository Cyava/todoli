#include "Button.hpp"


using namespace std;


Button::Button()
{
}

Button::Button(std::string _id)
{
	id = _id;
}

Button::Button(std::string _id, std::string _tex)
{
	id = _id;
	SetTexture(_tex);
}

void Button::Draw(sf::RenderWindow* window)
{
	if (!Visible()) return;

	sf::CircleShape c(radius);
	c.setOrigin(radius, radius);
	c.setPosition(pos);
	if (icon != nullptr)
	{
		icon->setSmooth(true);
		c.setTexture(icon);
	}
	else
	{
		c.setFillColor(sf::Color::Transparent);
		c.setOutlineColor(sf::Color::White);
		c.setOutlineThickness(-2.f);
	}

	c.setScale(scale, scale);

	window->draw(c);
}

bool Button::Init()
{
	return false;
}

void Button::SetPosition(sf::Vector2f _pos)
{
	pos = _pos;
}

void Button::SetTexture(std::string _path)
{
	icon = &Mnemosyne::GetTexture(_path);
}

void Button::SetVisible(bool _show)
{
	shown = _show;
}

void Button::Update()
{
	float target = (shown ? 1.f : 0.f);
	if (shown && state == Button::State::Down) target = .87f;

	if ((shown || scale > 0.f) && scale != target)
	{
		if (scalemode == ScaleMode::Linear)
		{
			float change = (target > scale ? scalespeed : -scalespeed);
			if (abs(scale - target) < abs(change))
			{
				scale = target;
			}
			else
			{
				scale += change;
			}
		}
		else if (scalemode == ScaleMode::Quadratic)
		{
			scale += (target - scale) * scalespeed;
			if (abs(scale - target) < .01f) scale = target;
		}
	}


	if (held)
	{
		if (state == Button::State::Up || state == Button::State::Released)
		{
			state = Button::State::Clicked;
			//cout << "clicked" << endl;
		}
		else if (state == Button::State::Clicked)
		{
			state = Button::State::Down;
			//cout << "down" << endl;
		}
	}
	else
	{
		if (state == Button::State::Down || state == Button::State::Clicked)
		{
			state = Button::State::Released;
			//cout << "released" << endl;
		}
		else if (state == Button::State::Released)
		{
			state = Button::State::Up;
			//cout << "up" << endl;
		}
	}
	held = false;
}

bool Button::Visible()
{
	return (shown || scale > 0.f);
}


void ButtonGroup::SetVisible(bool _show)
{
	for (int i = 0; i < buttons.size(); ++i)
	{
		buttons[i].SetVisible(_show);
	}
}

void ButtonGroup::Update()
{
	for (int i = 0; i < buttons.size(); ++i)
	{
		buttons[i].Update();
	}
}
