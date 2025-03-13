#include "include/localizer.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

std::unordered_map<std::string, std::string> Localizer::keys;
void Localizer::load( std::ifstream& file ) {
	for( std::string line; std::getline( file, line ); ) {
		auto i = line.find(',');
		Localizer::keys.emplace( line.substr(0, i), line.substr(i + 1, line.length() - i ) );
	}
}
void Localizer::load_override( std::ifstream& file ) {
	for( std::string line; std::getline( file, line ).good(); ) {
		auto i = line.find(',');
		Localizer::keys.erase( line.substr(0, i) );
		Localizer::keys.emplace( line.substr(0, i), line.substr(i + 1, line.length() - i - 1 ) );
	}
}

void Localizer::loadTranslation( std::string_view filename ) {
	std::ifstream f( filename.data() );
	if( f.is_open() ) {
		Localizer::keys.clear();
		load( f );
		std::cout << "Loaded localization file: " << filename << std::endl;
 		return;
	}
	std::cerr << "\e[33mWarning: Failed to load " << filename << "\e[0m" << std::endl;
}

void Localizer::appendNoOverride( std::string_view filename ) {
	std::ifstream f( filename.data() );
	if( f.is_open() ) {
		load( f );
	}
}
void Localizer::appendOverride( std::string_view filename ) {
	std::ifstream f( filename.data() );
	if( f.is_open() ) {
		load_override( f );
	}
}

const std::string_view Localizer::getTranslation( std::string_view key ) {
	try {
		return Localizer::keys.at( key.data() );
	}catch( std::out_of_range e ) {
		return key;
	}
}

const std::unordered_map<std::string, std::string>& Localizer::getAllKeys() {
	return Localizer::keys;
}