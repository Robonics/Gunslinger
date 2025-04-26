#pragma once

#include "SFML/Graphics/RenderStates.hpp"
#include "phys_object.hpp"
#include <SFML/Graphics.hpp>

#define MOVER_ITERATIONS 20

namespace Arcade {

	enum CollisionGroups {
		GROUP_World=1,
		GROUP_Player=1<<1,
		GROUP_Enemy=1<<2,
		GROUP_Prop=1<<3
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

		virtual void debug() const;
		virtual void tick( sf::Time dt )=0;
	public:

		virtual constexpr const char* GET_NAME() const {return "BaseEnt";}

		virtual sf::Vector2f getPosition() const;
		virtual void setPosition( sf::Vector2f n_pos );
		virtual sf::Angle getRotation() const;
		virtual void setRotation( sf::Angle n_ang );
	};
	class RenderableEntity : sf::Drawable, public Entity {
		friend class Engine;

	protected:
		RenderableEntity()=delete;
		RenderableEntity( Engine& eng );

		virtual void draw( sf::RenderTarget&, sf::RenderStates ) const override = 0;
	public:
		virtual constexpr const char* GET_NAME() const override {return "RenderableEnt";}
	};

	class PhysicsEntity : public RenderableEntity, public PhysObject {
		friend class Engine;

	protected:
		virtual void tick( sf::Time dt ) override;

		PhysicsEntity()=delete;
		PhysicsEntity( Engine& eng, b2BodyDef def, PhysSettings settings[], size_t n_shape );

		virtual void draw( sf::RenderTarget&, sf::RenderStates ) const override;
	public:
		virtual constexpr const char* GET_NAME() const override {return "PhysicsEnt";}

		sf::Vector2f getPosition() const override;
		sf::Angle getRotation() const override;
		void setPosition( sf::Vector2f n_pos ) override;
		void setRotation( sf::Angle n_ang ) override;
	};

	class MoverEntity : public RenderableEntity {
		b2Capsule collider;
		b2Vec2 velocity{0,0};
		b2QueryFilter collision_mask;
		bool grounded{true};

		sf::Vector2f bb_size;

		float acceleration;
		float move_speed;
		float jump_power;

		struct DebugInfo {
			unsigned int iterations;
			b2Vec2 translation;
			b2Vec2 sp_position;
			float fraction;
			sf::Vector2f delta;
		} debug_info;

	protected:
		virtual void tick( sf::Time dt ) override;
		virtual void draw( sf::RenderTarget&, sf::RenderStates ) const override;
		MoverEntity()=delete;
		MoverEntity(Engine&, b2Vec2);

		virtual void debug() const override;

	public:
	virtual constexpr const char* GET_NAME() const override {return "MoverEnt";}

		void setMoveSpeed( float n_speed );
		float getMoveSpeed() const;
		void setJumpPower( float n_power );
		float getJumpPower() const;
		void setAcceleration( float n_accel );
		float getAcceleration() const;
	};
}