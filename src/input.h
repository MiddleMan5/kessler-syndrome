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

	// Process input events
	void processEvents();

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

	sf::RenderWindow& window;

protected:
	bool paused { false };
	CallbackMap event_callbacks;

	void initEvents();
};