#include <iostream>

#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>

#include <box2d/box2d.h>
#include "SFML/Window/Keyboard.hpp"
#include "include/engine.hpp"
#include "include/localizer.hpp"

#include "include/entities/player.hpp"
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
	engine.getWindow().setFramerateLimit( 120u );

	engine.loadLevel("../levels/test.alv");

	Player player( engine );

	player.setPosition( sf::Vector2f{20.f, 0.f} );

	engine.bindManager.bind("Meta:Menu", sf::Keyboard::Key::Escape);
	engine.bindManager.bind("Meta:Debug", sf::Keyboard::Key::P).setModifiers(false, false, true, false);
	engine.bindManager.bind("Meta:Editor", sf::Keyboard::Key::O).setModifiers(false, false, true, false);
	
	engine.bindManager.bind("Camera:Zoom+", sf::Keyboard::Key::Up);
	engine.bindManager.bind("Camera:Zoom-", sf::Keyboard::Key::Down);

	engine.bindManager.bind("Editor:Tool:Primary", sf::Mouse::Button::Left);
	engine.bindManager.bind("Editor:Tool:Secondary", sf::Mouse::Button::Right);

	engine.bindManager.bind("Player:Float", sf::Keyboard::Key::Space);
	engine.bindManager.bind("Player:Right", sf::Keyboard::Key::D);
	engine.bindManager.bind("Player:Left", sf::Keyboard::Key::A);

	engine.getCamera().zoom( 0.2f );

	sf::Clock frame_timer;
	while( engine.getWindow().isOpen() ) {
		engine.handleEvents();
		if(!engine.getWindow().isOpen()) break;

		engine.update( frame_timer.restart() );

		static sf::View& v = engine.getCamera();
		constexpr static const float ZOOM_SPEED = 0.5f;
		if( engine.bindManager.isPressed("Camera:Zoom+") ) {
			v.zoom(1 + engine.getFrameTime().asSeconds() * ZOOM_SPEED);
		}else if( engine.bindManager.isPressed("Camera:Zoom-") ) {
			v.zoom(1 - engine.getFrameTime().asSeconds() * ZOOM_SPEED);
		}

		v.setCenter( player.getPosition() );

		engine.updateCamera();
		engine.render();

		engine.display();
	}
}