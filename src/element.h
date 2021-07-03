#pragma once

#include <SFGUI/Canvas.hpp>
#include <SFML/Graphics.hpp>
#include <vector>

#include "./uuid.h"
#include "quadtree/quadtree.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iterator>
#include <memory>
#include <random>
#include <ranges>
#include <type_traits>
#include <vector>

template <typename Iter, typename RandomGenerator>
static Iter select_randomly(Iter start, Iter end, RandomGenerator& g)
{
	std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
	std::advance(start, dis(g));
	return start;
}

template <typename Iter>
static Iter select_randomly(Iter start, Iter end)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return select_randomly(start, end, gen);
}

template <typename RandomGenerator>
static bool random_bool(RandomGenerator& g)
{
	std::uniform_int_distribution<> dis(0, 1);
	return bool(dis(g));
}

static bool random_bool()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return random_bool(gen);
}

static sf::Vector2f DEFAULT_POSITION { std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };

struct Element;
class ElementTree;
using ElementList = std::vector<Element>;

struct Element
{
	using Ptr = std::shared_ptr<Element>;
	using Shape = sf::RectangleShape;

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
	Element::Shape shape { sf::Vector2f(1, 1) };

	// A unique element id
	uuid::uuid4 id = uuid::generate_uuid_v4();

	bool collides(Element element, sf::Vector2f position = DEFAULT_POSITION) const
	{
		auto next_draw = Element::Shape(this->shape);
		next_draw.setPosition(position == DEFAULT_POSITION ? this->shape.getPosition() : position);
		return element.shape.getGlobalBounds().intersects(next_draw.getGlobalBounds());
	}

	bool canMove(const sf::Vector2f& next_pos, const std::vector<Element*>& elements)
	{
		auto not_self = [this](const Element* e) { return e->id != this->id; };
		for (auto element : elements | std::views::filter(not_self))
		{
			if (this->collides(*element, next_pos))
			{
				if (element->fixed)
				{
					return false;
				}
				else
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
		return this->shape.setPosition(std::forward<Args>(args)...);
	}

	auto getPosition() const
	{
		return this->shape.getPosition();
	}

	// Update block physics
	void update(double dT, const std::vector<Element*>& elements)
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
				// Check if stuck
				else if (!this->canMove(this->getPosition(), elements))
				{
					this->velocity = next_velocity;
					this->setPosition(next_pos);
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
			this->shape.setFillColor(this->color);
			canvas->Draw(this->shape);
		}
	}
};

static quadtree::Box<float> getElementBox(Element const& element)
{
	return element.shape.getGlobalBounds();
}

static quadtree::Box<float> MAX_SIZE { sf::Vector2f { 0, 0 }, sf::Vector2f { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() } };

static bool operator==(Element const& lhs, Element const& rhs) noexcept
{
	return lhs.id == rhs.id;
}

class ElementTree : public quadtree::Quadtree<Element, decltype(getElementBox)*>
{
public:
	using BoxType = Element::Shape;

	decltype(MAX_SIZE) screen_size = MAX_SIZE;

	ElementTree(decltype(MAX_SIZE) world_size = MAX_SIZE) :
		quadtree::Quadtree<Element, decltype(getElementBox)*>(world_size, getElementBox)
	{
		screen_size = world_size;
	}

	auto emplace(const Element& value)
	{
		Element el { value };
		el.id = uuid::generate_uuid_v4();
		return this->add(mRoot.get(), 0, mBox, el);
	}

	std::size_t size() const
	{
		auto children = this->query(MAX_SIZE);
		return children.size();
	}

	void clear()
	{
		auto children = this->query(MAX_SIZE);
		for (auto child : children)
		{
			this->remove(child);
		}
	}

	bool show_bounds = false;
	bool show_collisions = false;
	bool collide_all = false;
	float min_search_size = 2.0;

	auto getSearchWindowForElement(const Element& element)
	{
		// FIXME: This is ridiculous
		auto search_box { element.shape };
		const auto scale = search_box.getScale();
		const auto size = element.shape.getSize();
		const auto pos = element.shape.getPosition();
		const auto bounds = element.shape.getGlobalBounds();
		search_box.setOrigin(sf::Vector2f { size.x / 2.0f, size.y / 2.0f });
		search_box.setPosition(sf::Vector2f { pos.x + (bounds.width / 2.0f), pos.y + (bounds.height / 2.0f) });
		search_box.setScale(sf::Vector2f { scale.x * std::max(min_search_size, std::abs(element.velocity.x)), scale.y * std::max(min_search_size, std::abs(element.velocity.y)) });
		return search_box;
	}

	auto accessNeighbors(const Element& element)
	{
		return this->access(getSearchWindowForElement(element).getGlobalBounds());
	}

	void draw(sfg::Canvas::Ptr canvas)
	{
		auto children = this->access(screen_size);

		auto intersection_pairs = this->findAllIntersections();
		std::map<decltype(Element::id), Element*> intersections {};
		for (auto& pair : intersection_pairs)
		{
			for (auto& child : { pair.first, pair.second })
			{
				if (!intersections.contains(child.id))
				{
					auto found = std::find_if(children.begin(), children.end(), [&child](const auto& e) { return e->id == child.id; });
					if (found != children.end())
					{
						intersections[child.id] = *found;
					}
				}
			}
		}
		for (auto& child : children)
		{

			auto search_box = getSearchWindowForElement(*child);
			if (show_bounds)
			{
				search_box.setOutlineThickness(0.1);
				search_box.setOutlineColor(sf::Color::Red);
				search_box.setFillColor(sf::Color::Transparent);
				canvas->Draw(search_box);
			}

			if (show_collisions && intersections.contains(child->id))
			{
				auto orig_color = child->color;
				child->color = sf::Color::White;
				child->draw(canvas);
				child->color = orig_color;
			}
			else
			{
				child->draw(canvas);
			}
		}
	}

	void update(double dT)
	{

		auto children = this->access(screen_size);
		auto intersections = this->findAllIntersections();
		// For every child, only pass nearest neighbors for collision detection
		for (auto& child : children)
		{
			// Only update moveable elements
			if (!child->fixed)
			{
				child->update(dT, collide_all ? children : this->accessNeighbors(*child));
			}
		}
	}
};