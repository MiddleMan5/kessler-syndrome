#pragma once

#include "platform/Platform.hpp"
#include <filesystem>
#include <fstream>
#include <list>
#include <vector>

struct AppConfig
{
	// Window settings
	uint32_t width { 800 };
	uint32_t height { 400 };
	uint32_t frame_rate { 120 };
	std::string title = "Kessler Syndrome";
	sf::ContextSettings renderSettings { 0, 0, 4 };

	static AppConfig loadFile(util::fs::path path)
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
};
