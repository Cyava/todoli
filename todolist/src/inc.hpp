// Header file to grab all common includes
#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <stdio.h>
#include <sstream>
#include <vector>



#define DTOR .017453f
#define RTOD 57.29578f



enum class NumericOperation { None, Add, Subtract, Multiply, Divide };


template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}


template <class T>
class unique_vector
{
private:
	std::vector<T> elements;

public:
	unique_vector<T>() {};
	unique_vector<T>(size_t count)
	{
		elements = std::vector<T>(count);
	}
	unique_vector<T>(const std::vector<T>& list)
	{
		elements.clear();
		for (T val : list)
		{
			push_back(val);
		}
	}

	void add(const std::vector<T>& list)
	{
		for (T val : list)
		{
			push_back(val);
		}
	}

	size_t find(const T& val)
	{
		for (size_t i = 0; i < elements.size(); i++)
		{
			if (elements[i] == val) return i;
		}
		return -1;
	}

	bool exists(const T& val) { return (find(val) != -1); }

	//void remove(size_t n) { elements.erase(elements.begin() + n); }
	void remove(size_t index)
	{
		// ensure that we're not attempting to access out of the bounds of the container.
		//assert(index < elements.size());

		//Swap the element with the back element, except in the case when we're the last element.
		if (index + 1 != elements.size()) std::swap(elements[index], elements.back());

		//Pop the back of the container, deleting our old element.
		elements.pop_back();
	}

	void remove(const T& val) { remove(find(val)); }

	/* std::vector functions */
	typename std::vector<T>::iterator begin() { return elements.begin(); }
	typename std::vector<T>::iterator end() { return elements.end(); }
	typename std::vector<T>::reverse_iterator rbegin() { return elements.rbegin(); }
	typename std::vector<T>::reverse_iterator rend() { return elements.rend(); }

	void pop_back() { return elements.pop_back(); }
	void push_back(const T& val)
	{
		if (exists(val)) return;

		return elements.push_back(val);
	}

	void clear() { elements.clear(); }
	size_t size() { return elements.size(); }

	T& operator[] (size_t n) { return elements[n]; }
	const T& operator[] (size_t n) const { return elements[n]; }
};


class RollingAverage
{
private:
	float avg = 0.f;
	std::vector<float> vars;

	int size = 1;
	int roller = 0;

public:
	RollingAverage() {}
	RollingAverage(int _size)
	{
		size = _size;
	}

	void Add(float);

	float Get() { return avg; }
};


struct Point
{
	float x;
	float y;

	Point() { x = 0; y = 0; }
	Point(float _x, float _y) { x = _x; y = _y; }


	bool operator == (const Point& a) { return (a.x == x && a.y == y); }

	Point operator + (const Point& p) { return Point(x + p.x, y + p.y); }
};


namespace esp
{
	struct Rectangle
	{
		float centerX = 0.f;
		float centerY = 0.f;
		float width = 0.f;
		float height = 0.f;

		float extra_data = 0.f;

		Rectangle() {}
		Rectangle(float _x, float _y, float _w, float _h)
		{
			centerX = _x;
			centerY = _y;
			width = _w;
			height = _h;
		}
	};
};


static std::vector<std::string> split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

extern std::vector<std::string> splitsexy(std::string s, std::string delimiter);


static inline void _ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

static inline void _rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

static inline void Trim(std::string& s) {
	_ltrim(s);
	_rtrim(s);
}


static std::string Lower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

static std::string Upper(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}


static NumericOperation GetOperation(std::string s)
{
	switch (s[0])
	{
	case '+':
		return NumericOperation::Add;
	case '-':
		return NumericOperation::Subtract;
	case '*':
	case 'x':
		return NumericOperation::Multiply;
	case '/':
		return NumericOperation::Divide;
	default:
		return NumericOperation::None;
	}
}


static bool boolval(std::string s)
{
	if (s == "") return false;

	try
	{
		Trim(s);
		s = Lower(s);

		std::istringstream is(s);
		bool b;
		is >> std::boolalpha >> b;
		return b;
	}
	catch (...)
	{
		return false;
	}
}

static int intval(std::string s)
{
	try
	{
		Trim(s);
		return stoi(s);
	}
	catch (...)
	{
		return 0;
	}
}

static float floatval(std::string s)
{
	try
	{
		Trim(s);
		return stof(s);
	}
	catch (...)
	{
		return 0.f;
	}
}

extern void clamp(int& i, int _min, int _max);

extern float AngleBetween(Point p1, Point p2);
extern float AngleCompare(float, float);
extern int ClosestAngularDirection(float, float);

extern bool PointOnSegment(Point, Point, Point);
extern int LineOrientation(Point, Point, Point);
extern bool LinesIntersect(Point, Point, Point, Point);

extern float DistanceBetween(Point, Point);


static std::string FormattedString(unsigned long l)
{
	/*
	std::string numWithCommas = std::to_string(l);
	int insertPosition = numWithCommas.length() - 3;
	while (insertPosition > 0) {
		numWithCommas.insert(insertPosition, ",");
		insertPosition -= 3;
	}
	*/
	std::string s = std::to_string(l);
	if (s.size() > 5) s = s.substr(0, s.size() - 3) + "K";

	return s;
}


namespace uuid {
	static std::random_device              rd;
	static std::mt19937                    gen(rd());
	static std::uniform_int_distribution<> dis(0, 15);
	static std::uniform_int_distribution<> dis2(8, 11);

	std::string generate_uuid_v4();
}


extern std::string ExtractParens(const std::string);
extern std::string ExtractSexyBraces(const std::string);

extern std::vector<std::string> ToList(const std::string str, std::string sep = ",");


static std::string datestr(time_t rawtime)
{
	struct tm timeinfo;
	char buffer[80];

	localtime_s(&timeinfo, &rawtime);

	strftime(buffer, sizeof(buffer), "%m/%d/%Y %H:%M:%S", &timeinfo);
	return std::string(buffer);
}

static time_t dateint(std::string date)
{
	struct tm tm;
	std::istringstream iss(date + " 00:00:00");
	iss >> std::get_time(&tm, "%m/%d/%Y %H:%M:%S");
	time_t time = mktime(&tm);

	return time;
}


extern std::string SanitizeString(std::string);
extern std::string UnescapeString(std::string);