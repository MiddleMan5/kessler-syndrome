#pragma once

#include <SFML/Graphics.hpp>

using Event = sf::Event;
using EventType = Event::EventType;
template <typename event_type = Event>
using InputCallback = std::function<void(event_type)>;
using EventCallback = InputCallback<Event>;
using CallbackMap = std::map<EventType, std::vector<EventCallback>>;

class InputManager
{
public:
	InputManager(sf::RenderWindow& window) :
		window { window }
	{
	}

	// draw to current target
	template <typename... Args>
	auto draw(Args&&... args) const
	{
		return this->window.draw(std::forward<Args>(args)...);
	}

	template <EventType event_t>
	void registerEvent(EventCallback callback)
	{
		this->event_callbacks[event_t].push_back(callback);
	}

	void registerMouseButtonEvent(InputCallback<Event> callback)
	{
		auto wrapMouseEvent = [callback](Event event) {
			callback(event);
		};
		this->registerEvent<EventType::MouseButtonPressed>(wrapMouseEvent);
		this->registerEvent<EventType::MouseButtonReleased>(wrapMouseEvent);
	}

	// Process input events
	void processEvents()
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

protected:
	sf::RenderWindow& window;
	CallbackMap event_callbacks;

	void initEvents()
	{
	}
};
