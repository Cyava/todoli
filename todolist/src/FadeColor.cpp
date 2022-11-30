#include "FadeColor.hpp"


FadeColor::FadeColor(int r, int g, int b, int a)
{
	SetRGB(r, g, b, a);
}


sf::Color FadeColor::Color()
{
	return sf::Color((int)rgba[0], (int)rgba[1], (int)rgba[2], (int)rgba[3]);
}

bool FadeColor::SetRGB(int r, int g, int b, int a)
{
	if (r == targets[0] && g == targets[1] && b == targets[2] && a == targets[3]) return false;

	clamp(r, 0, 255);
	clamp(g, 0, 255);
	clamp(b, 0, 255);
	clamp(a, 0, 255);

	for (int i = 0; i < 4; ++i) { previous[i] = rgba[i]; }

	targets[0] = r;
	targets[1] = g;
	targets[2] = b;
	targets[3] = a;

	return true;
}

bool FadeColor::SetRGB(std::vector<int> _rgba)
{
	if (_rgba.size() < 4)
	{
		_rgba.resize(4, 0);
		_rgba[3] = 255;
	}

	return SetRGB(_rgba[0], _rgba[1], _rgba[2], _rgba[3]);
}

void FadeColor::Update()
{
	for (int i = 0; i < 4; ++i)
	{
		if (mode == Mode::Linear)
		{
			float change = (targets[i] - previous[i]) * speed;
			if (abs(rgba[i] - targets[i]) < abs(change))
			{
				rgba[i] = targets[i];
			}
			else
			{
				rgba[i] += change;
			}
		}
		else if (mode == Mode::Quadratic)
		{
			rgba[i] += (targets[i] - rgba[i]) * speed;
			if (abs(rgba[i] - targets[i]) < .01f) rgba[i] = targets[i];
		}
	}
}