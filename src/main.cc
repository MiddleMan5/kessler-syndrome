#include <SFML/Graphics.hpp>

#include <filesystem>
#include <fstream>
#include <list>
#include <vector>

#include "application.h"
#include "config.h"
#include "platform/Platform.hpp"

#if defined(__linux__)
	#include <X11/Xlib.h>
#endif

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
