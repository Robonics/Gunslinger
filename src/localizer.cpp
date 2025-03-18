#include "include/localizer.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

std::unordered_map<std::string, std::u8string> Localizer::keys;
std::string Localizer::loaded_lang{};
void Localizer::load( std::ifstream& file ) {
	std::cout << "loading" << std::endl;
	for( std::string line; std::getline( file, line ); ) {
		std::cout << line << std::endl;
		auto i = line.find(',');
		std::string k = line.substr(0, i);
		std::string v =  line.substr( i + 1 );
		std::u8string val(reinterpret_cast<const char8_t*>(v.data()), v.size());
		Localizer::keys.emplace( k, val );
	}
}
void Localizer::load_override( std::ifstream& file ) {
	for( std::string line; std::getline( file, line ); ) {
		auto i = line.find(',');
		std::string k = line.substr(0, i);
		Localizer::keys.erase( k );
		std::string v =  line.substr( i + 1 );
		std::u8string val(reinterpret_cast<const char8_t*>(v.data()), v.size());
		Localizer::keys.emplace( k, val );
	}
}

const std::string& Localizer::getLoadedLanguage() {
	return loaded_lang;
}

void Localizer::loadTranslation( std::string_view filename ) {
	std::ifstream f( filename.data() );
	if( f.is_open() ) {
		Localizer::keys.clear();
		load( f );
		std::cout << "Loaded localization file: " << filename << std::endl;
		loaded_lang = filename;
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

std::u8string Localizer::getTranslation( const std::string& key ) {
	try {
		return Localizer::keys.at( key );
	}catch( std::out_of_range e ) {
		return std::u8string(reinterpret_cast<const char8_t*>(key.data()), key.size());
	}
}

const std::unordered_map<std::string, std::u8string>& Localizer::getAllKeys() {
	return Localizer::keys;
}