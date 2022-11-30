#include "inc.hpp"


using namespace std;


void RollingAverage::Add(float n)
{
	if (vars.size() < size)
	{
		vars.push_back(n);

		avg = 0;
		for (int i = 0; i < vars.size(); i++)
		{
			avg += vars[i];
		}
		avg /= vars.size();
	}
	else
	{
		vars[roller] = n;
		roller = (++roller) % size;

		avg = 0;
		for (int i = 0; i < vars.size(); i++)
		{
			avg += vars[i];
		}
		avg /= vars.size();
	}
}


std::vector<std::string> splitsexy(std::string s, std::string delimiter)
{
	std::vector<std::string> tokens;
	bool quoted = false;

	while (s.length() > 0)
	{
		int nest = 0;
		for (int i = 0; i < s.length(); i++)
		{
			if (s[i] == '"')
			{
				quoted = !quoted;
			}
			else if (s[i] == '{')
			{
				nest++;
			}
			else if (s[i] == '}' && nest > 0)
			{
				nest--;
			}
			else if (s[i] == '\\')
			{
				++i;
				continue;
			}

			if (!quoted && nest == 0 && s.substr(i, delimiter.length()) == delimiter)
			{
				tokens.push_back(s.substr(0, i));
				s = s.substr(i + delimiter.length());
				break;
			}

			if (i >= s.length() - 1)
			{
				tokens.push_back(s);
				s = "";
				break;
			}
		}
	}
	return tokens;
}


void clamp(int& i, int _min, int _max)
{
	if (i < _min) i = _min;
	if (i > _max) i = _max;
}

float AngleBetween(Point p1, Point p2)
{
	if (p1.y == p2.y) return (p2.x > p1.x ? 0.f : 180.f);

	return RTOD * atan2f(p2.y - p1.y, p2.x - p1.x);
}

int ClosestAngularDirection(float a1, float a2)
{
	float k = AngleCompare(a1, a2);

	if (k == 0) return 0;
	if (k < 0) return -1;
	return 1;
}

float AngleCompare(float a1, float a2)
{
	a1 *= DTOR;
	a2 *= DTOR;
	return (atan2(sin(a2 - a1), cos(a2 - a1)) * RTOD);
}

bool PointOnSegment(Point p, Point q, Point r)
{
	if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) && q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y)) return true;
	return false;
}

int LineOrientation(Point p, Point q, Point r)
{
	int val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

	if (val == 0) return 0;

	return (val > 0) ? 1 : 2;
}

bool LinesIntersect(Point p1, Point q1, Point p2, Point q2)
{
	int o1 = LineOrientation(p1, q1, p2);
	int o2 = LineOrientation(p1, q1, q2);
	int o3 = LineOrientation(p2, q2, p1);
	int o4 = LineOrientation(p2, q2, q1);

	if (o1 != o2 && o3 != o4) return true;

	if (o1 == 0 && PointOnSegment(p1, p2, q1)) return true;
	if (o2 == 0 && PointOnSegment(p1, q2, q1)) return true;
	if (o3 == 0 && PointOnSegment(p2, p1, q2)) return true;
	if (o4 == 0 && PointOnSegment(p2, q1, q2)) return true;

	return false;
}


float DistanceBetween(Point a, Point b)
{
	return sqrtf(((b.y - a.y) * (b.y - a.y)) + ((b.x - a.x) * (b.x - a.x)));
}


std::string ExtractParens(const std::string str)
{
	int nest = 0;
	int sidx = 0;
	int eidx = std::string::npos;
	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] == '(')
		{
			++nest;
			sidx = i + 1;
		}
		else if (str[i] == ')' && nest > 0)
		{
			--nest;
			if (nest == 0) return str.substr(sidx, i - sidx);
		}
	}
	return str;
}


std::string ExtractSexyBraces(const std::string str)
{
	int sidx = str.find_first_of("{");
	int eidx = str.find_last_of("}");

	if (sidx == std::string::npos || eidx == std::string::npos || eidx < sidx) return "";
	return str.substr(sidx + 1, eidx - sidx - 1);
}


std::vector<std::string> ToList(const std::string str, std::string sep)
{
	std::vector<std::string> ret = splitsexy(str, sep);
	for (int i = 0; i < ret.size(); i++)
	{
		Trim(ret[i]);
	}
	return ret;
}


std::string uuid::generate_uuid_v4()
{
	std::stringstream ss;
	int i;
	ss << std::hex;
	for (i = 0; i < 8; i++) {
		ss << dis(gen);
	}
	ss << "-";
	for (i = 0; i < 4; i++) {
		ss << dis(gen);
	}
	ss << "-4";
	for (i = 0; i < 3; i++) {
		ss << dis(gen);
	}
	ss << "-";
	ss << dis2(gen);
	for (i = 0; i < 3; i++) {
		ss << dis(gen);
	}
	ss << "-";
	for (i = 0; i < 12; i++) {
		ss << dis(gen);
	};
	return ss.str();
}


std::string SanitizeString(std::string line)
{
	std::string ttos = "";
	for (int i = 0; i < line.size(); ++i)
	{
		if (line[i] == '\n')
		{
			ttos += "\\n";
		}
		else if (line[i] == '"')
		{
			ttos += "\\\"";
		}
		else
		{
			ttos += line[i];
		}
	}

	return ttos;
}

std::string UnescapeString(std::string str)
{
	std::string newval;
	for (size_t j = 0; j < str.size(); ++j)
	{
		if (str[j] == '\\')
		{
			if (str[j + 1] == 'n')
			{
				++j;
				newval += "\n";
				continue;
			}
		}
		else
		{
			newval += str[j];
		}
	}
	return newval;
}