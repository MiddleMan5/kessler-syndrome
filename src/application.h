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
		// We're not using SFML to render anything in this program, so reset OpenGL
		// states. Otherwise we wouldn't see anything.
		render_window.resetGLStates();

		window_main = sfg::Window::Create(sfg::Window::Style::TITLEBAR | sfg::Window::Style::BACKGROUND | sfg::Window::Style::CLOSE);
		window_main->SetTitle(config.title);

		loadFonts();
		configure();
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

		auto material_menu_frame = sfg::Frame::Create(L"Pixel Type");
		auto material_menu = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);

		const std::map<std::string, Element> element_types {
			{ "sand", Element { .color = sf::Color::Yellow } },
			{ "sand", Element { .color = sf::Color::Yellow } },
		};

		auto material_props_table = sfg::Table::Create();
		std::vector<std::string> prop_names { "Mass", "Color", "Scale" };
		for (std::size_t i { 0 }; i < prop_names.size(); i++)
		{
			auto& name = prop_names[i];
			material_props_table->Attach(sfg::Label::Create(name), sf::Rect<sf::Uint32>(0, i, 1, 1), sfg::Table::FILL, sfg::Table::FILL);
			material_props_table->Attach(sfg::Label::Create("Value"), sf::Rect<sf::Uint32>(1, i, 1, 1), sfg::Table::FILL, sfg::Table::FILL);
		}
		material_props_table->SetRowSpacings(5.f);
		material_props_table->SetColumnSpacings(5.f);

		material_menu->Pack(material_props_table);

		material_menu->SetSpacing(5.f);

		auto block_selector = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);

		for (std::size_t i { 0 }; i < prop_names.size(); i++)
		{
			auto& name = prop_names[i];
			material_props_table->Attach(sfg::Label::Create(name), sf::Rect<sf::Uint32>(0, i, 1, 1), sfg::Table::FILL, sfg::Table::FILL);
			material_props_table->Attach(sfg::Label::Create("Value"), sf::Rect<sf::Uint32>(1, i, 1, 1), sfg::Table::FILL, sfg::Table::FILL);
		}

		material_menu_frame->Add(sfg::Separator::Create(sfg::Separator::Orientation::HORIZONTAL));
		material_menu_frame->Add(material_menu);
		window_main->Add(material_menu);

		m_scrolled_window = sfg::ScrolledWindow::Create();
		m_scrolled_window->SetRequisition(sf::Vector2f(.0f, 160.f));
		m_scrolled_window->SetScrollbarPolicy(sfg::ScrolledWindow::HORIZONTAL_AUTOMATIC | sfg::ScrolledWindow::VERTICAL_AUTOMATIC);
		m_scrolled_window->SetPlacement(sfg::ScrolledWindow::Placement::TOP_LEFT);
		//m_scrolled_window->AddWithViewport(m_scrolled_window_box);

		auto scrollbar = sfg::Scrollbar::Create();
		scrollbar->SetRange(.0f, 100.f);

		m_scale = sfg::Scale::Create();
		m_scale->SetAdjustment(scrollbar->GetAdjustment());
		m_scale->SetRequisition(sf::Vector2f(100.f, .0f));

		m_combo_box = sfg::ComboBox::Create();

		for (int index = 0; index < 30; ++index)
		{
			std::stringstream sstr;

			sstr << "Item " << index;

			m_combo_box->AppendItem(sstr.str());
		}

		m_switch_renderer = sfg::Button::Create("Renderer: " + renderer_string);

		auto frame2 = sfg::Frame::Create(L"Toolbar 2");
		frame2->SetAlignment(sf::Vector2f(.8f, .0f));

		auto separatorh = sfg::Separator::Create(sfg::Separator::Orientation::HORIZONTAL);

		auto box_image = sfg::Box::Create(sfg::Box::Orientation::HORIZONTAL);
		box_image->SetSpacing(15.f);

		auto fixed_container = sfg::Fixed::Create();
		auto fixed_button = sfg::Button::Create(L"I'm at (34,61)");
		fixed_container->Put(fixed_button, sf::Vector2f(34.f, 61.f));
		box_image->Pack(fixed_container, false);

		sf::Image sfgui_logo;
		m_image = sfg::Image::Create();

		if (sfgui_logo.loadFromFile("resources/img/logo.png"))
		{
			m_image->SetImage(sfgui_logo);
			box_image->Pack(m_image, false);
		}

		auto mirror_image = sfg::Button::Create(L"Mirror Image");

		box_image->Pack(mirror_image, false);

		auto spinner_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);

		m_spinner = sfg::Spinner::Create();
		m_spinner->SetRequisition(sf::Vector2f(40.f, 40.f));
		m_spinner->Start();
		auto spinner_toggle = sfg::ToggleButton::Create(L"Spin");
		spinner_toggle->SetActive(true);
		spinner_box->SetSpacing(5.f);
		spinner_box->Pack(m_spinner, false);
		spinner_box->Pack(spinner_toggle, false);

		box_image->Pack(spinner_box, false);

		auto radio_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);

		auto radio1 = sfg::RadioButton::Create("Radio 1");
		auto radio2 = sfg::RadioButton::Create("Radio 2", radio1->GetGroup());
		auto radio3 = sfg::RadioButton::Create("Radio 3", radio2->GetGroup());

		radio_box->Pack(radio1);
		radio_box->Pack(radio2);
		radio_box->Pack(radio3);

		box_image->Pack(radio_box, false);

		auto spinbutton = sfg::SpinButton::Create(scrollbar->GetAdjustment());
		spinbutton->SetRequisition(sf::Vector2f(80.f, 0.f));
		spinbutton->SetDigits(3);

		auto spinbutton_box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		spinbutton_box->Pack(spinbutton, false, false);

		box_image->Pack(spinbutton_box, false, false);

		auto aligned_combo_box = sfg::ComboBox::Create();
		aligned_combo_box->AppendItem(L"I'm way over here");
		aligned_combo_box->AppendItem(L"Me too");
		aligned_combo_box->AppendItem(L"Me three");
		aligned_combo_box->SelectItem(0);

		auto alignment = sfg::Alignment::Create();
		alignment->Add(aligned_combo_box);
		box_image->Pack(alignment, true);
		alignment->SetAlignment(sf::Vector2f(1.f, .5f));
		alignment->SetScale(sf::Vector2f(0.f, .01f));

		auto boxmain = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
		boxmain->SetSpacing(5.f);
		boxmain->Pack(scrollbar, false);
		//boxmain->Pack(m_progress, false);
		//boxmain->Pack(material_menu, false);
		boxmain->Pack(frame2, false);
		//boxmain->Pack(m_boxbuttonsh, false);
		//boxmain->Pack(m_boxbuttonsv, false);
		boxmain->Pack(box_image, true);
		boxmain->Pack(separatorh, false);
		//boxmain->Pack(m_table, true);
		//boxmain->Pack(m_boxorientation, true);
		boxmain->Pack(m_scrolled_window);

		// Signals.
		// window_main->GetSignal(sfg::Window::OnCloseButton).Connect([this] { OnHideWindowClicked(); });
		// btnaddbuttonh->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnAddButtonHClick(); });
		// btnaddbuttonv->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnAddButtonVClick(); });
		// m_titlebar_toggle->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnToggleTitlebarClick(); });
		// btnhidewindow->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnHideWindowClicked(); });
		// btntoggleori->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnToggleOrientationClick(); });
		// btntogglespace->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnToggleSpaceClick(); });
		// m_limit_check->GetSignal(sfg::ToggleButton::OnToggle).Connect([this] { OnLimitCharsToggle(); });
		// btnloadstyle->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnLoadThemeClick(); });
		// m_scale->GetAdjustment()->GetSignal(sfg::Adjustment::OnChange).Connect([this] { OnAdjustmentChange(); });
		// spinner_toggle->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnToggleSpinner(); });
		// mirror_image->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnMirrorImageClick(); });
		// m_switch_renderer->GetSignal(sfg::Widget::OnLeftClick).Connect([this] { OnSwitchRendererClick(); });

		window_main->SetPosition(sf::Vector2f(100.f, 100.f));

		// Add window to desktop
		desktop.Add(window_main);

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

			render_window.draw(m_background_sprite);

			auto microseconds = clock.getElapsedTime().asMicroseconds();

			desktop.Update(static_cast<float>(microseconds) / 1000000.f);
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
				sstr << config.title << " -- FPS: " << m_fps_counter << " -- Frame Time (microsecs): min: "
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
	sfg::Box::Ptr m_boxbuttonsh;
	sfg::Box::Ptr m_boxbuttonsv;
	sfg::Box::Ptr m_boxorientation;
	sfg::Entry::Ptr m_entry;
	sfg::Table::Ptr m_table;
	sfg::ScrolledWindow::Ptr m_scrolled_window;
	sfg::Box::Ptr m_scrolled_window_box;
	sfg::ToggleButton::Ptr m_titlebar_toggle;
	sfg::CheckButton::Ptr m_limit_check;
	sfg::Scale::Ptr m_scale;
	sfg::ComboBox::Ptr m_combo_box;
	sfg::ProgressBar::Ptr m_progress;
	sfg::ProgressBar::Ptr m_progress_vert;
	sfg::Spinner::Ptr m_spinner;
	sfg::Image::Ptr m_image;
	sfg::Canvas::Ptr m_gl_canvas;
	//sfg::Canvas::Ptr m_sfml_canvas;
	sfg::Button::Ptr m_switch_renderer;

	sfg::Desktop desktop;

	unsigned int m_fps_counter;
	sf::Clock m_fps_clock;
};