#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

struct Element
{
	// Color of pixel
	sf::Color color { 255, 255, 255 };

	// The total net velocity in m/s
	sf::Vector2f velocity { 0, 0 };
	// The total net acceleration in m/s^2
	sf::Vector2f acceleration { 0, -9.81 };
	// This blocks net mass in kilograms
	float mass { 1 };

	// False if block should move on next render
	bool fixed { false };
	// True if this block should render
	bool visible { true };

	bool canMove(const sf::Vector2f&)
	{
		return true;
	}
};

struct ElementInstance : public Element, sf::Transformable
{
	// Update block physics
	void update(double dT)
	{
		if (!this->fixed)
		{
			// Calculate change in velocity
			auto dV = sf::Vector2f(this->acceleration.x * dT, this->acceleration.y * dT);
			auto next_velocity = this->velocity + dV;

			auto dPos = sf::Vector2f(next_velocity.x * dT, next_velocity.y * dT);
			auto next_pos = this->getPosition() + dPos;
			if (this->canMove(next_pos))
			{
				this->velocity = next_velocity;
				this->setPosition(next_pos);
			}
		}
	}

	// Render a block on screen
	virtual void draw(sf::RenderTarget& target, const sf::Transform& transform) const
	{
		if (this->visible)
		{
			sf::RectangleShape rect(sf::Vector2f(1, 1));
			rect.setFillColor(this->color);

			const auto display_coords = this->getPosition();
			rect.setPosition(display_coords);
			target.draw(rect, transform);
		}
	}
};

struct ElementContainer : sf::Transformable
{
	ElementContainer()
	{
		this->children.reserve(1000);
	}

	// draw to current target
	template <typename... Args>
	auto add(Args&&... args)
	{
		return this->children.emplace_back(std::make_shared<Element>(std::forward<Args>(args)...));
	}

	void draw(sf::RenderTarget& target, const sf::Transform& parentTransform) const
	{
		// combine the parent transform with the node's one
		sf::Transform combinedTransform = parentTransform * this->getTransform();
		// draw its children
		for (auto child : this->children)
		{
			child->draw(target, combinedTransform);
		}
	}

	void update(double dT)
	{
		// draw its children
		for (auto child : this->children)
		{
			child->update(dT);
		}
	}

	std::vector<std::shared_ptr<ElementInstance>> children;
};
