#include "input.h"

void InputManager::initEvents()
{
}

void InputManager::processEvents()
{
	sf::Vector2i mousePosition = sf::Mouse::getPosition(this->window);

	sf::Event event;
	while (this->window.pollEvent(event))
	{

		switch (event.type)
		{
			case sf::Event::Closed:
				this->window.close();
				break;
			case sf::Event::KeyPressed:
				switch (event.key.code)
				{
					case sf::Keyboard::Escape:
						this->window.close();
						break;
				}
				break;
			case sf::Event::MouseWheelMoved: {
				auto view = this->window.getView();
				view.zoom(1 + event.mouseWheel.delta * -0.2f);
				this->window.setView(view);
				break;
			}
			default:
				break;
		}

		// Process registered callbacks
		auto& callbacks = this->event_callbacks[event.type];
		for (auto& callback : callbacks)
		{
			callback(event);
		}
	}
}
