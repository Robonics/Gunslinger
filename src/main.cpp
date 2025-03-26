#include <iostream>

#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>

#include <box2d/box2d.h>
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "box2d/collision.h"
#include "box2d/math_functions.h"
#include "box2d/types.h"
#include "include/engine.hpp"
#include "include/localizer.hpp"

#include "include/level.hpp"

int main( int argc, const char** argv ) {

	// Handle Args
	for( int i = 0; i < argc; i++ ) {
		std::string arg( argv[i] );

		// TODO: Create arg registry file
		if( arg.compare("--clear-imgui") == 0 ) {
			if( !std::filesystem::remove("./imgui.ini") ) {
				std::cout << "\e[33mCouldn't delete imgui.ini; is it even there?\e[0m" << std::endl;
			}
		}
	}

	Localizer::loadTranslation("../lang/en_us.csv");

	Arcade::Engine engine("Arcade Engine");

	engine.loadLevel("../levels/test.alv");

	sf::Vector2f size{5.f, 5.f};
	
	std::vector<b2BodyId> bodies;
	for( int it = 0; it < 5; it++ ) {
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = {15.0f + (it * size.x), 0.0f};
		bodies.push_back( b2CreateBody( engine.getWorld(), &bodyDef ) );
		
		b2Polygon box = b2MakeBox( size.x / 2, size.y / 2 );
		b2ShapeDef boxDef = b2DefaultShapeDef();
		boxDef.density = 1.f;
		boxDef.friction = 0.3f;
	
		b2CreatePolygonShape( bodies[it], &boxDef, &box );
	}

	sf::RectangleShape rect(size);
	rect.setFillColor(sf::Color::White);
	rect.setTexture( engine.getTexture("dev/tile_1x1.png").lock().get() );
	rect.setOrigin({size.x / 2.f, size.y / 2.f});

	engine.bindManager.bind("Meta:Menu", sf::Keyboard::Key::Escape);
	engine.bindManager.bind("Meta:Debug", sf::Keyboard::Key::P).setModifiers(false, false, true, false);
	engine.bindManager.bind("Meta:Editor", sf::Keyboard::Key::O).setModifiers(false, false, true, false);

	engine.bindManager.bind("Camera:Right", sf::Keyboard::Key::D);
	engine.bindManager.bind("Camera:Left", sf::Keyboard::Key::A);
	engine.bindManager.bind("Camera:Up", sf::Keyboard::Key::W);
	engine.bindManager.bind("Camera:Down", sf::Keyboard::Key::S);
	
	engine.bindManager.bind("Camera:Zoom+", sf::Keyboard::Key::Up);
	engine.bindManager.bind("Camera:Zoom-", sf::Keyboard::Key::Down);

	engine.bindManager.bind("Editor:Tool:Primary", sf::Mouse::Button::Left);
	engine.bindManager.bind("Editor:Tool:Secondary", sf::Mouse::Button::Right);

	sf::Clock frame_timer;
	while( engine.getWindow().isOpen() ) {
		engine.handleEvents();
		if(!engine.getWindow().isOpen()) break;

		if( engine.bindManager.isPressed("Editor:Tool:Primary") ) {
			auto m_worldpos = engine.getWindow().mapPixelToCoords(ImGui::GetMousePos());
			auto b_worldpos = b2Body_GetPosition( bodies[0] );
		
			float theta = std::atan2( b_worldpos.y - m_worldpos.y, b_worldpos.x - m_worldpos.x );
			float strength = std::sqrt( std::pow(b_worldpos.y - m_worldpos.y, 2) + std::pow( b_worldpos.x - m_worldpos.x, 2) );
			strength *= -50.f;
			b2Vec2 force( std::cos(theta) * strength, std::sin(theta) * strength);
			// std::cout << "Applying force " << force.x << ", " << force.y << std::endl;
			b2Body_ApplyForceToCenter( bodies[0], force, true);
		}

		engine.update( frame_timer.restart() );

		static sf::View& v = engine.getCamera();
		constexpr static const float SPEED = 60.0f;
		constexpr static const float ZOOM_SPEED = 0.5f;
		if( engine.bindManager.isPressed("Camera:Right") ) {
			v.move({
				engine.getFrameTime().asSeconds() * SPEED,
				0.0f
			});
		}else if( engine.bindManager.isPressed("Camera:Left") ) {
			v.move({
				engine.getFrameTime().asSeconds() * -SPEED,
				0.0f
			});
		}
		if( engine.bindManager.isPressed("Camera:Up") ) {
			v.move({
				0.0f,
				engine.getFrameTime().asSeconds() * -SPEED
			});
		}else if( engine.bindManager.isPressed("Camera:Down") ) {
			v.move({
				0.0f,
				engine.getFrameTime().asSeconds() * SPEED
			});
		}
		if( engine.bindManager.isPressed("Camera:Zoom+") ) {
			v.zoom(1 + engine.getFrameTime().asSeconds() * ZOOM_SPEED);
		}else if( engine.bindManager.isPressed("Camera:Zoom-") ) {
			v.zoom(1 - engine.getFrameTime().asSeconds() * ZOOM_SPEED);
		}

		engine.updateCamera();
		engine.render();

		for( b2BodyId body : bodies ) {
			rect.setPosition( toSFMLVector(b2Body_GetPosition(body)) );
			rect.setRotation( sf::radians( b2Rot_GetAngle(b2Body_GetRotation(body)) ) );
			engine.getWindow().draw( rect );
		}

		engine.display();
	}
}