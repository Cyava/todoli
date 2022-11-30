#include "inc.hpp"

#include "Button.hpp"
#include "Mnemosyne.hpp"
#include "Task.hpp"

#include <chrono>
#include <ctime>

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>


using namespace std;


std::vector<Task> tasks;

Task task_sel;
std::string task_seluid;


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


	void Update(sf::Vector2i _pos, sf::RenderWindow* window)
	{
		//cout << _pos.x << ", " << _pos.y << endl;

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
};
MouseState mouse;

FadeColor menubar_color(40, 40, 40);
FadeColor filterbox_hilite(40, 40, 40);
FadeColor newtask_hilite(40, 40, 40);


bool ui_focus = false;

float filterbox_target = 0.f;
float filterbox_scale = 0.f;
float filterbox_speed = .1f;
float filterbox_width = 160;
float filterbox_height = 100;
std::vector<Task::Status> filteroptions{ Task::Status::None, Task::Status::Open, Task::Status::Complete, Task::Status::Canceled };


int winsize;
float taskx_target = 0.f;
float taskx_anchor = 0.f;
float taskx = 0;


enum class WindowState { TaskList, NewTask };
WindowState winstate;


tgui::GuiSFML* gui;
tgui::Theme blackTheme;


bool AddTask(Task, bool _do_refresh = true);

void SetupNewTaskPanel();
void NewTaskOK(bool);

void SetupEditTaskPanel();
void EditTaskOK(bool);

bool UpdateTasks();

void WriteLine(std::ofstream&, std::string);

bool ReadTasks();

bool SortTasks();


float BellValue(float, int);


int main(int argc, char** argv)
{
	Mnemosyne::Init();

	Task::InitializeColorMap();
	menubar_color.mode = FadeColor::Mode::Quadratic;
	menubar_color.speed = .16f;

	filterbox_hilite.mode = FadeColor::Mode::Quadratic;
	filterbox_hilite.speed = .16f;

	newtask_hilite.mode = FadeColor::Mode::Quadratic;
	newtask_hilite.speed = .16f;


	sf::ContextSettings context;
	context.depthBits = 24;
	context.stencilBits = 8;
	context.antialiasingLevel = 8;
	context.majorVersion = 3;
	context.minorVersion = 2;

	sf::RenderWindow window(sf::VideoMode(640, 480), "TODO", sf::Style::Default, context);
	window.setVerticalSyncEnabled(true);
	sf::View view(sf::FloatRect(0, 0, window.getSize().x, window.getSize().y));
	window.setView(view);

	tgui::GuiSFML maingui{ window };
	blackTheme.load("assets/themes/black.txt");

	gui = &maingui;

	winsize = window.getSize().x;

	Task::Status shown_status = Task::Status::Open;

	ReadTasks();
	SortTasks();


	float scroll_speed = 16.f;
	float scroll_render = 0.f;
	float scroll_actual = 0.f;
	float scroll_divide = 5.f;

	float scrollbar_width = 0.f;
	FadeColor scrollbar_color(255, 120, 0);
	scrollbar_color.speed = .12f;
	scrollbar_color.mode = FadeColor::Mode::Quadratic;

	bool showscroll = false;


	while (window.isOpen())
	{
		int menubar_height = 32;

		float tasklistHeight = 0.f;
		for (int i = 0; i < tasks.size(); ++i)
		{
			taskx = taskx_anchor * winsize;

			if (shown_status != Task::Status::None && tasks[i].status != shown_status) continue;
			tasklistHeight += tasks[i].Height();
		}

		if (tasklistHeight > (window.getSize().y - menubar_height))
		{
			showscroll = true;
		}
		else
		{
			showscroll = false;
			tasklistHeight = window.getSize().y - menubar_height;
		}


		mouse.Update(sf::Mouse::getPosition(window), &window);

		sf::Event event;
		while (window.pollEvent(event))
		{
			maingui.handleEvent(event);

			switch (event.type)
			{
				case sf::Event::Closed:
				{
					window.close();
					break;
				}
				case sf::Event::Resized:
				{
					sf::View view(sf::FloatRect(0, 0, window.getSize().x, window.getSize().y));
					window.setView(view);
					winsize = window.getSize().x;
					break;
				}
				case sf::Event::KeyReleased:
				{
					if (event.key.code == sf::Keyboard::Key::F1)
					{
						if (winstate == WindowState::TaskList)
						{
							maingui.removeAllWidgets();
							winstate = WindowState::NewTask;
							
							task_sel = Task();
							task_seluid = task_sel.uid;


							SetupNewTaskPanel();
						}
					}
					else if (event.key.code == sf::Keyboard::Key::F2)
					{
						ReadTasks();
					}
				}
				case sf::Event::MouseButtonPressed:
				{
					mouse.buttons[event.mouseButton.button] = MouseState::ButtonState::Pressed;
					break;
				}
				case sf::Event::MouseButtonReleased:
				{
					mouse.buttons[event.mouseButton.button] = MouseState::ButtonState::Released;
					break;
				}
				case sf::Event::MouseWheelScrolled:
				{
					float delta = event.mouseWheelScroll.delta * scroll_speed;
					
					scroll_actual -= delta;
					if (scroll_actual < 0 || scroll_actual > tasklistHeight - window.getSize().y + menubar_height) scroll_render -= delta;
				}
			}
		}

		if (scroll_actual < 0)
		{
			scroll_actual = 0;
		}
		else if (scroll_actual > tasklistHeight - window.getSize().y + menubar_height)
		{
			scroll_actual = tasklistHeight - window.getSize().y + menubar_height;
		}

		scroll_divide = 2.f;
		if (scroll_render < 0 || scroll_render > tasklistHeight - window.getSize().y + menubar_height) scroll_divide = 3.f;


		if (winstate == WindowState::TaskList)
		{
			taskx_target = 0.f;
		}
		else
		{
			taskx_target = -1.f;
		}


		bool writetasks = false;
		for (int i = 0; i < tasks.size(); ++i)
		{
			writetasks = writetasks | tasks[i].StatusUpdate();
		}
		if (writetasks) UpdateTasks();



		window.clear();


		if (scroll_render != scroll_actual)
		{
			scroll_render += (scroll_actual - scroll_render) / scroll_divide;
			if (abs(scroll_render - scroll_actual) <= .01) scroll_render = scroll_actual;
		}
		

		Point tsize(window.getSize().x, 0);

		if (taskx_anchor != taskx_target)
		{
			float tvel = (taskx_target - taskx_anchor) * .16f;
			tvel += (tvel < 0 ? -.001f : .001f);
			taskx_anchor += tvel;
			if (abs(taskx_target - taskx_anchor) < .002f)
			{
				taskx_anchor = taskx_target;

				if (winstate == WindowState::TaskList) gui->removeAllWidgets();
			}
		}

		float line_cutoff = 0.f;

		float tasky = menubar_height - scroll_render;
		taskx = taskx_anchor * winsize;
		for (int i = 0; i < tasks.size(); ++i)
		{
			taskx = taskx_anchor * winsize;

			if (shown_status != Task::Status::None && tasks[i].status != shown_status) continue;

			if (!ui_focus)
			{
				if (mouse.gpos.x >= taskx && mouse.gpos.x < taskx + tsize.x - scrollbar_width && mouse.gpos.y >= tasky && mouse.gpos.y < tasky + tasks[i].Height())
				{
					if (mouse.buttons[0] == MouseState::ButtonState::Pressed)
					{
						int buttoned = -1;
						for (int j = 0; j < tasks[i].buttons.buttons.size(); ++j)
						{
							if (!tasks[i].buttons.buttons[j].Visible()) continue;

							Point p(tasks[i].buttons.buttons[j].pos.x, tasks[i].buttons.buttons[j].pos.y);
							if (DistanceBetween(Point(mouse.gpos.x, mouse.gpos.y), p) <= tasks[i].buttons.buttons[j].radius)
							{
								buttoned = j;
								break;
							}
						}

						if (buttoned >= 0)
						{
							mouse.grabbed_key = GRAB_KEY_TASKBUTTON;
							mouse.grabbed_id = buttoned;
							mouse.grabbed_id_sub = i;
						}
						else
						{
							mouse.grabbed_key = GRAB_KEY_TASK;
							mouse.grabbed_id = i;
							mouse.grabbed_id_sub = -1;
						}
					}
					else if (mouse.buttons[0] == MouseState::ButtonState::Released)
					{
						if (mouse.grabbed_key == GRAB_KEY_TASK && mouse.grabbed_id == i)
						{
							if (tasks[i].renderstate == Task::RenderState::Selected)
							{
								int buttoned = -1;
								for (int j = 0; j < tasks[i].buttons.buttons.size(); ++j)
								{
									if (!tasks[i].buttons.buttons[j].Visible()) continue;

									Point p(tasks[i].buttons.buttons[j].pos.x, tasks[i].buttons.buttons[j].pos.y);
									if (DistanceBetween(Point(mouse.gpos.x, mouse.gpos.y), p) <= tasks[i].buttons.buttons[j].radius)
									{
										buttoned = j;
										break;
									}
								}

								tasks[i].renderstate = Task::RenderState::None;
							}
							else
							{
								tasks[i].renderstate = (tasks[i].renderstate == Task::RenderState::Selected ? Task::RenderState::None : Task::RenderState::Selected);
							}

							for (int j = 0; j < tasks.size(); ++j)
							{
								if (j == i) continue;
								if (tasks[j].renderstate == Task::RenderState::Selected) tasks[j].renderstate = Task::RenderState::None;
							}
						}
						else if (mouse.grabbed_key == GRAB_KEY_TASKBUTTON && mouse.grabbed_id_sub == i)
						{
							if (tasks[i].renderstate == Task::RenderState::Selected)
							{
								int buttoned = -1;
								for (int j = 0; j < tasks[i].buttons.buttons.size(); ++j)
								{
									if (!tasks[i].buttons.buttons[j].Visible()) continue;

									Point p(tasks[i].buttons.buttons[j].pos.x, tasks[i].buttons.buttons[j].pos.y);
									if (DistanceBetween(Point(mouse.gpos.x, mouse.gpos.y), p) <= tasks[i].buttons.buttons[j].radius)
									{
										buttoned = j;
										break;
									}
								}

								if (buttoned == -1)
								{
									//tasks[i].renderstate = Task::RenderState::None;
								}
								else if (mouse.grabbed_id == buttoned)
								{
									//cout << "button: " << tasks[i].buttons.buttons[buttoned].id << endl;
									if (tasks[i].buttons.buttons[buttoned].id == "task-complete")
									{
										tasks[i].SetStatus(Task::Status::Complete);
									}
									else if (tasks[i].buttons.buttons[buttoned].id == "task-edit")
									{
										maingui.removeAllWidgets();
										winstate = WindowState::NewTask;

										task_sel = tasks[i];
										task_seluid = task_sel.uid;


										SetupEditTaskPanel();
									}
									else if (tasks[i].buttons.buttons[buttoned].id == "task-remove")
									{
										tasks[i].SetStatus(Task::Status::Canceled);
									}
									else if (tasks[i].buttons.buttons[buttoned].id == "task-reopen")
									{
										tasks[i].SetStatus(Task::Status::Open);
									}
								}
							}
						}
					}

					if (tasks[i].renderstate != Task::RenderState::Selected) tasks[i].renderstate = Task::RenderState::Hovered;
				}
				else
				{
					if (tasks[i].renderstate == Task::RenderState::Hovered) tasks[i].renderstate = Task::RenderState::None;
				}
			}

			tasks[i].Update();
			tasks[i].ListRender(&window, taskx, tasky, tsize.x - scrollbar_width);
			
			if (tasks[i].is_expanding)
			{
				std::cout << "i need to scroll by " << ((tasky + tasks[i].Height()) - window.getSize().y) << std::endl;
				if (tasky + tasks[i].Height() > window.getSize().y)
				{
					//std::cout << "i need to scroll by " << ((tasky + tasks[i].Height()) - window.getSize().y) << std::endl;
					scroll_actual = ((tasky + tasks[i].Height()) - window.getSize().y) + scroll_render - 1;
				}
			}

#if 0
			if (tasks[i].renderstate == Task::RenderState::Selected) line_cutoff = (tasky + tasks[i].Height());

			if (tasks[i].has_expanded)
			{
				std::cout << "i missed by " << ((tasky + tasks[i].Height()) - window.getSize().y) << std::endl;

				std::cout << "delta: " << (line_cutoff - window.getSize().y + scroll_render) << " / " << scroll_actual << std::endl;
				// FIX THIS, IT'S WRONG
			}
#endif

			tasky += tasks[i].Height();
		}

		if (showscroll || scrollbar_width > 0)
		{
			if (showscroll && scrollbar_width < 12.f)
			{
				scrollbar_width += (12.f - scrollbar_width) * .25f;
				if (scrollbar_width >= 11.9f) scrollbar_width = 12.f;
			}
			else if (!showscroll)
			{
				float spdfac = .2f;
				if (scroll_render > 1) spdfac = .06f;

				scrollbar_width -= (scrollbar_width + 2) * spdfac;
				if (scrollbar_width <= .1f) scrollbar_width = 0.f;
			}

			sf::RectangleShape scrollrekt(sf::Vector2f(scrollbar_width, window.getSize().y - menubar_height));

			scrollrekt.setPosition(window.getSize().x - scrollbar_width, menubar_height);
			scrollrekt.setFillColor(sf::Color(12, 12, 12));
			scrollrekt.setOutlineColor(sf::Color(255, 255, 255));
			scrollrekt.setOutlineThickness(1.f);
			window.draw(scrollrekt);


			float viewportHeight = window.getSize().y - menubar_height;
			float sb_scale = viewportHeight / tasklistHeight;

			float sb_y = sb_scale * scroll_render;
			float sb_h = viewportHeight * sb_scale;


			if (!ui_focus)
			{
				scrollbar_color.SetRGB(255, 120, 0);

				if (mouse.gpos.x >= winsize - scrollbar_width && mouse.gpos.x < winsize && mouse.gpos.y >= menubar_height + sb_y && mouse.gpos.y < menubar_height + sb_y + sb_h)
				{
					scrollbar_color.SetRGB(255, 180, 64);

					if (mouse.buttons[0] == MouseState::ButtonState::Pressed)
					{
						mouse.grabbed_key = GRAB_KEY_SCROLLBAR;
						mouse.grabbed_id = sb_y;
						mouse.grabbed_id_sub = mouse.gpos.y;

						ui_focus = true;
					}
				}
			}

			if (mouse.grabbed_key == GRAB_KEY_SCROLLBAR)
			{
				scrollbar_color.SetRGB(150, 72, 0);

				float drag_dist = mouse.gpos.y - mouse.grabbed_id_sub;
				sb_y = mouse.grabbed_id + drag_dist;
				scroll_actual = sb_y / sb_scale;

				if (scroll_actual < 0)
				{
					float temp = log((abs(scroll_actual) + 100) * .01f);
					scroll_render = (temp > 0 ? -temp * 100 : scroll_actual);
					sb_y = sb_scale * scroll_render;

					scroll_actual = 0;
				}
				else if (scroll_actual > tasklistHeight - window.getSize().y + menubar_height)
				{
					float endscroll = tasklistHeight - window.getSize().y + menubar_height;
					float temp = log((abs(scroll_actual - endscroll) + 100) * .01f);
					scroll_render = (temp > 0 ? (temp * 100) + endscroll : scroll_actual);
					sb_y = sb_scale * scroll_render;

					scroll_actual = tasklistHeight - window.getSize().y + menubar_height;
				}


				if (mouse.buttons[0] == MouseState::ButtonState::Released)
				{
					mouse.grabbed_key = GRAB_KEY_NONE;
					mouse.grabbed_id = -1;
					mouse.grabbed_id_sub = -1;

					ui_focus = false;
				}
			}

			scrollrekt = sf::RectangleShape(sf::Vector2f(12, sb_h));
			scrollrekt.setPosition(window.getSize().x - scrollbar_width, menubar_height + sb_y);
			scrollrekt.setFillColor(scrollbar_color.Color());
			scrollrekt.setOutlineColor(sf::Color(255, 255, 255));
			scrollrekt.setOutlineThickness(1.f);
			window.draw(scrollrekt);
		}
		scrollbar_color.Update();




		if (winstate == WindowState::NewTask || taskx_anchor < 0)
		{
			float wx = taskx + winsize;
			float wy = menubar_height;
			float ww = winsize;
			float wh = window.getSize().y - menubar_height;

			if (taskx_anchor != taskx_target)
			{
				sf::RectangleShape separator(sf::Vector2f(1, wh));
				separator.setFillColor(sf::Color::White);
				separator.setPosition(taskx + winsize - 1, menubar_height);
				window.draw(separator);
			}

			sf::VertexArray bgrekt(sf::Quads, 4);
			bgrekt[0].position = sf::Vector2f(wx, wy);
			bgrekt[1].position = sf::Vector2f(wx + ww, wy);
			bgrekt[2].position = sf::Vector2f(wx + ww, wy + wh);
			bgrekt[3].position = sf::Vector2f(wx, wy + wh);
			bgrekt[0].color = sf::Color(32, 32, 32);
			bgrekt[1].color = sf::Color(32, 32, 32);
			bgrekt[2].color = sf::Color(16, 16, 16);
			bgrekt[3].color = sf::Color(16, 16, 16);
			window.draw(bgrekt);


			maingui.get<tgui::Widget>("anchor")->setPosition(wx, wy);
		}


		maingui.draw();


		menubar_color.Update();
		filterbox_hilite.Update();
		newtask_hilite.Update();

		sf::RectangleShape menubar(sf::Vector2f(tsize.x, menubar_height));
		menubar.setPosition(0, 0);
		menubar.setFillColor(menubar_color.Color());
		window.draw(menubar);

		menubar.setFillColor(sf::Color::White);
		menubar.setSize(sf::Vector2f(tsize.x, 1.f));
		menubar.setPosition(0, menubar_height - 1);
		window.draw(menubar);

		
		/* FILTER BOX */
		float fbx = 0;
		float fby = 0;
		float fbw = 132;
		int fbsel = -1;

		float filterbar_size = (filteroptions.size() * menubar_height) + 1;
		filterbox_height = menubar_height + (filterbox_scale * filterbar_size);

		if (filterbox_scale != filterbox_target)
		{
			float change = (filterbox_target > filterbox_scale ? filterbox_speed : -filterbox_speed);
			if (abs(filterbox_scale - filterbox_target) < abs(change))
			{
				filterbox_scale = filterbox_target;
			}
			else
			{
				filterbox_scale += change;
			}
		}

		if (mouse.gpos.x >= fbx && mouse.gpos.y >= fby && mouse.gpos.x < fbx + fbw && mouse.gpos.y < fby + filterbox_height)
		{
			if (mouse.gpos.y < fby + menubar_height)
			{
				filterbox_hilite.SetRGB(64, 64, 64);
			}
			else
			{
				filterbox_hilite.SetRGB(40, 40, 40);
				fbsel = (mouse.gpos.y - menubar_height) / menubar_height;
				if (fbsel < 0 || fbsel >= filteroptions.size()) fbsel = -1;
			}

			if (mouse.buttons[0] == MouseState::ButtonState::Pressed)
			{
				mouse.grabbed_key = GRAB_KEY_MENUBAR;
				mouse.grabbed_id = 0;
				mouse.grabbed_id_sub = -1;
			}
			else if (mouse.buttons[0] == MouseState::ButtonState::Released)
			{
				if (mouse.grabbed_key == GRAB_KEY_MENUBAR && mouse.grabbed_id == 0)
				{
					if (!ui_focus && filterbox_target == 0.f)
					{
						ui_focus = true;
						filterbox_target = 1.f;
					}
					else
					{
						if (fbsel != -1)
						{
							shown_status = filteroptions[fbsel];
						}

						ui_focus = false;
						filterbox_target = 0.f;
					}
				}

				mouse.grabbed_key = GRAB_KEY_MENUBAR;
				mouse.grabbed_id = 0;
				mouse.grabbed_id_sub = -1;
			}
		}
		else
		{
			filterbox_hilite.SetRGB(40, 40, 40);

			if (mouse.buttons[0] == MouseState::ButtonState::Released && ui_focus && filterbox_target > 0.f)
			{
				ui_focus = false;
				filterbox_target = 0.f;
			}
		}

		if (filterbox_scale > 0.f)
		{
			menubar.setFillColor(sf::Color::White);
			menubar.setSize(sf::Vector2f(fbw + 2, filterbox_height - menubar_height));
			menubar.setPosition(fbx - 1, fby + menubar_height);
			window.draw(menubar);
		}


		sf::View v = window.getView();

		float vx = fbx / window.getSize().x;
		float vy = fby / window.getSize().y;
		float vw = fbw / window.getSize().x;
		float vh = filterbox_height / window.getSize().y;

		sf::View filterview(sf::FloatRect(fbx, fby, fbw, filterbox_height));
		filterview.setViewport(sf::FloatRect(vx, vy, vw, vh));
		window.setView(filterview);

		menubar.setFillColor(filterbox_hilite.Color());
		menubar.setSize(sf::Vector2f(fbw, filterbox_height - 1));
		menubar.setPosition(fbx, fby);
		window.draw(menubar);

		sf::Vector2f ftpos(10, 11);
		sf::VertexArray filtertriangle(sf::Triangles, 3);
		filtertriangle[0].position = sf::Vector2f(ftpos.x, ftpos.y);
		filtertriangle[1].position = sf::Vector2f(ftpos.x + 6, ftpos.y + 4);
		filtertriangle[2].position = sf::Vector2f(ftpos.x, ftpos.y + 8);
		filtertriangle[0].color = sf::Color(255, 255, 255);
		filtertriangle[1].color = sf::Color(255, 255, 255);
		filtertriangle[2].color = sf::Color(255, 255, 255);
		window.draw(filtertriangle);

		std::string sname = Task::StatusString(shown_status);
		if (sname == "None")
		{
			sname = "Show All";
		}
		std::string filterstring = "Filter: " + sname;
		sf::Text filtertext;
		filtertext.setFont(Mnemosyne::GetFont("exo2"));
		filtertext.setCharacterSize(14);
		filtertext.setString(filterstring);
		filtertext.setPosition(21, 6);
		window.draw(filtertext);

		if (filterbox_scale > 0.f)
		{
			filtertext.setFont(Mnemosyne::GetFont("exo2"));
			filtertext.setCharacterSize(14);

			for (int i = 0; i < filteroptions.size(); ++i)
			{
				if (i == fbsel)
				{
					menubar.setFillColor(sf::Color(64, 64, 64));
					menubar.setSize(sf::Vector2f(fbw, menubar_height));
					menubar.setPosition(fbx, fby + menubar_height + (i * menubar_height));
					window.draw(menubar);
				}

				sname = Task::StatusString(filteroptions[i]);
				if (sname == "None")
				{
					sname = "Show All";
				}
				filtertext.setFillColor(sf::Color::White);
				if (shown_status == filteroptions[i]) filtertext.setFillColor(sf::Color(255, 180, 0));
				filtertext.setString(sname);
				filtertext.setPosition(24, fby + menubar_height + (i * menubar_height) + 8);
				window.draw(filtertext);

				if (i == 0)
				{
					menubar.setFillColor(sf::Color::White);
					menubar.setSize(sf::Vector2f(fbw * .8f, 1.f));
					menubar.setPosition(fbx + (fbw * .1f), fby + menubar_height - 1);
					window.draw(menubar);
				}
				else
				{
					menubar.setFillColor(sf::Color::White);
					menubar.setSize(sf::Vector2f(fbw * .7f, 1.f));
					menubar.setPosition(fbx + (fbw * .15f), fby + menubar_height + (i * menubar_height));
					window.draw(menubar);
				}
			}
		}

		window.setView(v);

		menubar.setFillColor(sf::Color(120, 120, 120));
		menubar.setSize(sf::Vector2f(1.f, menubar_height * .4f));
		menubar.setPosition(fbx + fbw, fby + menubar_height * .3f);
		window.draw(menubar);


		/* NEW TASK BUTTON */
		fbx += fbw + 1;
		fby = 0;
		fbw = 108;

		if (mouse.gpos.x >= fbx && mouse.gpos.y >= fby && mouse.gpos.x < fbx + fbw && mouse.gpos.y < fby + menubar_height)
		{
			newtask_hilite.SetRGB(64, 64, 64);

			if (mouse.buttons[0] == MouseState::ButtonState::Pressed)
			{
				mouse.grabbed_key = GRAB_KEY_MENUBAR;
				mouse.grabbed_id = 1;
				mouse.grabbed_id_sub = -1;
			}
			else if (mouse.buttons[0] == MouseState::ButtonState::Released)
			{
				if (mouse.grabbed_key == GRAB_KEY_MENUBAR && mouse.grabbed_id == 1)
				{
					maingui.removeAllWidgets();
					winstate = WindowState::NewTask;

					task_sel = Task();
					task_seluid = task_sel.uid;


					SetupNewTaskPanel();
				}

				mouse.grabbed_key = GRAB_KEY_MENUBAR;
				mouse.grabbed_id = 0;
				mouse.grabbed_id_sub = -1;
			}
		}
		else
		{
			newtask_hilite.SetRGB(40, 40, 40);
		}

		menubar.setFillColor(newtask_hilite.Color());
		menubar.setSize(sf::Vector2f(fbw, menubar_height - 1));
		menubar.setPosition(fbx, fby);
		window.draw(menubar);

		sf::Text menutext;
		menutext.setFont(Mnemosyne::GetFont("exo2"));
		menutext.setCharacterSize(14);
		menutext.setString("New Task...");
		menutext.setPosition(fbx + 12, fby + 6);
		window.draw(menutext);


		window.display();


		if (mouse.buttons[0] == MouseState::ButtonState::Released)
		{
			if (mouse.grabbed_key != 1 && mouse.grabbed_key != 2)
			{
				for (int i = 0; i < tasks.size(); ++i)
				{
					if (tasks[i].renderstate == Task::RenderState::Selected)
					{
						tasks[i].renderstate = Task::RenderState::None;
					}
				}
			}
		}

		if (mouse.grabbed_key == GRAB_KEY_TASKBUTTON)
		{
			if (mouse.grabbed_id_sub >= 0 && mouse.grabbed_id_sub < tasks.size())
			{
				tasks[mouse.grabbed_id_sub].buttons.buttons[mouse.grabbed_id].held = true;
			}
		}
	}

	maingui.removeAllWidgets();
    return 0;
}


bool AddTask(Task t, bool _do_refresh)
{
	t.SetupButtons();
	tasks.push_back(t);

	if (_do_refresh)
	{
		SortTasks();
		UpdateTasks();
	}

	return true;
}


void SetupNewTaskPanel()
{
	auto guianchor = tgui::Label::create();
	guianchor->setText("");
	guianchor->setVisible(false);
	gui->add(guianchor, "anchor");

	auto label = tgui::Label::create();
	label->setText("Task Name");
	label->setPosition("anchor.left + 24", "anchor.top + 30");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "new_task_name_label");

	auto editbox = tgui::EditBox::create();
	editbox->setSize("35%", 40);
	editbox->setPosition("anchor.left + 24", "anchor.top + 50");
	editbox->setText("");
	editbox->setDefaultText("Task Name");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "new_task_name_editbox");


	label = tgui::Label::create();
	label->setText("Priority");
	label->setPosition("anchor.left + 24", "anchor.top + 104");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "new_task_priority_label");

	auto combobox = tgui::ComboBox::create();
	combobox->setSize("20%", 28);
	combobox->setPosition("anchor.left + 24", "anchor.top + 124");
	combobox->setChangeItemOnScroll(true);
	combobox->addItem("Critical");
	combobox->addItem("Important");
	combobox->addItem("Normal");
	combobox->addItem("Low");
	combobox->addItem("Reminder");
	//combobox->setSelectedItem("Important");
	combobox->setTextSize(0);
	combobox->setRenderer(blackTheme.getRenderer("ComboBox"));
	gui->add(combobox, "new_task_priority_combobox");


	label = tgui::Label::create();
	label->setText("Client");
	label->setPosition("anchor.left + 24", "anchor.top + 166");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "new_task_client_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32);
	editbox->setPosition("anchor.left + 24", "anchor.top + 186");
	editbox->setText("");
	editbox->setDefaultText("Client Name");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "new_task_client_editbox");


	label = tgui::Label::create();
	label->setText("Date Opened");
	label->setPosition("anchor.left + 24", "anchor.top + 232");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "new_task_date_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32);
	editbox->setPosition("anchor.left + 24", "anchor.top + 252");
	editbox->setText(splitsexy(datestr(task_sel.date_created), " ")[0]);
	editbox->setDefaultText("Date Opened");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "new_task_date_editbox");



	label = tgui::Label::create();
	label->setText("Description");
	label->setPosition("new_task_name_editbox.right + 5%", "new_task_name_label.top");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "new_task_description_label");

	auto textarea = tgui::TextArea::create();
	textarea->setRenderer(blackTheme.getRenderer("TextArea"));
	//textarea->setSize("95% - new_task_name_editbox.width - 48", "new_task_ok_button.top - new_task_name_editbox.top - 24");
	textarea->setPosition("new_task_name_editbox.right + 5%", "new_task_name_editbox.top");
	gui->add(textarea, "new_task_description_textarea");


	tgui::Button::Ptr button = tgui::Button::create();
	button->setSize(100, 32);
	button->setPosition("anchor.left + 24", "100% - 56");
	button->setText("OK");
	button->setRenderer(blackTheme.getRenderer("Button"));
	button->onPress(NewTaskOK, true);
	gui->add(button, "new_task_ok_button");

	button = tgui::Button::create();
	button->setSize(100, 32);
	button->setPosition("new_task_ok_button.right + 12", "new_task_ok_button.top");
	button->setText("Cancel");
	button->setRenderer(blackTheme.getRenderer("Button"));
	button->onPress(NewTaskOK, false);
	gui->add(button, "new_task_cancel_button");


	gui->get<tgui::Widget>("new_task_description_textarea")->setSize("95% - new_task_name_editbox.width - 48", "new_task_ok_button.top - new_task_name_editbox.top - 24");
}

void NewTaskOK(bool ok)
{
	if (ok)
	{
		std::string namestr = gui->get<tgui::EditBox>("new_task_name_editbox")->getText().toStdString();
		if (namestr == "") return;

		int priority = gui->get<tgui::ComboBox>("new_task_priority_combobox")->getSelectedItemIndex() + 1;
		if (priority <= 0) return;

		time_t date = dateint(gui->get<tgui::EditBox>("new_task_date_editbox")->getText().toStdString());
		if (date == -1) return;

		task_sel.SetName(namestr);
		task_sel.SetPriority(priority);
		task_sel.date_created = date;
		task_sel.SetClient(gui->get<tgui::EditBox>("new_task_client_editbox")->getText().toStdString());
		task_sel.SetDescription(gui->get<tgui::TextArea>("new_task_description_textarea")->getText().toStdString());

		task_sel.SetStatus(Task::Status::Open);
		task_sel.StatusUpdate();
		AddTask(task_sel);
	}

	winstate = WindowState::TaskList;
}


void SetupEditTaskPanel()
{
	auto guianchor = tgui::Label::create();
	guianchor->setText("");
	guianchor->setVisible(false);
	gui->add(guianchor, "anchor");

	auto label = tgui::Label::create();
	label->setText("Task Name");
	label->setPosition("anchor.left + 24", "anchor.top + 30");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "edit_task_name_label");

	auto editbox = tgui::EditBox::create();
	editbox->setSize("35%", 40);
	editbox->setPosition("anchor.left + 24", "anchor.top + 50");
	editbox->setText(task_sel.GetName());
	editbox->setDefaultText("Task Name");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "edit_task_name_editbox");


	label = tgui::Label::create();
	label->setText("Priority");
	label->setPosition("anchor.left + 24", "anchor.top + 104");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "edit_task_priority_label");

	auto combobox = tgui::ComboBox::create();
	combobox->setSize("20%", 28);
	combobox->setPosition("anchor.left + 24", "anchor.top + 124");
	combobox->setChangeItemOnScroll(true);
	combobox->addItem("Critical");
	combobox->addItem("Important");
	combobox->addItem("Normal");
	combobox->addItem("Low");
	combobox->addItem("Reminder");
	combobox->setSelectedItemByIndex(task_sel.GetPriority() - 1);
	combobox->setTextSize(0);
	combobox->setRenderer(blackTheme.getRenderer("ComboBox"));
	gui->add(combobox, "edit_task_priority_combobox");


	label = tgui::Label::create();
	label->setText("Client");
	label->setPosition("anchor.left + 24", "anchor.top + 166");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "edit_task_client_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32);
	editbox->setPosition("anchor.left + 24", "anchor.top + 186");
	editbox->setText(task_sel.GetClient());
	editbox->setDefaultText("Client Name");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "edit_task_client_editbox");


	label = tgui::Label::create();
	label->setText("Date Opened");
	label->setPosition("anchor.left + 24", "anchor.top + 232");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "edit_task_date_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32);
	editbox->setPosition("anchor.left + 24", "anchor.top + 252");
	editbox->setText(splitsexy(datestr(task_sel.date_created), " ")[0]);
	editbox->setDefaultText("Date Opened");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	editbox->setEnabled(false);
	editbox->setReadOnly(true);
	gui->add(editbox, "edit_task_date_editbox");



	label = tgui::Label::create();
	label->setText("Description");
	label->setPosition("edit_task_name_editbox.right + 5%", "edit_task_name_label.top");
	label->setRenderer(blackTheme.getRenderer("Label"));
	gui->add(label, "edit_task_description_label");

	auto textarea = tgui::TextArea::create();
	textarea->setRenderer(blackTheme.getRenderer("TextArea"));
	//textarea->setSize("95% - edit_task_name_editbox.width - 48", "edit_task_ok_button.top - edit_task_name_editbox.top - 24");
	textarea->setPosition("edit_task_name_editbox.right + 5%", "edit_task_name_editbox.top");
	textarea->setText(task_sel.GetDescription());
	gui->add(textarea, "edit_task_description_textarea");


	tgui::Button::Ptr button = tgui::Button::create();
	button->setSize(100, 32);
	button->setPosition("anchor.left + 24", "100% - 56");
	button->setText("OK");
	button->setRenderer(blackTheme.getRenderer("Button"));
	button->onPress(EditTaskOK, true);
	gui->add(button, "edit_task_ok_button");

	button = tgui::Button::create();
	button->setSize(100, 32);
	button->setPosition("edit_task_ok_button.right + 12", "edit_task_ok_button.top");
	button->setText("Cancel");
	button->setRenderer(blackTheme.getRenderer("Button"));
	button->onPress(EditTaskOK, false);
	gui->add(button, "edit_task_cancel_button");


	gui->get<tgui::Widget>("edit_task_description_textarea")->setSize("95% - edit_task_name_editbox.width - 48", "edit_task_ok_button.top - edit_task_name_editbox.top - 24");
}

void EditTaskOK(bool ok)
{
	if (ok)
	{
		std::string namestr = gui->get<tgui::EditBox>("edit_task_name_editbox")->getText().toStdString();
		if (namestr == "") return;

		int priority = gui->get<tgui::ComboBox>("edit_task_priority_combobox")->getSelectedItemIndex() + 1;
		if (priority <= 0) return;

		time_t date = dateint(gui->get<tgui::EditBox>("edit_task_date_editbox")->getText().toStdString());
		if (date == -1) return;

		task_sel.SetName(namestr);
		task_sel.SetPriority(priority);
		task_sel.date_created = date;
		task_sel.SetClient(gui->get<tgui::EditBox>("edit_task_client_editbox")->getText().toStdString());
		task_sel.SetDescription(gui->get<tgui::TextArea>("edit_task_description_textarea")->getText().toStdString());

		task_sel.SetStatus(Task::Status::Open);
		task_sel.StatusUpdate();
		
		for (int i = 0; i < tasks.size(); ++i)
		{
			if (tasks[i].GetUID() == task_seluid)
			{
				tasks[i] = task_sel;
				break;
			}
		}

		SortTasks();
		UpdateTasks();
	}

	winstate = WindowState::TaskList;
}


bool UpdateTasks()
{
	cout << "Writing data..." << flush;

	std::ofstream fileout("tasks");
	if (!fileout.is_open())
	{
		cout << "failed" << endl;
		cout << "Error opening file for writing" << endl;
		return false;
	}

	for (int i = 0; i < tasks.size(); ++i)
	{
		if (i > 0) WriteLine(fileout, ",\n");
		WriteLine(fileout, tasks[i].Serialize());
	}

	fileout.close();
	cout << "done" << endl;
	return true;
}


void WriteLine(std::ofstream& fileout, std::string line)
{
	fileout << line;
}


bool ReadTasks()
{
	tasks.clear();


	ifstream filein("tasks");
	if (!filein.is_open())
	{
		cout << "Error opening file for reading" << endl;
		return false;
	}

	std::string line;
	while (getline(filein, line))
	{
		std::map<std::string, std::string> vars;

		std::vector<std::string> fields = ToList(ExtractSexyBraces(line));
		for (int i = 0; i < fields.size(); ++i)
		{
			std::vector<std::string> args = ToList(fields[i], ":");
			if (args.size() != 2)
			{
				cout << "field incorrectly formatted" << endl;
				cout << "Field: " << endl;

				for (int j = 0; j < args.size(); ++j)
				{
					cout << "<<" << args[j] << ">>" << endl;
				}

				continue;
			}

			if (args[1][0] == '"' && args[1][args[1].size() - 1] == '"')
			{
				args[1] = args[1].substr(1, args[1].size() - 2);
				std::string newval;
				for (int j = 0; j < args[1].size(); ++j)
				{
					if (args[1][j] == '\\')
					{
						if (args[1][j + 1] == 'n')
						{
							++j;
							newval += "\n";
							continue;
						}
					}
					else
					{
						newval += args[1][j];
					}
				}
				args[1] = newval;
			}

			Trim(args[1]);
			vars[Lower(args[0])] = args[1];
		}

		AddTask(Task::Create(vars), false);
	}

	return true;
}


bool SortTasks()
{
	std::sort(tasks.begin(), tasks.end());
	return true;
}


float BellValue(float x, int p)
{
	if (x < 0 || x >= p) return 0.f;
	float y = (-cos((20.f * x) / (p * 3.14159f)) + 1) * .5f;

	return y;
}
