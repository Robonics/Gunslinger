#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "include/entity.hpp"
#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/PrimitiveType.hpp"
#include "SFML/System/Vector2.hpp"
#include "include/phys_object.hpp"
#include "include/engine.hpp"

using Arcade::Engine;
using Arcade::Entity;
using Arcade::RenderableEntity;
using Arcade::PhysicsEntity;
using Arcade::PhysSettings;

/* Entity */
Entity::Entity( Engine& eng ) : engine(eng), world_pos() {
	engine.registerEntity( this );
}
Entity::~Entity() {
	engine.deregisterEntity( this );
}

sf::Vector2f Entity::getPosition() {
	return world_pos;
}
void Entity::setPosition( sf::Vector2f n_pos ) {
	world_pos = n_pos;
}

sf::Angle Entity::getRotation() {
	return rotation;
}
void Entity::setRotation( sf::Angle n_ang ) {
	rotation = n_ang;
}

/* RenderableEngine */
RenderableEntity::RenderableEntity( Engine& eng ) : Entity( eng ) {}

/* PhysicsEntity */
void PhysicsEntity::tick( sf::Time dt ) {
	// std::cout << engine.getGlobalTime().asSeconds() << "s: Ticked PhysicsEntity @" << std::hex << reinterpret_cast<uintptr_t>(this) << std::dec << "!" << std::endl;
	Entity::setPosition( PhysObject::getPosition() );
	Entity::setRotation( PhysObject::getRotation() );
}

sf::Vector2f PhysicsEntity::getPosition() const {
	return PhysObject::getPosition();
}
void PhysicsEntity::setPosition( sf::Vector2f n_pos ) {
	Entity::setPosition( n_pos );
	PhysObject::setPosition( n_pos );
}
void PhysicsEntity::setRotation( sf::Angle n_ang ) {
	Entity::setRotation( n_ang );
	PhysObject::setRotation( n_ang );
}
sf::Angle PhysicsEntity::getRotation() const {
	return PhysObject::getRotation();
}

PhysicsEntity::PhysicsEntity( Engine& eng, b2BodyDef def, PhysSettings settings[], size_t n_shape ) : RenderableEntity(eng), PhysObject( eng.getWorld(), def, settings, n_shape ) {
	tick( sf::Time::Zero );
}
/// You really should be overriding this
void PhysicsEntity::draw( sf::RenderTarget& target, sf::RenderStates states) const {
	int size = b2Body_GetShapeCount( getID() );
	b2ShapeId shapes[ size ];
	b2Body_GetShapes(getID(), shapes, size);

	states.transform.translate( PhysObject::getPosition() );
	states.transform.rotate( PhysObject::getRotation() );

	for( size_t it = 0; it < size; it++ ) {
		switch( b2Shape_GetType( shapes[it] ) ) {
			case b2_circleShape: {
				b2Circle shape = b2Shape_GetCircle(shapes[it] );
				static sf::CircleShape cir;
				cir.setRadius( shape.radius );
				cir.setPosition( toSFMLVector( shape.center ) );
				cir.setFillColor( sf::Color::Red );

				target.draw( cir, states );
				break;
			}
			case b2_polygonShape: {
				b2Polygon shape = b2Shape_GetPolygon( shapes[it] );
				static sf::VertexArray verts( sf::PrimitiveType::TriangleStrip );
				for(int i = 0; i < shape.count; i++ ) {
					verts.append( sf::Vertex( toSFMLVector( shape.vertices[i] ), sf::Color::Red ) );
				}

				target.draw( verts, states );
			}
			default:
				break;
        }
	}

	static sf::CircleShape test(1.f);
	test.setFillColor( sf::Color::Green );
	test.setPosition( PhysObject::getPosition() );
	test.move({-1.f, -1.f});

	target.draw( test );
}