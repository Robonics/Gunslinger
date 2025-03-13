#pragma once

#include <string>
#include <unordered_map>
#include <fstream>

class Localizer {
	static std::unordered_map<std::string, std::string> keys;

	static void load( std::ifstream&  file );
	static void load_override( std::ifstream&  file );

public:
	Localizer() = delete;
	~Localizer() = delete;

	static const std::string_view getTranslation( std::string_view key );
	static void loadTranslation( std::string_view file );
	static void appendNoOverride( std::string_view file );
	static void appendOverride( std::string_view file );

	static const std::unordered_map<std::string, std::string>& getAllKeys();
};