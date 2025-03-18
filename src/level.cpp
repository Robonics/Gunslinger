#include "include/level.hpp"
#include "include/engine.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/System/Vector2.hpp"
#include "include/arcade_errors.hpp"

#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>

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

#define TILE_SIZE 50.0f

using Arcade::Error;

Arcade::TileRegistryEntry::TileRegistryEntry( const std::shared_ptr<sf::Texture>& texture ) : texture(texture) {}
Arcade::TileRegistryEntry::TileRegistryEntry( const std::weak_ptr<sf::Texture>& texture ) : texture( texture ) {}

const std::weak_ptr<sf::Texture> Arcade::TileRegistryEntry::getTexture() {
	return this->texture;
}

Arcade::Tile::Tile( uint8_t flags ) : flags(flags) {}
Arcade::Tile::Tile( const std::shared_ptr<TileRegistryEntry>& entry ) : registryObject(entry), shape({TILE_SIZE,TILE_SIZE}) {
	this->shape.setTexture( this->registryObject.lock()->texture.lock().get() );
}
Arcade::Tile::Tile( const std::shared_ptr<TileRegistryEntry>& entry, uint8_t flags) : flags(flags), registryObject(entry), shape({TILE_SIZE,TILE_SIZE}) {
	this->shape.setTexture( this->registryObject.lock()->texture.lock().get() );
}
Arcade::Tile::Tile( Arcade::Engine& engine, const std::string& entry ) {
	engine.getTileEntry( entry );
}
Arcade::Tile::~Tile() {}
void Arcade::Tile::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	if( this->registryObject.expired() ) return;
	// auto p = states.transform.transformPoint({0.0f,0.0f});
	// std::cout << "Drawing tile at " << p.x << ", " << p.y << std::endl;
	target.draw( this->shape, states );
}
std::weak_ptr<Arcade::TileRegistryEntry> Arcade::Tile::getRegistryEntry() {
	return this->registryObject;
}

Arcade::Chunk::~Chunk() {

}
void Arcade::Chunk::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	// Be smart and cull ourselves
	// if(!target.getView().getViewport().findIntersection( bounds ) && (flags & Flags::NoCull)) return;
	
	for( auto it = tiles.begin(); it != tiles.end(); it++ ) {
		size_t i = std::distance( tiles.begin(), it );
		unsigned long x = i % this->size;
		unsigned long y = i / this->size;
		sf::RenderStates n_state( states );
		n_state.transform.translate({
			x*TILE_SIZE,
			y*TILE_SIZE
		});
		(*it).draw( target, n_state );
	}
}
void Arcade::Chunk::doTilePostInit() {
	for( size_t i = 0; i < tiles.size(); i++ ) {
		size_t x = i % size;
		size_t y = i / size;

		// First, set up the neighbor nibble
		tiles[i].neighbor.reset();
		if( y != 0 ) { // Top
			tiles[i].neighbor.set( 0u,
				tiles[((y-1) * size) + x].registryObject.lock()
					==
				tiles[i].registryObject.lock()
			);
		}
		if( x != size-1 ) { // Right
			tiles[i].neighbor.set( 0u,
				tiles[(y * size) + (x+1)].registryObject.lock()
					==
				tiles[i].registryObject.lock()
			);
		}
		if( y != size-1 ) { // Bottom
			tiles[i].neighbor.set( 0u,
				tiles[((y+1) * size) + x].registryObject.lock()
					==
				tiles[i].registryObject.lock()
			);
		}
		if( x != 0 ) { // Left
			tiles[i].neighbor.set( 0u,
				tiles[(y * size) + (x-1)].registryObject.lock()
					==
				tiles[i].registryObject.lock()
			);
		}
	}
}
const uint8_t Arcade::Chunk::getFlags() {
	return this->flags;
}

Arcade::Level::Level( Arcade::Engine& engine, std::ifstream& file ) {
	this->load( engine, file );
}
Arcade::Level::~Level() {}

void Arcade::Level::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	for( auto& chunk : chunks ) {
		sf::RenderStates n_state( states );
		n_state.transform.translate( chunk.bounds.position );
		chunk.draw( target, n_state );
	}
}

// .alv is stored as BIG ENDIAN always.
void Arcade::Level::load( Arcade::Engine& engine, std::ifstream& file ) {
	// Alright, first let's check on the status of the ifstream
	if(!file.good()) throw Error<ErrorType::FILE_ERROR>("File is bad");
	
	uint32_t magic;
	file.read((char*)&magic, 4u);
	magic = be32toh( magic );
	if( magic != ALV_MAGIC) {
		std::stringstream err_msg;
		err_msg << "Incorrect magic, expected 0x" << std::hex << ALV_MAGIC << ", got 0x" << magic;
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

	file.read( (char*)&chunk_size, 4u );
	chunk_size = be32toh( chunk_size );

	// Now we sequentially read in each chunk
	for( size_t i = 0; i < world_x*world_y; i++ ) {
		Chunk& chunk = chunks.emplace_back();
		file.read( (char*)&chunk.flags, 1u );

		chunk.bounds = {
			{(i % world_x)*(chunk_size*TILE_SIZE), static_cast<int>(i / world_x)*(chunk_size*TILE_SIZE)},
			{ chunk_size*TILE_SIZE, chunk_size*TILE_SIZE }
		};
		chunk.size = chunk_size;

		std::cout << i << " has flags 0x" << std::hex << (int)chunk.flags << std::dec << "\n";
		std::cout << "\tChunk Bounds: " << chunk.bounds.position.x << ", " << chunk.bounds.position.y
				<< " " << chunk.bounds.size.x << "x" << chunk.bounds.size.y << std::endl;

		if( !(chunk.flags & Chunk::Flags::HasTiles) ) {
			std::cout << "Chunk " << i << " marked as empty, skipping..." << std::endl;
			continue;
		}

		std::unordered_map<uint8_t, std::string> chunk_dict;
		uint8_t dict_size;
		file.read( (char*)&dict_size, 1u );

		for( uint8_t it = 0; it < dict_size; it++ ) {
			uint8_t n;
			file.read( (char*)&n, 1u );
			std::string identifier;
			char c;
			while( file.read(&c, 1u) ) {
				if(c == '\0') break;
				identifier.insert( identifier.end(), c );
			}

			// Final sanity checks
			if( identifier.length() < 1 ) continue;
			chunk_dict.emplace( n, identifier );
		}

		// woohoo, we can finally read in tiles.
		for( size_t it = 0; it < chunk_size*chunk_size; it++ ) {
			uint8_t tile_header;
			file.read( (char*)&tile_header, 1u );
			if( tile_header & Tile::Flags::Empty ) {
				chunk.tiles.emplace_back( tile_header );
				std::cout << std::endl;
				continue;
			};
			/// \todo Implement Data
			uint8_t tile;
			file.read( (char*)&tile, 1u );
			try {
				chunk.tiles.emplace_back( engine.getTileEntry(chunk_dict.at(tile)).lock(), tile_header );
			}catch(Error<ErrorType::RESOURCE_ERROR> e) {
				std::cerr << "\e[32mFailed to load resource: " << e.what() << "\n\t" << "Defaulting to empty tile\e[0m" << std::endl;
				chunk.tiles.emplace_back( tile_header );
			}catch( std::out_of_range e ) {
				std::cerr << "\e[32mHey stupid! You screwed up your chunk dictionary. " << (int)tile << " wasn't in the dictionary" << "\n\t" << "Defaulting to empty tile\e[0m" << std::endl;
				chunk.tiles.emplace_back( tile_header );
			}
		}

	}
}
void Arcade::Level::load( Arcade::Engine& engine, const std::string& filename ) {
	std::ifstream file( filename );
	if( file ) {
		this->load( engine, file );
	}
}
void Arcade::Level::unload() {
	this->chunk_size=0;
	this->chunks.clear();
}

const std::string& Arcade::Level::getName() {
	return this->tl_key;
}
const sf::Vector2u Arcade::Level::getSize() {
	return this->world_size;
}
const uint32_t Arcade::Level::getChunkSize() {
	return this->chunk_size;
}