#include <iostream>

#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>

#include "SFML/Window/Keyboard.hpp"
#include "include/arcade_errors.hpp"
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

	try {
		std::ifstream file( "../levels/test.alv" );
		Arcade::Level lvl( file );

		auto size = lvl.getSize();
		std::cout << "Loaded " << Localizer::getTranslation( lvl.getName() ) << " with " << size.x << "x" << size.y << " chunks. Each being " << lvl.getChunkSize() << "x" << lvl.getChunkSize() << " tiles." << std::endl;
	}catch( Arcade::Error<Arcade::ErrorType::FILE_ERROR> e ) {
		std::cerr << (int)e.type() << " | " << e.what() << std::endl; 
	}catch( Arcade::Error<Arcade::ErrorType::ARG_ERROR> e ) {
		std::cerr << (int)e.type() << " | " << e.what() << std::endl;
	}

	Arcade::Engine engine("Arcade Engine");

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