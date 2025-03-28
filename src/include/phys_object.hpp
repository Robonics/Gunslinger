#pragma once

#include <box2d/box2d.h>
#include <SFML/System.hpp>
#include <variant>

namespace Arcade {
	struct PhysSettings {
		b2BodyDef def = b2DefaultBodyDef();

		size_t shape_count;
		b2ShapeDef* shapes;
		std::variant<
			b2Polygon,
			b2Circle,
			b2Capsule,
			b2Segment,
			b2ChainSegment
		>* shape_data;

		b2ShapeType type;

	};
	class PhysObject {
		b2BodyId b2_id;

	protected:
		PhysObject() = delete;
		PhysObject( b2WorldId world, PhysSettings& settings );
		~PhysObject();
	public:

		const b2BodyId getID() const;
		sf::Vector2f getPosition() const;
		sf::Vector2f getVelocity() const;
		sf::Angle getRotation() const;
		sf::Angle getAngularVelocity() const;

		void applyImpulse( sf::Vector2f impulse );
		void applyImpulseAt( sf::Vector2f impulse, sf::Vector2f local_pos );
		void applyAngularImpulse( sf::Angle impulse );

		void setPosition( sf::Vector2f pos );
		void setVelocity( sf::Vector2f vel );
		void setRotation( sf::Angle rotation );
		void setAngularVelocity( sf::Angle r_vels );
	};
};