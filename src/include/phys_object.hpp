#pragma once

#include <box2d/box2d.h>
#include <SFML/System.hpp>
#include <variant>

namespace Arcade {
	struct PhysSettings {

		b2ShapeType type;
		b2ShapeDef shape;
		std::variant<
			b2Polygon,
			b2Circle,
			b2Capsule,
			b2Segment,
			b2ChainSegment
		> shape_data;


	};
	class PhysObject {
		friend class Engine;
		b2BodyId b2_id;

	protected:
		PhysObject() = delete;
		PhysObject( b2WorldId world, b2BodyDef def, PhysSettings settings[], size_t n_shape );
		~PhysObject();
	public:

		const b2BodyId getID() const;
		virtual sf::Vector2f getPosition() const;
		sf::Vector2f getVelocity() const;
		virtual sf::Angle getRotation() const;
		sf::Angle getAngularVelocity() const;
		b2AABB getAABB() const;

		void applyImpulse( sf::Vector2f impulse );
		void applyImpulseAt( sf::Vector2f impulse, sf::Vector2f local_pos );
		void applyAngularImpulse( sf::Angle impulse );


		virtual void setPosition( sf::Vector2f pos );
		void setVelocity( sf::Vector2f vel );
		virtual void setRotation( sf::Angle rotation );
		void setAngularVelocity( sf::Angle r_vels );
	};
};