#include "include/phys_object.hpp"
#include "SFML/Graphics/Transform.hpp"
#include "SFML/System/Angle.hpp"
#include "SFML/System/Vector2.hpp"
#include "box2d/box2d.h"
#include "box2d/math_functions.h"
#include "include/engine.hpp"
#include <cassert>

using Arcade::PhysSettings;
using Arcade::PhysObject;

PhysObject::PhysObject( b2WorldId world, b2BodyDef def, PhysSettings settings[], size_t n_shape ) {
	assert( b2World_IsValid( world ) && "Passed world is not valid");
	b2_id = b2CreateBody( world, &def );
	assert( b2Body_IsValid(b2_id) && "Body failed to init");
	assert( n_shape > 0 && "Must pass at least one shape!");
	for( size_t i = 0; i < n_shape; i++ ) {
		switch( settings[i].type ) {
			case b2ShapeType::b2_polygonShape:
				b2CreatePolygonShape(b2_id, &settings[i].shape, &std::get<b2Polygon>(settings[i].shape_data));
				break;
			case b2_circleShape:
				b2CreateCircleShape( b2_id, &settings[i].shape, &std::get<b2Circle>(settings[i].shape_data)  );
				break;
			case b2_capsuleShape:
				b2CreateCapsuleShape( b2_id, &settings[i].shape, &std::get<b2Capsule>( settings[i].shape_data) );
				break;
			default:
				assert( "Unsupperted Shape" );
        }
	}
}

PhysObject::~PhysObject() {
	b2DestroyBody( b2_id );
}

const b2BodyId PhysObject::getID() const {
	return b2_id;
}

sf::Vector2f PhysObject::getPosition() const {
	return toSFMLVector( b2Body_GetPosition(b2_id) );
}
sf::Vector2f PhysObject::getVelocity() const {
	return toSFMLVector( b2Body_GetLinearVelocity(b2_id) );
}
b2AABB PhysObject::getAABB() const {
	return b2Body_ComputeAABB( getID() );
}

sf::Angle PhysObject::getRotation() const {
	return sf::radians( b2Rot_GetAngle(b2Body_GetRotation(b2_id)) );
}
sf::Angle PhysObject::getAngularVelocity() const {
	return sf::radians( b2Body_GetAngularVelocity( b2_id ) );
}

/**
	\brief Applies an impulse to the body at the center
	\param impulse The vector of impulse
*/
void PhysObject::applyImpulse( sf::Vector2f impulse ) {
	b2Body_ApplyLinearImpulseToCenter(b2_id, toB2DVector(impulse), true);
}
/**
	\brief Applies an impulse to a point on the body.
	\param impulse The vector of impulse
	\param local_pos The position to apply the impluse to, relative to the shape. Does not account for rotation
*/
void PhysObject::applyImpulseAt( sf::Vector2f impulse, sf::Vector2f local_pos ) {
	sf::Transform t;
	t.translate( getPosition() );
	t.rotate( getRotation() );
	auto p  = t.transformPoint( local_pos );
	b2Body_ApplyLinearImpulse(b2_id, toB2DVector(impulse), toB2DVector(p), true);
}
void PhysObject::applyAngularImpulse( sf::Angle impulse ) {
	b2Body_ApplyAngularImpulse(b2_id, impulse.asRadians(), true);
}

void PhysObject::setPosition( sf::Vector2f pos ) {
	b2Body_SetTransform(b2_id, toB2DVector(pos), b2Body_GetRotation(b2_id));
}
void PhysObject::setVelocity( sf::Vector2f vel ) {
	b2Body_SetLinearVelocity(b2_id, toB2DVector(vel));
}
void PhysObject::setRotation( sf::Angle rotation ) {
	b2Body_SetTransform(b2_id, b2Body_GetPosition(b2_id), toB2DAngle(rotation));
}
void PhysObject::setAngularVelocity( sf::Angle r_vel ) {
	b2Body_SetAngularVelocity(b2_id, r_vel.asRadians());
}