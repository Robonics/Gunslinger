#include <iostream>

#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>

#include "SFML/Window/Keyboard.hpp"
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

		engine.update( frame_timer.restart() );

		static sf::View& v = engine.getCamera();
		constexpr static const float SPEED = 240.0f;
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

		engine.display();
	}
}