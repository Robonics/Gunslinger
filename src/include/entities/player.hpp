#pragma once

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "../engine.hpp"
#include "../entity.hpp"
#include "box2d/types.h"

using namespace Arcade;

constexpr static sf::Vector2f PLAYER_SIZE{8.f, 10.f};

class Player : public PhysicsEntity {
	float acceleration{1.f};
	sf::Vector2f max_speed{10.f, 10.f};

	// b2QueryFilter collision_group{
	// 	.categoryBits=CollisionGroups::Player,
	// 	.maskBits=CollisionGroups::World
	// };

	static bool collision_handler( b2ShapeId shape, void* ctx ) {
		Player* self = reinterpret_cast<Player*>(ctx);

		return false;
	}

protected:
	virtual void draw( sf::RenderTarget& target, sf::RenderStates states ) const override {
		static sf::RectangleShape rect = []() {
			sf::RectangleShape rect(PLAYER_SIZE);
			rect.setOrigin({PLAYER_SIZE.x / 2.f, PLAYER_SIZE.y / 2.f});
			rect.setFillColor( sf::Color::Blue );

			return rect;
		}();
		rect.setPosition( getPosition() );
		
		target.draw( rect, states );
	}
	virtual void tick( sf::Time dt ) override {
		b2World_OverlapAABB(engine.getWorld(), getAABB(), b2DefaultQueryFilter(), &collision_handler, this);
	}
public:
	Player( Engine& engine ) : PhysicsEntity( engine, [](){
		b2BodyDef def = b2DefaultBodyDef();
		def.type = b2BodyType::b2_kinematicBody;
		def.fixedRotation=true;
		def.enableSleep=false;
		return def;
	}(), (PhysSettings[1]){PhysSettings{
		.type=b2ShapeType::b2_polygonShape,
		.shape=[]() {
			b2ShapeDef def = b2DefaultShapeDef();
			def.friction = 0.1f;
			def.density = 1.0f;
			return def;
		}(),
		.shape_data=b2MakeBox( PLAYER_SIZE.x / 2.f, PLAYER_SIZE.y / 2.f )
	}}, 1u) {

	}
};