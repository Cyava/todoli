#include "Task.hpp"


std::map<int, std::map<int, std::vector<int>>> Task::color_map;

sf::Vector2f Task::winscale(0, 0);



Task::Task()
{
	status = Task::Status::Uninitialized;
	Init();
}

bool Task::Init()
{
	uid = uuid::generate_uuid_v4();
	status = Task::Status::None;
	statusTarget = Task::Status::None;

	notes.clear();


	time(&date_created);


	color.speed = .12f;
	color.mode = FadeColor::Mode::Quadratic;

	bgcolor.speed = .12f;
	bgcolor.mode = FadeColor::Mode::Quadratic;
	
	return true;
}


void Task::InitializeColorMap()
{
	/* ZERO PRIORITY */
	color_map[0][0] = { 100, 0, 120 };
	color_map[0][1] = { 180, 0, 255 };
	color_map[0][2] = { 120, 0, 150 };
	color_map[0][3] = { 40, 32, 42 };

	/* RED PRIORITY */
	color_map[1][0] = { 120, 0, 0 };
	color_map[1][1] = { 255, 0, 32 };
	color_map[1][2] = { 150, 0, 0 };
	color_map[1][3] = { 44, 38, 38 };

	/* YELLOW PRIORITY */
	color_map[2][0] = { 120, 120, 0 };
	color_map[2][1] = { 255, 255, 0 };
	color_map[2][2] = { 150, 140, 0 };
	color_map[2][3] = { 42, 41, 32 };

	/* GREEN PRIORITY */
	color_map[3][0] = { 0, 110, 0 };
	color_map[3][1] = { 48, 255, 0 };
	color_map[3][2] = { 0, 150, 0 };
	color_map[3][3] = { 36, 40, 36 };

	/* BLUE PRIORITY */
	color_map[4][0] = { 0, 40, 120 };
	color_map[4][1] = { 0, 120, 255 };
	color_map[4][2] = { 0, 80, 150 };
	color_map[4][3] = { 40, 44, 48 };

	/* NO PRIORITY */
	color_map[5][0] = { 160, 160, 160 };
	color_map[5][1] = { 255, 255, 255 };
	color_map[5][2] = { 180, 180, 180 };
	color_map[5][3] = { 44, 44, 44 };
}


bool Task::SetCategory(std::string _category)
{
	category = _category;

	return true;
}

bool Task::SetClient(std::string _client)
{
	client = _client;

	return true;
}

bool Task::SetDescription(std::string _desc)
{
	description = _desc;

	return true;
}
bool Task::SetName(std::string _name)
{
	name = _name;

	return true;
}

bool Task::SetPriority(unsigned int _prio)
{
	priority = _prio;

	return true;
}

bool Task::SetStatus(Task::Status _status)
{
	if (_status == status) return false;

	statusTarget = _status;

	if (statusTarget == Task::Status::Complete)
	{
		time(&date_closed);
	}
	else
	{
		if (status == Task::Status::Complete)
		{
			date_closed = -1;
		}
	}

	return true;
}

bool Task::SetTags(std::string _tags)
{
	std::vector<std::string> newtags = ToList(_tags, ", ");
	tags.clear();
	tags.add(newtags);

	return true;
}


void Task::SetupButtons()
{
	buttons.buttons.clear();

	switch (status)
	{
		case Task::Status::Open:
		{
			buttons.buttons.push_back(Button("task-complete", "confirm"));
			buttons.buttons.push_back(Button("task-edit", "edit"));
			buttons.buttons.push_back(Button("task-remove", "cancel"));

			break;
		}
		case Task::Status::Canceled:
		{
			buttons.buttons.push_back(Button("task-reopen"));
			buttons.buttons.push_back(Button("task-edit", "edit"));

			break;
		}
		case Task::Status::Complete:
		{
			buttons.buttons.push_back(Button("task-reopen"));
			buttons.buttons.push_back(Button("task-edit", "edit"));

			break;
		}
		default:
		{
			buttons.buttons.push_back(Button("task-edit"));

			break;
		}
	}
}

bool Task::StatusUpdate()
{
	if (statusTarget != status)
	{
		status = statusTarget;
		SetupButtons();

		return true;
	}

	return false;
}


void Task::ListRender(sf::RenderWindow* window, float x, float y, float w, MouseState* mouse)
{
	sf::Vector2f winmeasure(window->getSize().x / winscale.x, window->getSize().y / winscale.y);


	int hoverthicc = 10;
	float colorbar_w = 4 + (hovertime * hoverthicc);

	sf::RectangleShape rekt(sf::Vector2f(w, Height()));
	rekt.setPosition(x, y);
	rekt.setFillColor(bgcolor.Color());
	window->draw(rekt);

	rekt.setSize(sf::Vector2f(colorbar_w, Height()));
	rekt.setFillColor(color.Color());
	window->draw(rekt);


	int nameshift = 12 + (hovertime * hoverthicc);
	unsigned int taskname_size = 24;

	sf::Text taskname;
	taskname.setFont(Mnemosyne::GetFont("exo2"));
	taskname.setCharacterSize(taskname_size);
	taskname.setString(name);
	taskname.setPosition(x + nameshift + 4, y + 4);


	int taghover_top = TextInfo::TextTop(taskname) - 5;
	int taghover_bot = taghover_top + TextInfo::StandardTextHeight(taskname) + 9;
	int taghover_left = x + nameshift;
	int taghover_right = x + nameshift + 8 + taskname.getGlobalBounds().width;
	if (mouse->gpos.x >= taghover_left && mouse->gpos.x <= taghover_right && mouse->gpos.y >= taghover_top && mouse->gpos.y <= taghover_bot)
	{
		tagdisplay_x = mouse->gpos.x + (12 * winscale.x);
		if (tagdisplay < 1.f)
		{
			tagdisplay += (1.f - tagdisplay) * .12f;
			if (tagdisplay > .99f) tagdisplay = 1.f;
		}
	}
	else if (tagdisplay > 0.f)
	{
		tagdisplay *= .84f;
		if (tagdisplay < .01f) tagdisplay = 0.f;
	}

	if (tagdisplay > 0.f)
	{
		tagdisplay_y = (taghover_top + taghover_bot) / 2;

		sf::Color taghovercol = sf::Color(255, 255, 255, tagdisplay * 32);

		sf::RectangleShape tagRekt(sf::Vector2f(taghover_right - taghover_left, taghover_bot - taghover_top));
		tagRekt.setPosition(taghover_left, taghover_top);
		tagRekt.setFillColor(taghovercol);
		window->draw(tagRekt);
	}


	taskname.setFillColor(sf::Color(0, 0, 0, 140));
	taskname.setPosition(x + nameshift + 3, y + 3);
	window->draw(taskname);

	taskname.setFillColor(sf::Color::White);
	taskname.setPosition(x + nameshift + 4, y + 4);
	window->draw(taskname);

#if 0
	{
		int d_ty = taskname.getGlobalBounds().top;
		int d_by = taskname.getGlobalBounds().top + taskname.getGlobalBounds().height;

		if (tagdisplay > 0)
		{
			/* THIS CAN GET THE HEIGHT LESS THE "BELOW THE LINE" TAILS ON LETTERS */
			/* font_size - (text_top - drawn_y) = text_height (without tails) */

			//std::cout << taskname.getCharacterSize() << " (" << TextInfo::TextHeight(taskname) << " + " << (TextInfo::TextTop(taskname) - (y + 4)) << ")" << std::endl;
			std::cout << taskname.getCharacterSize() << " (" << TextInfo::TextHeight(taskname) << " + " << TextInfo::TextHeadspace(taskname) << ")" << std::endl;
		}

		sf::RectangleShape debuggy(sf::Vector2f(w, d_ty - (y + 4)));
		debuggy.setPosition(0, y + 4);
		debuggy.setFillColor(sf::Color::Green);
		window->draw(debuggy);

		debuggy = sf::RectangleShape(sf::Vector2f(w, d_by - d_ty));
		debuggy.setPosition(0, d_ty);
		debuggy.setFillColor(sf::Color::Transparent);
		debuggy.setOutlineColor(sf::Color::Magenta);
		debuggy.setOutlineThickness(2.f);
		window->draw(debuggy);
	}
#endif


	std::string dc_str = splitsexy(datestr(date_created), " ")[0];

	unsigned int taskdate_size = 14;
	sf::Text taskdate;
	taskdate.setFont(Mnemosyne::GetFont("exo2"));
	taskdate.setCharacterSize(taskdate_size);
	if (date_closed != -1)
	{
		dc_str = splitsexy(datestr(date_closed), " ")[0];

		std::chrono::system_clock::time_point t2 = std::chrono::system_clock::from_time_t(date_closed);
		std::chrono::system_clock::time_point t1 = std::chrono::system_clock::from_time_t(date_created);
		long long dt = std::chrono::milliseconds(date_closed - date_created).count();
		int daydiff = (dt / 86400);

		taskdate.setString("Closed: " + dc_str + " (" + std::to_string(daydiff) + " days)");
	}
	else
	{
		taskdate.setString("Created: " + dc_str);
	}

	taskdate.setFillColor(sf::Color(0, 0, 0, 140));
	taskdate.setPosition(x + nameshift + 5, y + height / 2 + 4);
	window->draw(taskdate);

	taskdate.setFillColor(sf::Color(150, 150, 150, 222));
	taskdate.setPosition(x + nameshift + 6, y + height / 2 + 5);
	window->draw(taskdate);


	if (client.length() > 0)
	{
		float clientshift = 0.f;
		if (expansion > 0 && buttons.buttons.size() > 0)
		{
			float mshifter = (((buttons.buttons[0].radius * 2) + 6) * buttons.buttons.size()) + 12;
			clientshift = mshifter * expansion;
		}

		taskdate.setString("Client: " + client);

		taskdate.setFillColor(sf::Color(0, 0, 0, 140));
		taskdate.setPosition(x + (nameshift / 2) + w - 21 - clientshift - taskdate.getLocalBounds().width, y + height / 2 + 4);
		window->draw(taskdate);

		taskdate.setFillColor(sf::Color(150, 150, 150, 222));
		taskdate.setPosition(x + (nameshift / 2) + w - 20 - clientshift - taskdate.getLocalBounds().width, y + height / 2 + 5);
		window->draw(taskdate);
	}


	sf::RectangleShape separator(sf::Vector2f(w, 1));
	//separator.setFillColor(sf::Color(255, 255, 255, 150));
	separator.setFillColor(sf::Color(0, 0, 0, 180));
	separator.setPosition(x, y + Height() - 1);
	window->draw(separator);


	for (int i = 0; i < buttons.buttons.size(); ++i)
	{
		buttons.buttons[i].SetPosition(sf::Vector2f(x + w + 4 - (((buttons.buttons[i].radius * 2) + 6) * (buttons.buttons.size() - i)), y + (height / 2)));
		buttons.buttons[i].Draw(window);
	}


	if (expansion > 0 && (description.size() > 0 || notes.size() > 0))
	{
		sf::View v = window->getView();

		float vx = x / winmeasure.x;
		float vy = y / winmeasure.y;
		float vw = w / winmeasure.x;
		float vh = Height() / winmeasure.y;
		
		sf::View taskview(sf::FloatRect(x, y, w, Height()));
		taskview.setViewport(sf::FloatRect(vx, vy, vw, vh));
		window->setView(taskview);

		expansion_height = 0;
		int ypoint = height;

		if (description.size() > 0)
		{
			separator = sf::RectangleShape(sf::Vector2f(w - colorbar_w - 24, 1));
			separator.setFillColor(sf::Color(255, 255, 255, 120));
			separator.setPosition(x + colorbar_w + 12, y + ypoint);
			window->draw(separator);


			unsigned int descFontSize = 12;
			sf::Text desctext;
			desctext.setFont(Mnemosyne::GetFont("exo2"));
			desctext.setCharacterSize(descFontSize);

			std::vector<std::string> desclines;
			std::string subdesc = description;
			size_t subi = 0;
			size_t last_space = 0;
			int maxdescwide = w - 20 - hoverthicc - 4;
			int cwide = 0;
			for (;;)
			{
				if (subi >= subdesc.size())
				{
					desclines.push_back(subdesc);
					break;
				}

				if (subdesc[subi] == '\n')
				{
					if (subi == 0)
					{
						desclines.push_back("");
						subdesc = subdesc.substr(1);
						subi = 0;
						cwide = 0;
						last_space = 0;

						continue;
					}

					desclines.push_back(subdesc.substr(0, subi));
					subdesc = subdesc.substr(subi + 1);
					subi = 0;
					cwide = 0;
					last_space = 0;

					continue;
				}
#if 0
				else if (subdesc[subi] == '<')
				{
					++subi;
					continue;
				}
#endif

				if (subdesc[subi] == ' ') last_space = subi;
				desctext.setString(subdesc.substr(subi, 1));
				cwide += desctext.findCharacterPos(1).x;

				//desctext.setString(subdesc.substr(0, subi));
				//std::cout << "width: " << cwide << " / " << desctext.findCharacterPos(subi).x << std::endl;

				if (cwide > maxdescwide)
				{
					if (last_space > 0)
					{
						subi = last_space;
					}
					else
					{
						//--subi;
					}
					desclines.push_back(subdesc.substr(0, subi));
					subdesc = subdesc.substr(subi + 1);
					subi = 0;
					cwide = 0;
					last_space = 0;

					continue;
				}

				++subi;
			}
			expansion_height += ((descFontSize + 3) * desclines.size()) + 13;

			for (int i = 0; i < desclines.size(); ++i)
			{
				desctext.setString(desclines[i]);
				desctext.setPosition(x + colorbar_w + 8, y + ypoint + 6 + ((descFontSize + 3) * i));
				window->draw(desctext);
			}
			//desctext.setString(description);
			//desctext.setPosition(x + colorbar_w + 8, y + height + 6);
			//window->draw(desctext);

			//v.setViewport(sf::FloatRect(0.f, 0.f, 1.f, 1.f));

			ypoint += expansion_height;
		}

		if (notes.size() > 0)
		{
			separator = sf::RectangleShape(sf::Vector2f(w - colorbar_w - 24, 1));
			separator.setFillColor(sf::Color(255, 255, 255, 120));
			separator.setPosition(x + colorbar_w + 12, y + ypoint);
			window->draw(separator);

			unsigned int noteFontSize = 11;
			sf::Text notetext;
			notetext.setFillColor(sf::Color(120, 150, 190, 255));
			notetext.setFont(Mnemosyne::GetFont("exo2"));
			notetext.setCharacterSize(noteFontSize);

			int maxnotewidth = w - 36 - hoverthicc - 4;
			int linespacing = 8;

			for (int i = 0; i < notes.size(); ++i)
			{
				std::vector<std::string> notelines = TextInfo::StringToLines(notes[i], "exo2", 12, maxnotewidth);

				for (int j = 0; j < notelines.size(); ++j)
				{
					notetext.setString(notelines[j]);
					notetext.setPosition(x + colorbar_w + 8, y + ypoint + 6 + ((noteFontSize + 3) * j));
					window->draw(notetext);
				}

				expansion_height += ((noteFontSize + 3) * notelines.size()) + (i < notes.size() - 1 ? (linespacing + 6) : 0);
				ypoint += ((noteFontSize + 3) * notelines.size()) + (i < notes.size() - 1 ? linespacing : 0);
			}
			//expansion_height += (6 * (notes.size() - 1));
		}

		window->setView(v);
	}
}

std::string Task::Serialize()
{
	std::string serstr = "{";

	serstr += "name: \"" + SanitizeString(name) + "\"";
	serstr += ", ";
	serstr += "category: \"" + SanitizeString(category) + "\"";
	serstr += ", ";
	serstr += "status: " + SanitizeString(std::to_string((int)status));
	serstr += ", ";
	serstr += "priority: " + SanitizeString(std::to_string((int)priority));
	serstr += ", ";
	serstr += "client: \"" + SanitizeString(client) + "\"";
	serstr += ", ";
	serstr += "date-created: " + SanitizeString(std::to_string(date_created));
	serstr += ", ";
	serstr += "date-closed: " + SanitizeString(std::to_string(date_closed));
	serstr += ", ";
	serstr += "description: \"" + SanitizeString(description) + "\"";
	serstr += ", ";
	serstr += "notes: {";
	for (int i = 0; i < notes.size(); ++i)
	{
		if (i > 0) serstr += ", ";
		serstr += "\"" + SanitizeString(notes[i]) + "\"";
	}
	serstr += "}";
	serstr += ", ";
	serstr += "tags: {";
	for (int i = 0; i < tags.size(); ++i)
	{
		if (i > 0) serstr += ", ";
		serstr += "\"" + SanitizeString(tags[i]) + "\"";
	}
	serstr += "}";

	serstr += "}";

	return serstr;
}

void Task::Update()
{
	/* COLOR BAR CHANGE */
	if (renderstate == RenderState::None)
	{
		color.SetRGB(color_map[priority][0]);
		bgcolor.SetRGB(32, 32, 32);

		if (hovertime > 0.f)
		{
			hovertime -= .1f;
			if (hovertime < 0.f) hovertime = 0.f;
		}
	}
	else if (renderstate == RenderState::Hovered)
	{
		color.SetRGB(color_map[priority][1]);
		bgcolor.SetRGB(32, 32, 32);
		
		if (hovertime < 1.f)
		{
			hovertime += .1f;
			if (hovertime > 1.f) hovertime = 1.f;
		}
		else if (hovertime > 1.f)
		{
			hovertime -= .1f;
			if (hovertime < 1.f) hovertime = 1.f;
		}
	}
	else if (renderstate == RenderState::Selected)
	{
		color.SetRGB(color_map[priority][2]);
		bgcolor.SetRGB(color_map[priority][3]);

		if (hovertime < 1.f)
		{
			hovertime += .1f;
			if (hovertime > 1.f) hovertime = 1.f;
		}
	}

	/* EXPANSION */
	float expand_speed = .1f;
	is_expanding = false;
	has_expanded = false;

	if (renderstate == RenderState::Selected)
	{
		if (expansion < 1.f)
		{
			expansion += expand_speed;
			if (expansion > 1.f)
			{
				expansion = 1.f;
				has_expanded = true;
			}

			is_expanding = true;
		}

		buttons.SetVisible(true);
	}
	else
	{
		if (expansion > 0.f)
		{
			expansion -= expand_speed;
			if (expansion < 0.f) expansion = 0.f;
		}

		buttons.SetVisible(false);
	}

	if (description.size() <= 0) expansion_height = 0;


	color.Update();
	bgcolor.Update();

	buttons.Update();
}


Task Task::Create(std::map<std::string, std::string> vars)
{
	Task t;

	t.SetName(vars["name"]);
	t.SetCategory(vars["category"]);
	t.SetClient(vars["client"]);
	t.SetDescription(vars["description"]);
	t.SetPriority(intval(vars["priority"]));
	t.date_created = intval(vars["date-created"]);
	t.date_closed = intval(vars["date-closed"]);
	t.status = (Task::Status)intval(vars["status"]);

	std::vector<std::string> notes = ToList(ExtractSexyBraces(vars["notes"]), ",");
	if (notes.size() > 0)
	{
		std::cout << t.name << std::endl;
		for (int i = 0; i < notes.size(); ++i)
		{
			if (notes[i][0] == '"' && notes[i][notes[i].size() - 1] == '"') notes[i] = notes[i].substr(1, notes[i].size() - 2);
			notes[i] = UnescapeString(notes[i]);
			std::cout << "\t[" << i << "] " << notes[i] << std::endl;

			t.notes.push_back(notes[i]);
		}
	}

	std::vector<std::string> tags = ToList(ExtractSexyBraces(vars["tags"]), ",");
	if (tags.size() > 0)
	{
		std::cout << t.name << std::endl;
		for (int i = 0; i < tags.size(); ++i)
		{
			if (tags[i][0] == '"' && tags[i][tags[i].size() - 1] == '"') tags[i] = tags[i].substr(1, tags[i].size() - 2);
			tags[i] = UnescapeString(tags[i]);
			std::cout << "\t[" << i << "] " << tags[i] << std::endl;

			t.tags.push_back(tags[i]);
		}
	}

	t.statusTarget = t.status;


#if 0
	/* ### TEMPORARY ### */
	std::string fixedname = t.name;
	size_t spos = fixedname.find_first_of('[');
	size_t epos = fixedname.find_first_of(']', spos);
	while (spos != std::string::npos && epos != std::string::npos)
	{
		std::string taggy = fixedname.substr(spos + 1, epos - spos - 1);
		std::cout << "Tag found: '" << taggy << "'" << std::endl;
		t.tags.push_back(taggy);
		fixedname = fixedname.substr(0, spos) + fixedname.substr(epos + 1);

		spos = fixedname.find_first_of('[');
		epos = fixedname.find_first_of(']', spos);
	}
	Trim(fixedname);
	t.name = fixedname;
#endif


	return t;
}

std::string Task::StatusString(Task::Status status)
{
	switch (status)
	{
		case Task::Status::Uninitialized:
		{
			return "Uninitialized";
		}
		case Task::Status::None:
		{
			return "None";
		}
		case Task::Status::Open:
		{
			return "Open";
		}
		case Task::Status::Canceled:
		{
			return "Canceled";
		}
		case Task::Status::Complete:
		{
			return "Complete";
		}
	}

	return "Unknown";
}

