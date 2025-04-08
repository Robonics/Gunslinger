#pragma once

#include "SFML/Graphics/RenderStates.hpp"
#include "phys_object.hpp"
#include <SFML/Graphics.hpp>

namespace Arcade {

	enum CollisionGroups {
		World=1,
		Player=1<<1,
		Enemy=1<<2,
		Prop=1<<3
	};

	class Engine;
	class Entity {
		friend class Engine;
		sf::Vector2f world_pos;
		sf::Angle rotation;
		
	protected:
		Arcade::Engine& engine;
		Entity()=delete;
		Entity( Arcade::Engine& eng );
		~Entity();

		virtual void tick( sf::Time dt )=0;
	public:
		virtual sf::Vector2f getPosition();
		virtual void setPosition( sf::Vector2f n_pos );
		virtual sf::Angle getRotation();
		virtual void setRotation( sf::Angle n_ang );
	};
	class RenderableEntity : sf::Drawable, public Entity {
		friend class Engine;

	protected:
		RenderableEntity()=delete;
		RenderableEntity( Engine& eng );

		virtual void draw( sf::RenderTarget&, sf::RenderStates ) const = 0;
	};

	class PhysicsEntity : public RenderableEntity, public PhysObject {
		friend class Engine;

	protected:
		virtual void tick( sf::Time dt ) override;

		PhysicsEntity()=delete;
		PhysicsEntity( Engine& eng, b2BodyDef def, PhysSettings settings[], size_t n_shape );

		virtual void draw( sf::RenderTarget&, sf::RenderStates ) const override;
	public:
		sf::Vector2f getPosition() const override;
		sf::Angle getRotation() const override;
		void setPosition( sf::Vector2f n_pos ) override;
		void setRotation( sf::Angle n_ang ) override;
	};
}