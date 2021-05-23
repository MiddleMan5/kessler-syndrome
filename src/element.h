#pragma once

#include <SFGUI/Canvas.hpp>
#include <SFML/Graphics.hpp>
#include <vector>

#include <iterator>
#include <random>

template <typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g)
{
	std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
	std::advance(start, dis(g));
	return start;
}

template <typename Iter>
Iter select_randomly(Iter start, Iter end)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return select_randomly(start, end, gen);
}

template <typename RandomGenerator>
bool random_bool(RandomGenerator& g)
{
	std::uniform_int_distribution<> dis(0, 1);
	return bool(dis(g));
}

bool random_bool()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return random_bool(gen);
}

struct Element;
using ElementList = std::vector<std::shared_ptr<Element>>;

struct Element
{
	// Color of pixel
	sf::Color color { 255, 255, 255 };

	// The total net velocity in m/s
	sf::Vector2f velocity { 0, 0 };

	// This blocks net mass in kilograms
	float mass { 1 };

	// False if block should move on next render
	bool fixed { false };
	// True if this block should render
	bool visible { true };

	// Holds transformable properties such as position and scale
	sf::RectangleShape drawable { sf::Vector2f(1, 1) };

	bool canMove(const sf::Vector2f& next_pos, ElementList& elements)
	{
		auto current_pos = this->drawable.getPosition();
		auto next_draw = sf::RectangleShape(this->drawable);
		next_draw.setPosition(next_pos);

		for (auto element : elements)
		{
			// FIXME: Use element id's instead
			if (element->drawable.getPosition() != current_pos)
			{
				if (element->drawable.getGlobalBounds().intersects(next_draw.getGlobalBounds()))
				{
					return false;
				}
			}
		}
		return true;
	}

	// All beacause sf::Transformable defines a useless explicit default ctor...
	template <typename... Args>
	auto setPosition(Args&&... args)
	{
		return this->drawable.setPosition(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto getPosition(Args&&... args)
	{
		return this->drawable.getPosition(std::forward<Args>(args)...);
	}

	// Update block physics
	void update(double dT, ElementList& elements)
	{
		if (!this->fixed)
		{

			sf::Vector2f initial_force { 0, 0 };
			std::vector<sf::Vector2f> forces {
				// fGrav
				{ 0, 9.81f * std::abs(this->mass) }
			};
			auto net_force = std::accumulate(forces.begin(), forces.end(), initial_force);

			// Calculate change in velocity
			auto dV = sf::Vector2f((net_force.x * dT) / this->mass, (net_force.y * dT) / this->mass);
			auto next_velocity = this->velocity + dV;

			auto dPos = sf::Vector2f(next_velocity.x * dT, next_velocity.y * dT);
			auto next_pos = this->getPosition() + dPos;
			if (this->canMove(next_pos, elements))
			{
				this->velocity = next_velocity;
				this->setPosition(next_pos);
			}
			else
			{
				// Half velocity to left and right
				auto checkDir = [&](float dir) {
					auto next_v = this->velocity + sf::Vector2f(dV.x + dir * (dV.y / 2), 0);
					auto next_p = this->getPosition() + sf::Vector2f(next_v.x * dT, next_v.y * dT);
					if (this->canMove(next_p, elements))
					{
						this->velocity = next_v;
						this->setPosition(next_p);
						return true;
					}
					return false;
				};

				const auto left_first = random_bool();
				if (checkDir(left_first ? -1 : 1))
				{
				}
				else if (checkDir(left_first ? 1 : -1))
				{
				}
				else
				{
					this->velocity = sf::Vector2f { 0, 0 };
				}
			}
		}
	}

	// Render a block on screen
	void draw(sfg::Canvas::Ptr canvas)
	{
		if (this->visible)
		{
			this->drawable.setFillColor(this->color);
			canvas->Draw(this->drawable);
		}
	}
};

struct ElementContainer
{
	// draw to current target
	template <typename... Args>
	auto add(Args&&... args)
	{
		return this->children.emplace_back(std::make_shared<Element>(std::forward<Args>(args)...));
	}

	void draw(sfg::Canvas::Ptr canvas) const
	{
		// draw its children
		for (auto child : this->children)
		{
			child->draw(canvas);
		}
	}

	void update(double dT)
	{
		// draw its children
		for (auto child : this->children)
		{
			child->update(dT, this->children);
		}
	}

	ElementList children {};
};
