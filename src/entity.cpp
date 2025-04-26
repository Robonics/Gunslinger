#include <SFML/Graphics.hpp>

#include <box2d/box2d.h>


#include <vector>

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
using Arcade::MoverEntity;
using Arcade::PhysSettings;

/* Entity */
Entity::Entity( Engine& eng ) : engine(eng), world_pos() {
	engine.registerEntity( this );
}
Entity::~Entity() {
	engine.deregisterEntity( this );
}
void Entity::debug() const {
	ImGui::Text("Entity "); ImGui::SameLine();
	ImGui::TextColored(sf::Color::Cyan, "%s " , this->GET_NAME()); ImGui::SameLine();
	ImGui::TextColored( sf::Color::Magenta, "@%zu", reinterpret_cast<std::intptr_t>(this));
}

sf::Vector2f Entity::getPosition() const {
	return world_pos;
}
void Entity::setPosition( sf::Vector2f n_pos ) {
	world_pos = n_pos;
}

sf::Angle Entity::getRotation() const {
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

MoverEntity::MoverEntity( Engine& engine, b2Vec2 size ) : RenderableEntity(engine), bb_size( toSFMLVector(size) ) {
	assert( size.x < size.y && "Bounds must be tall!");
	collider.radius = size.x * 0.5f;
	collider.center1 = b2Vec2{size.x * .5f, collider.radius};
	collider.center2 = b2Vec2{size.x * .5f, size.y - collider.radius};

	collision_mask = b2DefaultQueryFilter();
}

namespace {
	struct MoverColliderContext {
		std::vector<b2CollisionPlane> planes;
		b2Capsule mover;
	};

	struct ShapeUserData {
		float maxPush;
		bool clipVelocity;
	};
}

void MoverEntity::tick( sf::Time dt ) {
	if( !b2World_IsValid(engine.getWorld()) ) return;
	if(engine.bindManager.isPressed("Player:Left")) velocity.x += -this->acceleration * dt.asSeconds();
	if(engine.bindManager.isPressed("Player:Right")) velocity.x += this->acceleration * dt.asSeconds();
	if(engine.bindManager.startedPressing("Player:Jump") && grounded) velocity.y += -this->jump_power;

	velocity += b2World_GetGravity(engine.getWorld()) * dt.asSeconds();

	b2Capsule ws_capsule = this->collider;
	
	static const float tolerance = 0.01f;
	unsigned int total_iterations = 0;
	MoverColliderContext mv_ctx;
	
	b2Vec2 target = toB2DVector(getPosition()) + (velocity * dt.asSeconds());

	for( int i = 0; i < MOVER_ITERATIONS; i++ ) {
		ws_capsule.center1 = collider.center1 + toB2DVector(getPosition());
		ws_capsule.center2 = collider.center2 + toB2DVector(getPosition());

		b2World_CollideMover(
			engine.getWorld(),
			&ws_capsule,
			collision_mask,
			[]( b2ShapeId shape, const b2PlaneResult* result, void* _ctx ) -> bool {
				assert( result->hit == true );
				MoverColliderContext* ctx = static_cast<MoverColliderContext*>( _ctx );
				float maxPush = FLT_MAX;
				bool clipVelocity = true;
	
				ShapeUserData* userData = static_cast<ShapeUserData*>( (void*)b2Shape_GetUserData( shape ) );
				if ( userData != nullptr ) {
					maxPush = userData->maxPush;
					clipVelocity = userData->clipVelocity;
				}
				
				if( b2IsValidPlane( result->plane ) ) {
					ctx->planes.push_back(b2CollisionPlane{
						result->plane,
						maxPush,
						0.0f,
						clipVelocity
					});
				}else {
					std::cerr << "\e[1;31mInvalid plane: \n\t"
						<< result->plane.normal.x << ", " << result->plane.normal.x << "\n\t"
						<< result->plane.offset << "\e[0m" << std::endl;
				}
		
				return true;
			},
			(void*)(&mv_ctx)
		);
		
		b2PlaneSolverResult sp_result = b2SolvePlanes(target, mv_ctx.planes.data(), mv_ctx.planes.size());
		total_iterations += sp_result.iterationCount;

		debug_info.sp_position = sp_result.position;

		b2Vec2 translation = sp_result.position - toB2DVector(getPosition());
		float fraction = b2World_CastMover(
			engine.getWorld(),
			&ws_capsule,
			translation,
			collision_mask
		);
		debug_info.translation = translation;
		debug_info.fraction = fraction;
		sf::Vector2f delta = toSFMLVector(translation * fraction);
		setPosition( getPosition() + delta );

		debug_info.delta = delta;

		if( b2LengthSquared(toB2DVector(delta) ) < tolerance*tolerance) {
			break;
		}
	}
	debug_info.iterations = total_iterations;

	velocity = b2ClipVector( velocity, mv_ctx.planes.data(), mv_ctx.planes.size());
	// ...
}
/// \warning This is a *placeholder*. It should be overridden by any classes that derive from MoverEntity.
void MoverEntity::draw( sf::RenderTarget& target, sf::RenderStates states ) const {
	static sf::CircleShape circle = ([]() -> sf::CircleShape {
		sf::CircleShape cir;

		cir.setFillColor(sf::Color::Transparent);
		cir.setOutlineColor(sf::Color::Cyan);
		cir.setOutlineThickness(.2f);

		return cir;
	})();
	static sf::RectangleShape bounds = ([]() -> sf::RectangleShape {
		sf::RectangleShape rect;
		rect.setFillColor( sf::Color(0xFF0000A0) );
		rect.setOutlineColor( sf::Color::Red );
		rect.setOutlineThickness( .2f );

		return rect;
	})();

	bounds.setPosition( getPosition() );
	bounds.setSize( bb_size );
	target.draw(bounds, states);

	circle.setRadius( this->collider.radius );

	circle.setPosition( toSFMLVector( this->collider.center1 + toB2DVector(getPosition()) - b2Vec2{ this->collider.radius, this->collider.radius } ) );
	target.draw(circle, states);
	circle.setPosition( toSFMLVector( this->collider.center2 + toB2DVector(getPosition()) - b2Vec2{ this->collider.radius, this->collider.radius } ) );
	target.draw(circle, states);

}

void MoverEntity::debug() const {
	Entity::debug();

	ImGui::TextColored( sf::Color::Magenta, "Move / Jump Speed: %f, %f", move_speed, jump_power); 
	ImGui::Text("Position: %f, %f", getPosition().x, getPosition().y); 
	ImGui::Text("Velocity: %f, %f", velocity.x, velocity.y); 
	ImGui::Text("Last Frame"); 
	ImGui::Text("\tPlane Solver Pos: %f, %f", debug_info.sp_position.x, debug_info.sp_position.y ); 
	ImGui::Text("\tIterations: %u", debug_info.iterations ); 
	ImGui::Text("\tTranslation: %f, %f", debug_info.translation.x, debug_info.translation.y ); 
	ImGui::Text("\tFraction: %f", debug_info.fraction ); 
	ImGui::Text("\tDelta: %f, %f", debug_info.delta.x, debug_info.delta.y ); 

}

void MoverEntity::setMoveSpeed( float n_speed ) {
	this->move_speed = std::abs(n_speed);
}
float MoverEntity::getMoveSpeed() const {
	return this->move_speed;
}
void MoverEntity::setJumpPower( float n_power ) {
	this->jump_power = std::max(0.f, n_power);
}
float MoverEntity::getJumpPower() const {
	return this->jump_power;
}
void MoverEntity::setAcceleration( float n_accel ) {
	this->acceleration = std::max(0.f, n_accel);
}
float MoverEntity::getAcceleration() const {
	return this->acceleration;
}