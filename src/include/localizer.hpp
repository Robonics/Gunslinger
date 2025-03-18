#pragma once

#include <string>
#include <unordered_map>
#include <fstream>

class Localizer {
	static std::unordered_map<std::string, std::u8string> keys;
	static std::string loaded_lang;

	static void load( std::ifstream&  file );
	static void load_override( std::ifstream&  file );

public:
	Localizer() = delete;
	~Localizer() = delete;

	static const std::string& getLoadedLanguage(); 
	static std::u8string getTranslation( const std::string& key );
	static void loadTranslation( std::string_view file );
	static void appendNoOverride( std::string_view file );
	static void appendOverride( std::string_view file );

	static const std::unordered_map<std::string, std::u8string>& getAllKeys();
};