#pragma once


#include "inc.hpp"
#include "TextInfo.hpp"

#include "Button.hpp"
#include "FadeColor.hpp"
#include "Mnemosyne.hpp"
#include "MouseState.hpp"

#include "wtypes.h"


struct Task
{
	enum class Status { Uninitialized, None, Open, Canceled, Complete };
	enum class RenderState { None, Hovered, Selected };


	/* IMPORTANT TASK FIELDS */
	std::string uid;

	std::string name;
	std::string category;
	std::string description;

	Status status;
	Status statusTarget;

	std::vector<std::string> notes;
	unique_vector<std::string> tags;

	std::string client;

	unsigned int priority = 0;

	time_t date_created;
	time_t date_closed = -1;


	/* RENDERING FIELDS */
	float height = 64.f;

	float hovertime = 0.f;
	float expansion = 0.f;
	int expansion_height = 64;
	bool is_expanding = false;
	bool has_expanded = false;

	bool hidden_by_filter = false;

	ButtonGroup buttons;

	RenderState renderstate = RenderState::None;

	FadeColor color;
	FadeColor bgcolor;

	static std::map<int, std::map<int, std::vector<int>>> color_map;

	static sf::Vector2f winscale;

	float tagdisplay = 0.f;
	float tagdisplay_x = 0.f;
	float tagdisplay_y = 0.f;


	Task();

	std::string GetClient() { return client; }
	std::string GetDescription() { return description; }
	std::string GetName() { return name; }
	unsigned int GetPriority() { return priority; }
	std::string GetUID() { return uid; }

	static void InitializeColorMap();

	float Height() { return height + (expansion * expansion_height); }

	bool Init();
	
	bool SetCategory(std::string);
	bool SetClient(std::string);
	bool SetDescription(std::string);
	bool SetName(std::string);
	bool SetPriority(unsigned int);
	bool SetStatus(Task::Status);
	bool SetTags(std::string);

	void SetupButtons();
	bool StatusUpdate();

	void ListRender(sf::RenderWindow*, float, float, float, MouseState*);

	std::string Serialize();

	void Update();


	static Task Create(std::map<std::string, std::string>);

	static std::string StatusString(Task::Status);


	bool operator < (const Task& a) const { if (priority == a.priority) return (date_created < a.date_created); return (priority < a.priority); }
};