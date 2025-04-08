#pragma once

#include "../entity.hpp"

#include <SFML/Graphics.hpp>

using Arcade::PhysicsEntity;

class PropDynamic : public PhysicsEntity {
	sf::Sprite visual;
protected:
	virtual void draw( sf::RenderTarget& target, sf::RenderStates states ) {}
public:

};