#include <SFML/Graphics.hpp>
#include <vector>

struct Particle2D : sf::Transformable
{
	// Color of pixel
	sf::Color color { 255, 255, 255 };

	// The total net velocity in m/s
	sf::Vector2f velocity { 0, 0 };
	// The total net acceleration in m/s^2
	sf::Vector2f acceleration { 0, -9.81 };
	// This blocks net mass in kilograms
	float mass { 1 };

	// Width and height of Block
	uint32_t scale { 10 };
	// False if block should move on next render
	bool fixed { false };
	// True if this block should render
	bool visible { true };

	bool canMove(const sf::Vector2f&)
	{
		return true;
	}

	// Update block physics
	virtual void update(double dT)
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
			const auto scale_factor = this->scale;
			rect.setPosition(display_coords);
			rect.setScale(scale_factor, scale_factor);
			target.draw(rect, transform);
		}
	}
};

struct ParticleContainer : sf::Transformable
{
	ParticleContainer()
	{
		this->children.reserve(1000);
	}

	// draw to current target
	template <typename... Args>
	auto add(Args&&... args)
	{
		return this->children.emplace_back(std::make_shared<Particle2D>(std::forward<Args>(args)...));
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

	std::vector<std::shared_ptr<Particle2D>> children;
};
