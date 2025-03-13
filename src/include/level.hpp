#pragma once

#include "SFML/System/Vector2.hpp"
#include <fstream>
#include <string_view>
#include <vector>

namespace Arcade {
	class Tile{};
	class Chunk {
		unsigned char flags;
		size_t size;
		Tile tiles[];
	};

	class Level {
		std::string tl_key;
		sf::Vector2u world_size;
		size_t chunk_size;
		std::vector<Chunk> chunks;
	public:
		Level() = default;
		Level( std::ifstream& file );
	
		void unload();
		void load( std::ifstream& file );
		void load( const std::string_view filename );

		const std::string_view getName();
		const sf::Vector2u getSize();
		const size_t getChunkSize();
	};
};