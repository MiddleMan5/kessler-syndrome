#include "input.h"
#include <SFML/Graphics.hpp>

#include <filesystem>
#include <fstream>
#include <list>
#include <vector>

#include "application.h"
#include "config.h"
#include "particle.h"
#include "platform/Platform.hpp"

#if defined(__linux__)
	#include <X11/Xlib.h>
#endif

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
	sfg::SFGUI sfgui;
#if defined(__linux__)
	XInitThreads();
#endif
	const auto config_path = util::fs::path("configs/config.txt");
	const auto config = AppConfig::loadFile(config_path);
	Application app { config };
	return app.run();
}
