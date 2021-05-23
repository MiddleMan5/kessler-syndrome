#pragma once

#include "config.h"
#include "element.h"
#include <SFGUI/Renderers.hpp>
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <SFML/Graphics.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System/Clock.hpp>
#include <cmath>
#include <memory>
#include <sstream>

class Application
{
public:
	Application(AppConfig config) :
		config { config },
		render_window { sf::VideoMode(config.width, config.height), config.title.c_str(), sf::Style::Default, config.renderSettings },
		sfgui {}
	{
		loadFonts();
		configure();

		// We're not using SFML to render anything in this program, so reset OpenGL
		// states. Otherwise we wouldn't see anything.
		render_window.resetGLStates();

		window_main = sfg::Window::Create(sfg::Window::Style::TITLEBAR | sfg::Window::Style::BACKGROUND | sfg::Window::Style::CLOSE);
		window_main->SetTitle(config.title);
		window_main->SetPosition(sf::Vector2f(1400.f, 100.f));

		window_canvas = sfg::Window::Create(sfg::Window::Style::TITLEBAR | sfg::Window::Style::BACKGROUND);
		window_canvas->SetPosition(sf::Vector2f(10.f, 10.f));
		canvas = sfg::Canvas::Create(false);
		canvas->SetRequisition(sf::Vector2f(config.width / 2.f, config.height / 2.f));
		window_canvas->Add(canvas);

		// Add windows to desktop
		desktop.Add(window_main);
		desktop.Add(window_canvas);
	}

	void configure()
	{
		render_window.setVerticalSyncEnabled(false);
		render_window.setFramerateLimit(config.frame_rate);
		render_window.setActive(true);
	}

	void loadFonts()
	{
		const auto font_folder = "resources/fonts";
		std::vector<std::string> font_extensions { ".otf", ".ttf" };

		const auto is_font = [&](util::fs::path const& path) -> bool {
			if (util::fs::is_regular_file(path))
			{
				const auto file_ext = path.extension();
				for (auto& ext : font_extensions)
				{
					if (file_ext == ext)
					{
						return true;
					}
				}
			}
			return false;
		};

		for (const auto& entry : util::fs::directory_iterator(font_folder))
		{
			auto path = entry.path();
			if (is_font(path))
			{
				// Play around with resource manager.
				auto font_name = path.filename();
				auto font_ptr = std::make_shared<sf::Font>();
				font_ptr->loadFromFile(path);
				desktop.GetEngine().GetResourceManager().AddFont(font_name, font_ptr);
			}
		}
	}

	int run()
	{
		sf::Texture m_background_texture;
		sf::Sprite m_background_sprite;
		sf::Sprite m_canvas_sprite;

		m_background_texture.create(config.width, config.height);

		std::vector<sf::Uint8> pixels(config.width * config.height * 4);

		sf::Uint8 pixel_value = 139;

		for (std::size_t index = 0; index < config.width * config.height; ++index)
		{
			pixel_value = static_cast<sf::Uint8>(pixel_value ^ (index + 809));
			pixel_value = static_cast<sf::Uint8>(pixel_value << (index % 11));
			pixel_value = static_cast<sf::Uint8>(pixel_value * 233);

			pixels[index * 4 + 0] = static_cast<sf::Uint8>(pixel_value % 16 + 72); // R

			pixel_value ^= static_cast<sf::Uint8>(index);
			pixel_value = static_cast<sf::Uint8>(pixel_value * 23);

			pixels[index * 4 + 1] = static_cast<sf::Uint8>(pixel_value % 16 + 72); // G

			pixel_value ^= static_cast<sf::Uint8>(index);
			pixel_value = static_cast<sf::Uint8>(pixel_value * 193);

			pixels[index * 4 + 2] = static_cast<sf::Uint8>(pixel_value % 16 + 72); // B

			pixels[index * 4 + 3] = 255; // A
		}

		m_background_texture.update(pixels.data());
		m_background_sprite.setTexture(m_background_texture);

		sf::Event event;
		std::string renderer_string;

		// Tune Renderer
		auto renderer_name = sfgui.GetRenderer().GetName();
		if (renderer_name == "Non-Legacy Renderer")
		{
			static_cast<sfg::NonLegacyRenderer*>(&sfgui.GetRenderer())->TuneUseFBO(true);
			static_cast<sfg::NonLegacyRenderer*>(&sfgui.GetRenderer())->TuneCull(true);

			renderer_string = "NLR";
		}
		else if (renderer_name == "Vertex Buffer Renderer")
		{
			static_cast<sfg::VertexBufferRenderer*>(&sfgui.GetRenderer())->TuneUseFBO(true);
			static_cast<sfg::VertexBufferRenderer*>(&sfgui.GetRenderer())->TuneAlphaThreshold(.2f);
			static_cast<sfg::VertexBufferRenderer*>(&sfgui.GetRenderer())->TuneCull(true);

			renderer_string = "VBR";
		}
		else if (renderer_name == "Vertex Array Renderer")
		{
			static_cast<sfg::VertexArrayRenderer*>(&sfgui.GetRenderer())->TuneAlphaThreshold(.2f);
			static_cast<sfg::VertexArrayRenderer*>(&sfgui.GetRenderer())->TuneCull(true);

			renderer_string = "VAR";
		}

		// Set desktop properties.

		// We will batch the above properties into this call.
		desktop.SetProperties(
			"Window#second_window > Box > Label {"
			"	FontName: custom_font;"
			"	FontSize: 18;"
			"}"
			"Button#close:Normal {"
			"	Color: #FFFF00FF;"
			"}"
			"#close {"
			"	FontName: resources/fonts/linden_hill.otf;"
			"	FontSize: 15;"
			"}");

		auto element_menu = sfg::Notebook::Create();
		element_menu->SetTabPosition(sfg::Notebook::TabPosition::TOP);

		for (auto& [name, element] : element_types)
		{
			auto element_page = sfg::Table::Create();
			element_page->Attach(sfg::Label::Create("mass"), sf::Rect<sf::Uint32>(0, 0, 1, 1), sfg::Table::FILL, sfg::Table::FILL);
			element_page->Attach(sfg::Label::Create(std::to_string(element.mass)), sf::Rect<sf::Uint32>(1, 0, 1, 1), sfg::Table::FILL, sfg::Table::FILL);
			element_page->SetId(name);
			element_page->SetRowSpacings(5.f);
			element_page->SetColumnSpacings(5.f);
			element_menu->AppendPage(element_page, sfg::Label::Create(name));
		}

		auto element_menu_frame = sfg::Frame::Create(L"Element Type");
		element_menu_frame->Add(element_menu);

		auto buttons_frame = sfg::Frame::Create(L"Buttons");
		auto buttons = sfg::Box::Create();
		auto clear_button = sfg::Button::Create("Clear");
		buttons->Pack(clear_button);
		buttons_frame->Add(buttons);

		// Update the currently selected element
		clear_button->GetSignal(sfg::Notebook::OnLeftClick).Connect([this] {
			this->elements.children.clear();
		});

		auto layout = sfg::Box::Create();
		layout->Pack(element_menu_frame);
		layout->Pack(buttons_frame);
		window_main->Add(layout);

		// Signals

		// Update the currently selected element
		element_menu->GetSignal(sfg::Notebook::OnTabChange).Connect([this, &element_menu] {
			auto current_page = element_menu->GetCurrentPage();
			this->active_element_name = element_menu->GetNthPage(current_page)->GetId();
		});

		canvas->GetSignal(sfg::Canvas::OnLeftClick).Connect([this] {
			auto abs_mouse_position = sf::Vector2f(sf::Mouse::getPosition(this->render_window));
			auto rel_mouse_position = abs_mouse_position - this->canvas->GetAbsolutePosition();
			Element element { this->element_types.at(this->active_element_name) };
			element.setPosition(rel_mouse_position);
			element.drawable.setScale(sf::Vector2f(10, 10));
			this->elements.add(element);
		});

		m_fps_counter = 0;
		m_fps_clock.restart();

		sf::Clock clock;
		sf::Clock frame_time_clock;

		sf::Int64 frame_times[5000];
		std::size_t frame_times_index = 0;

		std::fill(std::begin(frame_times), std::end(frame_times), 0);

		desktop.Update(0.f);
		while (render_window.isOpen())
		{
			while (render_window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
				{
					return EXIT_SUCCESS;
				}

				desktop.HandleEvent(event);
			}

			if (clock.getElapsedTime().asMicroseconds() >= 5000)
			{
				auto dT = float(clock.getElapsedTime().asMicroseconds()) / 1000000.f;
				// Update() takes the elapsed time in seconds.
				desktop.Update(dT);
				// FIXME: Why slow if not * 10!?
				elements.update(dT * 10);

				clock.restart();
			}

			render_window.clear();

			canvas->Bind();
			canvas->Clear(sf::Color(0, 0, 0, 0));

			// Draw elements on canvas
			elements.draw(canvas);
			canvas->Display();
			canvas->Unbind();

			render_window.draw(m_background_sprite);

			render_window.setActive(true);
			sfgui.Display(render_window);
			render_window.display();

			auto frame_time = frame_time_clock.getElapsedTime().asMicroseconds();
			frame_time_clock.restart();

			frame_times[frame_times_index] = frame_time;
			frame_times_index = (frame_times_index + 1) % 5000;

			if (m_fps_clock.getElapsedTime().asMicroseconds() >= 1000000)
			{
				m_fps_clock.restart();

				sf::Int64 total_time = 0;

				for (std::size_t index = 0; index < 5000; ++index)
				{
					total_time += frame_times[index];
				}

				std::stringstream sstr;
				sstr << config.title << " -- " << this->active_element_name << " -- FPS: " << m_fps_counter << " -- Frame Time (microsecs): min: "
					 << *std::min_element(frame_times, frame_times + 5000) << " max: "
					 << *std::max_element(frame_times, frame_times + 5000) << " avg: "
					 << static_cast<float>(total_time) / 5000.f;

				render_window.setTitle(sstr.str());

				m_fps_counter = 0;
			}

			++m_fps_counter;
		}
		return EXIT_SUCCESS;
	}

protected:
	AppConfig config {};

	sf::RenderWindow render_window;

	sfg::SFGUI sfgui {};

	sfg::Window::Ptr window_main;
	sfg::Window::Ptr window_canvas;
	sfg::Canvas::Ptr canvas;

	sfg::Desktop desktop;

	unsigned int m_fps_counter;
	sf::Clock m_fps_clock;

	// FIXME: Find a better way to associate selected element
	std::string active_element_name = "fire";
	const std::unordered_map<std::string, Element> element_types {
		{ "sand", Element { .color = sf::Color::Yellow } },
		{ "grass", Element { .color = sf::Color::Green, .fixed = true } },
		{ "water", Element { .color = sf::Color::Blue } },
		{ "fire", Element { .color = sf::Color::Red, .mass = -1.5 } },
	};

	ElementContainer elements {};
};