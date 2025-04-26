#pragma once

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "../engine.hpp"
#include "../entity.hpp"

using namespace Arcade;

constexpr static sf::Vector2f PLAYER_SIZE{8.f, 12.f};

class Player : public MoverEntity {

protected:
public:
	virtual constexpr const char* GET_NAME() const override {return "Player";}

	Player( Engine& engine ) : MoverEntity(engine, toB2DVector(PLAYER_SIZE)) {}
};