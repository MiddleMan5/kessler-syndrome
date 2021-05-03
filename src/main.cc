#include "input.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <fstream>
#include <list>
#include <vector>

#include "Platform/Platform.hpp"
#include "particle.h"

#if defined(__linux__)
	#include <X11/Xlib.h>
#endif

struct AppConfig
{
	// Window settings
	uint32_t width { 800 };
	uint32_t height { 400 };
	uint32_t frame_rate { 120 };
	std::string title = "Kessler Syndrome";
};

static AppConfig loadUserConf(std::filesystem::path path)
{
	std::ifstream conf_file(path.c_str());
	AppConfig config {};
	if (conf_file)
	{
		conf_file >> config.width;
		conf_file >> config.height;
		conf_file >> config.frame_rate;
	}
	else
	{
		std::cout << "Couldn't find " << path << ", loading default" << std::endl;
	}
	return config;
}

struct ThreadArgs
{
	sf::RenderWindow& window;
	InputManager& input_manager;
	ParticleContainer& particle_container;
};

static void renderingThread(ThreadArgs& args)
{
	sf::Font font;
	font.loadFromFile("/usr/share/fonts/truetype/msttcorefonts/arial.ttf");
	sf::Text text;
	text.setFont(font);

	const auto drawWall = [&](sf::RectangleShape rect) {
		rect.setFillColor(sf::Color::Green);
		args.window.draw(rect);
	};

	const auto drawBackground = [&]() {
		const auto window_size = args.window.getSize();
		sf::RectangleShape background { sf::Vector2f(window_size.x, window_size.y) };
		background.setFillColor(sf::Color::Black);
		args.window.draw(background);
	};

	const auto drawStats = [&]() {
		auto& blocks = args.particle_container.children;
		auto selected_block = args.particle_container.children[0];
		auto selected_velocity = selected_block == nullptr ? sf::Vector2f(-1, -1) : selected_block->velocity;

		std::string stats[] = {
			{ "Block Count: " + std::to_string(blocks.size()) },
			{ "Block[0] Velocity X: " + std::to_string(selected_velocity.x) + " Y: " + std::to_string(selected_velocity.y) },
		};

		// Build stats string
		std::string stats_message {};
		for (auto line : stats)
		{
			stats_message += line;
			stats_message += "\n";
		}
		// set the string to display
		text.setString(stats_message);
		text.setCharacterSize(12);
		text.setFillColor(sf::Color::White);
		args.window.draw(text);
	};

	// the rendering loop
	while (args.window.isOpen())
	{
		args.window.clear(sf::Color(94, 87, 87));
		drawBackground();
		args.particle_container.draw(args.window, sf::Transform());
		drawStats();
		args.window.display();
	}
}

static void physicsThread(ThreadArgs& args)
{
	using clock = std::chrono::high_resolution_clock;
	auto last_loop_time = clock::now();
	// the rendering loop
	while (args.window.isOpen())
	{
		auto now = clock::now();
		auto duration = now - last_loop_time;
		last_loop_time = now;
		args.particle_container.update(double(duration.count()) / double(clock::duration::period::den));
	}
}

int main()
{
#if defined(__linux__)
	XInitThreads();
#endif
	const auto config = loadUserConf("conf.txt");

	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;

	sf::RenderWindow window(sf::VideoMode(config.width, config.height), config.title.c_str(), sf::Style::Default, settings);
	//sf::View view{};

	window.setFramerateLimit(config.frame_rate);
	window.setActive(false);

	InputManager input_manager { window };
	ParticleContainer particle_container;

	// Handle mouse clicks
	input_manager.registerMouseButtonEvent([&](sf::Event event) {
		if (event.type == sf::Event::MouseButtonPressed)
		{
			if (event.mouseButton.button == sf::Mouse::Button::Left)
			{
				const auto mouse_position = sf::Mouse::getPosition(window);
				const auto world_coords = window.mapPixelToCoords(mouse_position);
				Particle2D new_block {};
				new_block.setPosition(world_coords);
				std::cout << "Creating new block: X:" << world_coords.x << " Y:" << world_coords.y << std::endl;
				particle_container.add(new_block);
			}
		}
	});

	// launch the rendering thread
	ThreadArgs args { window, input_manager, particle_container };
	sf::Thread render_thread(&renderingThread, args);
	sf::Thread physics_thread(&physicsThread, args);
	render_thread.launch();
	physics_thread.launch();

	while (window.isOpen())
	{
		input_manager.processEvents();
	}
	return 0;
}

#if defined(_WIN32)
	#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline,
	int cmdshow)
{
	return main();
}
#endif