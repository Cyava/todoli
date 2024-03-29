#include "inc.hpp"

#include "Button.hpp"
#include "MouseState.hpp"
#include "Mnemosyne.hpp"
#include "Task.hpp"

#include <chrono>
#include <ctime>

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>

#include "dwmapi.h"
#include "winuser.h"
#include "wtypes.h"


using namespace std;


std::vector<Task> tasks;
unique_vector<std::string> ttags;

Task task_sel;
std::string task_seluid;

sf::Vector2f winscale(1, 1);


MouseState mouse;

FadeColor menubar_color(40, 40, 40);
FadeColor filterbox_hilite(40, 40, 40);
FadeColor newtask_hilite(40, 40, 40);
FadeColor searchbox_hilite(40, 40, 40);


bool ui_focus = false;

float filterbox_target = 0.f;
float filterbox_scale = 0.f;
float filterbox_speed = .1f;
float filterbox_width = 160;
float filterbox_height = 100;
std::vector<Task::Status> filteroptions{ Task::Status::None, Task::Status::Open, Task::Status::Complete, Task::Status::Canceled };

float searchbox_min_width = 120.f;
float searchbox_target_width = searchbox_min_width;
float searchbox_speed = .2f;
FadeColor searchbox_outline(255, 255, 255);
float searchbox_width = searchbox_min_width;
float searchbox_height = filterbox_height;
std::string searchString = "";


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

bool UpdateTasks(bool do_write = true);

void WriteLine(std::ofstream&, std::string);

bool ReadTasks();

bool SortTasks();


float BellValue(float, int);


int main(int argc, char** argv)
{
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);

	sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();

	cout << "sfml says:    " << desktopMode.width << " x " << desktopMode.height << endl;
	cout << "windows says: " << desktop.right << " x " << desktop.bottom << endl;
	winscale = sf::Vector2f(desktopMode.width / (float)desktop.right, desktopMode.height / (float)desktop.bottom);
	cout << "scaling says: " << winscale.x << " x " << winscale.y << endl;

	Task::winscale = winscale;
	mouse.winscale = winscale;


	Mnemosyne::Init();

	Task::InitializeColorMap();
	menubar_color.mode = FadeColor::Mode::Quadratic;
	menubar_color.speed = .16f;

	filterbox_hilite.mode = FadeColor::Mode::Quadratic;
	filterbox_hilite.speed = .16f;

	newtask_hilite.mode = FadeColor::Mode::Quadratic;
	newtask_hilite.speed = .16f;

	searchbox_hilite.mode = FadeColor::Mode::Quadratic;
	searchbox_hilite.speed = .16f;

	searchbox_outline.mode = FadeColor::Mode::Quadratic;
	searchbox_outline.speed = .16f;


	sf::ContextSettings context;
	context.depthBits = 24;
	context.stencilBits = 8;
	context.antialiasingLevel = 8;
	context.majorVersion = 3;
	context.minorVersion = 2;

	sf::RenderWindow window(sf::VideoMode(640 * winscale.x, 480 * winscale.y), "Litodo", sf::Style::Default, context);
	window.setVerticalSyncEnabled(true);

	sf::WindowHandle hwin = window.getSystemHandle();

	BOOL USE_DARK_MODE = true;
	BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(hwin, 20, &USE_DARK_MODE, sizeof(USE_DARK_MODE)));
	//window.setSize(sf::Vector2u(640 * winscale.x, 481 * winscale.y));
	//window.setSize(sf::Vector2u(640 * winscale.x, 480 * winscale.y));
	window.setVisible(false);
	window.setVisible(true);
	sf::Vector2f winmeasure(window.getSize().x / winscale.x, window.getSize().y / winscale.y);
	
	sf::Image litodo_icon;
	if (litodo_icon.loadFromFile("assets/texture/icon2.png"))
	{
		window.setIcon(litodo_icon.getSize().x, litodo_icon.getSize().y, litodo_icon.getPixelsPtr());
	}
	//SetWindowPos(hwin, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);


	//sf::View view(sf::FloatRect(0, 0, winmeasure.x, winmeasure.y));
	sf::View view(sf::FloatRect(0, 0, 640, 480));
	window.setView(view);

	tgui::GuiSFML maingui{ window };
	blackTheme.load("assets/themes/black.txt");

	gui = &maingui;

	float viewwidth = window.getView().getSize().x;
	float viewheight = window.getView().getSize().y;
	winsize = window.getSize().x / winscale.x;

	Task::Status shown_status = Task::Status::Open;

	ReadTasks();
	SortTasks();


	RollingAverage rav(16);
	float pval = 0.f;
	int last_n = 4;

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
		bool searching = (searchbox_target_width > searchbox_min_width);

		int menubar_height = 32;
		winmeasure = sf::Vector2f(window.getSize().x / winscale.x, window.getSize().y / winscale.y);


		float tasklistHeight = 0.f;
		for (int i = 0; i < tasks.size(); ++i)
		{
			tasks[i].hidden_by_filter = false;
			taskx = taskx_anchor * winsize;

			//if (shown_status != Task::Status::None && tasks[i].status != shown_status) continue;

			if (shown_status != Task::Status::None && tasks[i].status != shown_status)
			{
				tasks[i].tagdisplay = 0.f;
				tasks[i].hidden_by_filter = true;
				continue;
			}

			if (searchString.length() > 0)
			{
				std::vector<std::string> searchterms = ToList(searchString, ";");

				bool matches = false;
				//if (Upper(tasks[i].name).find(Upper(searchString)) == std::string::npos) continue;
				std::string namecomp = Upper(tasks[i].name);
				for (std::string term : searchterms)
				{
					if (term.length() <= 0) continue;
					std::string sterm = Upper(term);

					if (namecomp.find(sterm) != std::string::npos)
					{
						matches = true;
						break;
					}

					size_t cidx = sterm.find_first_of(':');
					if (cidx != std::string::npos)
					{
						std::string field = sterm.substr(0, cidx);
						std::string val = sterm.substr(cidx + 1);
						if (field == "TAG")
						{
							for (std::string tagcomp : tasks[i].tags)
							{
								tagcomp = Upper(tagcomp);

								if (val == tagcomp)
								{
									matches = true;
									break;
								}
							}
						}
					}

					if (matches) break;
				}

				if (!matches)
				{
					tasks[i].tagdisplay = 0.f;
					tasks[i].hidden_by_filter = true;
					continue;
				}
			}

			tasklistHeight += tasks[i].Height();
		}

		if (tasklistHeight > (winmeasure.y - menubar_height))
		{
			showscroll = true;
		}
		else
		{
			showscroll = false;
			tasklistHeight = winmeasure.y - menubar_height;
		}


		mouse.Update(sf::Mouse::getPosition(window), &window);
		float prevscroll = mouse.scrolled_amount;
		mouse.scrolled_amount = 0.f;


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
					sf::View view(sf::FloatRect(0, 0, window.getSize().x / winscale.x, window.getSize().y / winscale.y));
					window.setView(view);
					winsize = window.getSize().x / winscale.x;

					winmeasure = sf::Vector2f(window.getSize().x / winscale.x, window.getSize().y / winscale.y);

					break;
				}
				case sf::Event::TextEntered:
				{
					if (searching)
					{
						uint32_t c = event.text.unicode;

						if (c == 8)
						{
							if (searchString.length() > 0) searchString = searchString.substr(0, searchString.length() - 1);
						}
						else if (c == 10 || c == 13)
						{
							//searchString += c;
						}
						else if (c == 27)
						{
							//searchString += c;
						}
						else
						{
							searchString += c;
							//cout << c << endl;
						}
					}

					break;
				}
				case sf::Event::KeyPressed:
				{
					if (event.key.code == sf::Keyboard::Key::F)
					{
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl))
						{
							ui_focus = true;
							searchbox_target_width = winmeasure.x * .4f;

							mouse.grabbed_key = GRAB_KEY_MENUBAR;
							mouse.grabbed_id = 2;
							mouse.grabbed_id_sub = -1;
						}
					}

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
					else if (event.key.code == sf::Keyboard::Key::Escape)
					{
						if (searching)
						{
							searchString = "";
							
							ui_focus = false;
							searchbox_target_width = searchbox_min_width;
							searching = false;
						}
					}

					break;
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
					mouse.scrolled_amount = delta;

#if 0
					if (delta == 0 && rav.AvgOfLastN(last_n) == 0)
					{
						rav.Clear();
					}
					else if (rav.Get() == 0)
					{
						rav.Fill(delta);
					}
					else
					{
						rav.Add(delta);
					}
					//cout << "- - -\n" << "> Delta: " << std::to_string(delta) << "\n> Rav: " << std::to_string(rav.Get()) << endl;
					//delta = rav.Get();
					
					scroll_actual -= delta;
					if (scroll_actual < 0 || scroll_actual > tasklistHeight - winmeasure.y + menubar_height) scroll_render -= delta;
#endif
				}
			}
		}


#if 1
		if (mouse.scrolled_amount == 0 && rav.AvgOfLastN(last_n) == 0)
		{
			rav.Clear();
		}
		else if (rav.Get() == 0)
		{
			rav.Fill(mouse.scrolled_amount);
		}
		else
		{
			rav.Add(mouse.scrolled_amount);
		}
		if (rav.Get() != 0)
		{
			float delta = rav.Get();// mouse.scrolled_amount;

			scroll_actual -= delta;
			if (scroll_actual < 0 || scroll_actual > tasklistHeight - winmeasure.y + menubar_height) scroll_render -= delta;
		}
#endif


		if (scroll_actual < 0)
		{
			scroll_actual = 0;
		}
		else if (scroll_actual > tasklistHeight - winmeasure.y + menubar_height)
		{
			scroll_actual = tasklistHeight - winmeasure.y + menubar_height;
		}

		scroll_divide = 2.f;
		if (scroll_render < 0 || scroll_render > tasklistHeight - winmeasure.y + menubar_height) scroll_divide = 3.f;


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
		

		Point tsize(winmeasure.x, 0);

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

			if (shown_status != Task::Status::None && tasks[i].status != shown_status)
			{
				tasks[i].tagdisplay = 0.f;
				continue;
			}

			if (searchString.length() > 0)
			{
				std::vector<std::string> searchterms = ToList(searchString, ";");

				bool matches = false;
				//if (Upper(tasks[i].name).find(Upper(searchString)) == std::string::npos) continue;
				std::string namecomp = Upper(tasks[i].name);
				for (std::string term : searchterms)
				{
					if (term.length() <= 0) continue;
					std::string sterm = Upper(term);

					if (namecomp.find(sterm) != std::string::npos)
					{
						matches = true;
						break;
					}

					size_t cidx = sterm.find_first_of(':');
					if (cidx != std::string::npos)
					{
						std::string field = sterm.substr(0, cidx);
						std::string val = sterm.substr(cidx + 1);
						if (field == "TAG")
						{
							for (std::string tagcomp : tasks[i].tags)
							{
								tagcomp = Upper(tagcomp);

								if (val == tagcomp)
								{
									matches = true;
									break;
								}
							}
						}
					}

					if (matches) break;
				}

				if (!matches)
				{
					tasks[i].tagdisplay = 0.f;
					continue;
				}
			}


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
			tasks[i].ListRender(&window, taskx, tasky, tsize.x - scrollbar_width, &mouse);
			
			if (tasks[i].is_expanding)
			{
				//std::cout << "i need to scroll by " << ((tasky + tasks[i].Height()) - window.getSize().y) << std::endl;
				if (tasky + tasks[i].Height() > winmeasure.y)
				{
					//std::cout << "i need to scroll by " << ((tasky + tasks[i].Height()) - window.getSize().y) << std::endl;
					scroll_actual = ((tasky + tasks[i].Height()) - winmeasure.y) + scroll_render - 1;
				}
			}

#if 0
			if (tasks[i].renderstate == Task::RenderState::Selected) line_cutoff = (tasky + tasks[i].Height());

			if (tasks[i].has_expanded)
			{
				std::cout << "i missed by " << ((tasky + tasks[i].Height()) - winmeasure.y) << std::endl;

				std::cout << "delta: " << (line_cutoff - winmeasure.y + scroll_render) << " / " << scroll_actual << std::endl;
				// FIX THIS, IT'S WRONG
			}
#endif

			tasky += tasks[i].Height();
		}

		for (int i = 0; i < tasks.size(); ++i)
		{
			if (tasks[i].tagdisplay > 0.f && tasks[i].tags.size() > 0)
			{
				int ex_spacing = 2;

				sf::Text tagtext;
				float tagFontSize = 15;
				tagtext.setFont(Mnemosyne::GetFont("exo2"));
				tagtext.setCharacterSize(tagFontSize);

				int widest = 0;

				for (int j = 0; j < tasks[i].tags.size(); ++j)
				{
					tagtext.setString(tasks[i].tags[j]);
					if (tagtext.getGlobalBounds().width > widest) widest = tagtext.getGlobalBounds().width;
				}
				float headspace = TextInfo::StandardTextHeadspace(tagtext);

				sf::RectangleShape tagDispRekt(sf::Vector2f(widest + 16, (tasks[i].tags.size() * (tagFontSize + ex_spacing)) + headspace + ex_spacing));
				float tdx = tasks[i].tagdisplay_x;
				float tdy = tasks[i].tagdisplay_y - (tagDispRekt.getSize().y / 2);

				if (tdx > ((window.getSize().x / winscale.x) - tagDispRekt.getSize().x - scrollbar_width - 1)) tdx = (window.getSize().x / winscale.x) - tagDispRekt.getSize().x - scrollbar_width - 1;
				if (tdy < menubar_height + 1) tdy = menubar_height + 1;
				if (tdy + tagDispRekt.getSize().y > (window.getSize().y / winscale.y) - 1) tdy = (window.getSize().y / winscale.y) - tagDispRekt.getSize().y - 1;

				tagDispRekt.setPosition(tdx, tdy);
				tagDispRekt.setFillColor(sf::Color(24, 24, 24, tasks[i].tagdisplay * 255));
				tagDispRekt.setOutlineColor(sf::Color(255, 150, 0, tasks[i].tagdisplay * 255));
				tagDispRekt.setOutlineThickness(1.f);

				window.draw(tagDispRekt);


				for (int j = 0; j < tasks[i].tags.size(); ++j)
				{
					tagtext.setString(tasks[i].tags[j]);
					tagtext.setPosition(tdx + 8, tdy + (tagFontSize * j) + (ex_spacing * (j + 1)));
					tagtext.setFillColor(sf::Color(255, 222, 180, tasks[i].tagdisplay * 255));
					window.draw(tagtext);
				}
			}
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

			scrollrekt.setPosition(taskx + winmeasure.x - scrollbar_width, menubar_height);
			scrollrekt.setFillColor(sf::Color(12, 12, 12));
			scrollrekt.setOutlineColor(sf::Color(255, 255, 255));
			scrollrekt.setOutlineThickness(1.f);
			window.draw(scrollrekt);


			float viewportHeight = winmeasure.y - menubar_height;
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
				else if (scroll_actual > tasklistHeight - winmeasure.y + menubar_height)
				{
					float endscroll = tasklistHeight - winmeasure.y + menubar_height;
					float temp = log((abs(scroll_actual - endscroll) + 100) * .01f);
					scroll_render = (temp > 0 ? (temp * 100) + endscroll : scroll_actual);
					sb_y = sb_scale * scroll_render;

					scroll_actual = tasklistHeight - winmeasure.y + menubar_height;
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
			scrollrekt.setPosition(taskx + winmeasure.x - scrollbar_width, menubar_height + sb_y);
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
			float wh = winmeasure.y - menubar_height;

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


			maingui.get<tgui::Widget>("anchor")->setPosition(wx * winscale.x, wy * winscale.y);
		}


		maingui.draw();


		menubar_color.Update();
		filterbox_hilite.Update();
		newtask_hilite.Update();
		searchbox_hilite.Update();
		searchbox_outline.Update();

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
					if (filterbox_target == 0.f) // if (!ui_focus && filterbox_target == 0.f)
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

		float vx = fbx / winmeasure.x;
		float vy = fby / winmeasure.y;
		float vw = fbw / winmeasure.x;
		float vh = filterbox_height / winmeasure.y;

		sf::View filterview(sf::FloatRect(fbx, fby, fbw, filterbox_height));
		filterview.setViewport(sf::FloatRect(vx, vy, vw, vh));
		window.setView(filterview);

		menubar.setFillColor(filterbox_hilite.Color());
		menubar.setSize(sf::Vector2f(fbw, filterbox_height - 1));
		menubar.setPosition(fbx, fby);
		window.draw(menubar);

		sf::Vector2f ftpos(10, 11);
		sf::VertexArray filtertriangle(sf::Triangles, 3);
		//filtertriangle[0].position = sf::Vector2f(ftpos.x, ftpos.y);
		//filtertriangle[1].position = sf::Vector2f(ftpos.x + 6, ftpos.y + 4);
		//filtertriangle[2].position = sf::Vector2f(ftpos.x, ftpos.y + 8);
		filtertriangle[0].position = sf::Vector2f(-3, -4);
		filtertriangle[1].position = sf::Vector2f(3, 0);
		filtertriangle[2].position = sf::Vector2f(-3, 4);

		filtertriangle[0].color = sf::Color(255, 255, 255);
		filtertriangle[1].color = sf::Color(255, 255, 255);
		filtertriangle[2].color = sf::Color(255, 255, 255);

		sf::Transform filtertriangleRotator;
		filtertriangleRotator.translate(ftpos.x + 3, ftpos.y + 4);
		filtertriangleRotator.rotate(filterbox_scale * 90.f);

		window.draw(filtertriangle, filtertriangleRotator);

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



		/* SEARCH BAR */
		float sbx = tsize.x - searchbox_width;
		fby = 0;

		menubar.setFillColor(searchbox_hilite.Color());
		menubar.setSize(sf::Vector2f(searchbox_width, menubar_height - 1));
		menubar.setPosition(sbx, fby);
		window.draw(menubar);

		if (searching)
		{
			searchbox_outline.SetRGB(255, 160, 0);
		}
		else
		{
			searchbox_outline.SetRGB(255, 255, 255);
		}
		menubar.setOutlineColor(searchbox_outline.Color());
		menubar.setOutlineThickness(-1.f);
		menubar.setFillColor(sf::Color(12, 12, 12));
		menubar.setSize(sf::Vector2f(searchbox_width - 12, menubar_height - 11));
		menubar.setPosition(sbx + 6, fby + 5);
		window.draw(menubar);
		menubar.setOutlineThickness(0);

		sf::Text searchText;
		searchText.setCharacterSize(menubar_height - 16);
		searchText.setFont(Mnemosyne::GetFont("exo2"));
		if (searchString.length() > 0)
		{
			searchText.setString(searchString);
			searchText.setFillColor(sf::Color::White);
		}
		else
		{
			searchText.setString("Search");
			searchText.setFillColor(sf::Color(180, 180, 180));
		}
		searchText.setPosition(menubar.getPosition().x + 4, menubar.getPosition().y);
		window.draw(searchText);

		/*
		menubar.setFillColor(sf::Color(120, 120, 120));
		menubar.setSize(sf::Vector2f(1.f, menubar_height * .4f));
		menubar.setPosition(sbx - 1.f, fby + menubar_height * .3f);
		window.draw(menubar);
		*/

		if (searchbox_width != searchbox_target_width)
		{
			float change = (searchbox_target_width - searchbox_width) * searchbox_speed;
			if (abs(change) < .01f)
			{
				searchbox_width = searchbox_target_width;
			}
			else
			{
				searchbox_width += change;
			}
		}

		if (mouse.gpos.x >= sbx && mouse.gpos.y >= fby && mouse.gpos.x < sbx + searchbox_width && mouse.gpos.y < fby + menubar_height)
		{
			searchbox_hilite.SetRGB(64, 64, 64);

			if (mouse.buttons[0] == MouseState::ButtonState::Pressed)
			{
				mouse.grabbed_key = GRAB_KEY_MENUBAR;
				mouse.grabbed_id = 2;
				mouse.grabbed_id_sub = -1;
			}
			else if (mouse.buttons[0] == MouseState::ButtonState::Released)
			{
				if (mouse.grabbed_key == GRAB_KEY_MENUBAR && mouse.grabbed_id == 2)
				{
					ui_focus = true;
					searchbox_target_width = tsize.x * .4f;
				}

				mouse.grabbed_key = GRAB_KEY_MENUBAR;
				mouse.grabbed_id = 2;
				mouse.grabbed_id_sub = -1;
			}
		}
		else
		{
			searchbox_hilite.SetRGB(40, 40, 40);

			if (mouse.buttons[0] == MouseState::ButtonState::Released && ui_focus && searchbox_target_width > searchbox_min_width)
			{
				ui_focus = false;
				searchbox_target_width = searchbox_min_width;
			}
		}



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
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(30 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "new_task_name_label");

	auto editbox = tgui::EditBox::create();
	editbox->setSize("35%", 40 * winscale.y);
	editbox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(50 * winscale.y)).c_str());
	editbox->setText("");
	editbox->setDefaultText("Task Name");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "new_task_name_editbox");


	label = tgui::Label::create();
	label->setText("Priority");
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(104 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "new_task_priority_label");

	auto combobox = tgui::ComboBox::create();
	combobox->setSize("20%", 28 * winscale.y);
	combobox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(124 * winscale.y)).c_str());
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
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(166 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "new_task_client_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32 * winscale.y);
	editbox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(186 * winscale.y)).c_str());
	editbox->setText("");
	editbox->setDefaultText("Client Name");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "new_task_client_editbox");


	label = tgui::Label::create();
	label->setText("Date Opened");
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(232 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "new_task_date_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32 * winscale.y);
	editbox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(252 * winscale.y)).c_str());
	editbox->setText(splitsexy(datestr(task_sel.date_created), " ")[0]);
	editbox->setDefaultText("Date Opened");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "new_task_date_editbox");


	label = tgui::Label::create();
	label->setText("Tags");
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(298 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "new_task_tag_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32 * winscale.y);
	editbox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(318 * winscale.y)).c_str());
	editbox->setText("");
	editbox->setDefaultText("Tags");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "new_task_tag_editbox");



	label = tgui::Label::create();
	label->setText("Description");
	label->setPosition("new_task_name_editbox.right + 5%", "new_task_name_label.top");
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "new_task_description_label");

	auto textarea = tgui::TextArea::create();
	textarea->setRenderer(blackTheme.getRenderer("TextArea"));
	//textarea->setSize("95% - new_task_name_editbox.width - 48", "new_task_ok_button.top - new_task_name_editbox.top - 24");
	textarea->setPosition("new_task_name_editbox.right + 5%", "new_task_name_editbox.top");
	textarea->setTextSize(16 * winscale.y);
	gui->add(textarea, "new_task_description_textarea");


	tgui::Button::Ptr button = tgui::Button::create();
	button->setSize(100 * winscale.x, 32 * winscale.y);
	button->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("100% - " + std::to_string(56 * winscale.y)).c_str());
	button->setText("OK");
	button->setRenderer(blackTheme.getRenderer("Button"));
	button->onPress(NewTaskOK, true);
	button->setTextSize(0);
	gui->add(button, "new_task_ok_button");

	button = tgui::Button::create();
	button->setSize(100 * winscale.x, 32 * winscale.y);
	button->setPosition(("new_task_ok_button.right + " + std::to_string(12 * winscale.x)).c_str(), "new_task_ok_button.top");
	button->setText("Cancel");
	button->setRenderer(blackTheme.getRenderer("Button"));
	button->onPress(NewTaskOK, false);
	button->setTextSize(0);
	gui->add(button, "new_task_cancel_button");


	gui->get<tgui::Widget>("new_task_description_textarea")->setSize(("95% - new_task_name_editbox.width - " + std::to_string(48 * winscale.x)).c_str(), ("new_task_ok_button.top - new_task_name_editbox.top - " + std::to_string(24 * winscale.y)).c_str());
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

		task_sel.SetTags(gui->get<tgui::EditBox>("new_task_tag_editbox")->getText().toStdString());

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
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(30 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "edit_task_name_label");

	auto editbox = tgui::EditBox::create();
	editbox->setSize("35%", 40 * winscale.y);
	editbox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(50 * winscale.y)).c_str());
	editbox->setText(task_sel.GetName());
	editbox->setDefaultText("Task Name");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "edit_task_name_editbox");


	label = tgui::Label::create();
	label->setText("Priority");
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(104 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "edit_task_priority_label");

	auto combobox = tgui::ComboBox::create();
	combobox->setSize("20%", 28 * winscale.y);
	combobox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(124 * winscale.y)).c_str());
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
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(166 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "edit_task_client_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32 * winscale.y);
	editbox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(186 * winscale.y)).c_str());
	editbox->setText(task_sel.GetClient());
	editbox->setDefaultText("Client Name");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "edit_task_client_editbox");


	label = tgui::Label::create();
	label->setText("Date Opened");
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(232 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "edit_task_date_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32 * winscale.y);
	editbox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(252 * winscale.y)).c_str());
	editbox->setText(splitsexy(datestr(task_sel.date_created), " ")[0]);
	editbox->setDefaultText("Date Opened");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	editbox->setEnabled(false);
	editbox->setReadOnly(true);
	gui->add(editbox, "edit_task_date_editbox");


	label = tgui::Label::create();
	label->setText("Tags");
	label->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(298 * winscale.y)).c_str());
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "edit_task_tag_label");

	editbox = tgui::EditBox::create();
	editbox->setSize("35%", 32 * winscale.y);
	editbox->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("anchor.top + " + std::to_string(318 * winscale.y)).c_str());
	std::string taglist;
	for (int i = 0; i < task_sel.tags.size(); ++i)
	{
		if (i > 0) taglist += ", ";
		taglist += task_sel.tags[i];
	}
	editbox->setText(taglist);
	editbox->setDefaultText("Tags");
	editbox->setTextSize(0);
	editbox->setRenderer(blackTheme.getRenderer("EditBox"));
	gui->add(editbox, "edit_task_tag_editbox");



	label = tgui::Label::create();
	label->setText("Description");
	label->setPosition("edit_task_name_editbox.right + 5%", "edit_task_name_label.top");
	label->setRenderer(blackTheme.getRenderer("Label"));
	label->setTextSize(13 * winscale.y);
	gui->add(label, "edit_task_description_label");

	auto textarea = tgui::TextArea::create();
	textarea->setRenderer(blackTheme.getRenderer("TextArea"));
	//textarea->setSize("95% - edit_task_name_editbox.width - 48", "edit_task_ok_button.top - edit_task_name_editbox.top - 24");
	textarea->setPosition("edit_task_name_editbox.right + 5%", "edit_task_name_editbox.top");
	textarea->setText(task_sel.GetDescription());
	textarea->setTextSize(16 * winscale.y);
	gui->add(textarea, "edit_task_description_textarea");


	tgui::Button::Ptr button = tgui::Button::create();
	button->setSize(100 * winscale.x, 32 * winscale.y);
	button->setPosition(("anchor.left + " + std::to_string(24 * winscale.x)).c_str(), ("100% - " + std::to_string(56 * winscale.y)).c_str());
	button->setText("OK");
	button->setRenderer(blackTheme.getRenderer("Button"));
	button->onPress(EditTaskOK, true);
	button->setTextSize(0);
	gui->add(button, "edit_task_ok_button");

	button = tgui::Button::create();
	button->setSize(100 * winscale.x, 32 * winscale.y);
	button->setPosition(("edit_task_ok_button.right + " + std::to_string(12 * winscale.x)).c_str(), "edit_task_ok_button.top");
	button->setText("Cancel");
	button->setRenderer(blackTheme.getRenderer("Button"));
	button->onPress(EditTaskOK, false);
	button->setTextSize(0);
	gui->add(button, "edit_task_cancel_button");


	gui->get<tgui::Widget>("edit_task_description_textarea")->setSize(("95% - edit_task_name_editbox.width - " + std::to_string(48 * winscale.x)).c_str(), ("edit_task_ok_button.top - edit_task_name_editbox.top - " + std::to_string(24 * winscale.y)).c_str());
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

		//task_sel.SetStatus(Task::Status::Open);
		task_sel.StatusUpdate();

		task_sel.SetTags(gui->get<tgui::EditBox>("edit_task_tag_editbox")->getText().toStdString());
		
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


bool UpdateTasks(bool do_write)
{
	for (std::string str : ttags)
	{
		cout << str << endl;
	}

	ttags.clear();
	for (int i = 0; i < tasks.size(); ++i)
	{
		ttags.add(tasks[i].tags);
	}

	cout << "[ TAGS ]" << endl;
	for (int i = 0; i < ttags.size(); ++i)
	{
		if (i > 0) cout << ", ";
		cout << ttags[i];
	}
	cout << endl;


	if (do_write)
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
	}
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
	UpdateTasks(false);

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
