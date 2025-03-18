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

	sf::Clock frame_timer;
	while( engine.getWindow().isOpen() ) {
		engine.handleEvents();
		if(!engine.getWindow().isOpen()) break;

		engine.update( frame_timer.restart() );



		engine.render();
	}
}