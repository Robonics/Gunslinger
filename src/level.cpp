#include "include/level.hpp"
#include "SFML/System/Vector2.hpp"
#include "include/arcade_errors.hpp"

#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <netinet/in.h>
#include <sstream>

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif


#define ALV_MAGIC 0x00414C56

using Arcade::Error;

Arcade::Level::Level( std::ifstream& file ) {
	this->load( file );
}
// .alv is stored as BIG ENDIAN always.
void Arcade::Level::load( std::ifstream& file ) {
	// Alright, first let's check on the status of the ifstream
	if(!file.good()) throw Error<ErrorType::FILE_ERROR>("File is bad");
	
	uint32_t magic;
	file.read((char*)&magic, 4u);
	magic = be32toh( magic );
	if( magic != ALV_MAGIC) {
		std::stringstream err_msg;
		err_msg << "Incorrect magic, expected 0x" << std::hex << ALV_MAGIC << ", got 0x" << std::hex << magic;
		throw Error<ErrorType::FILE_ERROR>(err_msg.str(), ErrorType::FILE_BAD_MAGIC);
	}

	tl_key.clear();
	{
		char c;
		while( file.read(&c, 1) ) {
			if( file.eof() ) {
				throw Error<ErrorType::FILE_ERROR>("Unexpected end of file", ErrorType::FILE_EOF);
			}
			if( c == '\0' ) break;
			tl_key.insert( tl_key.end(), 1, c );
		}
		if( tl_key.length() < 3 ) throw Error<ErrorType::ARG_ERROR>("", ErrorType::ARG_LENGTH);
	}
	uint32_t world_x;
	uint32_t world_y;
	file.read( (char*)&world_x, 4u );
	file.read( (char*)&world_y, 4u );
	world_x = be32toh( world_x );
	world_y = be32toh( world_y );

	world_size = { world_x, world_y };
	file.read( (char*)&chunk_size, 8u );
	chunk_size = be64toh( chunk_size );
}

const std::string_view Arcade::Level::getName() {
	return this->tl_key;
}
const sf::Vector2u Arcade::Level::getSize() {
	return this->world_size;
}
const size_t Arcade::Level::getChunkSize() {
	return this->chunk_size;
}