#pragma once

#define ALV_MAGIC 0x00414C56
#define ATSET_MAGIC 0x004154534554AFAF

#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Transformable.hpp"
#include <SFML/Graphics.hpp>
#include <fstream>
#include <memory>
#include <vector>

namespace Arcade {

	class Engine;

	class TileRegistryEntry {
		friend class Tile;
		friend class Chunk;
		std::weak_ptr<sf::Texture> texture;

	public:
		TileRegistryEntry()=delete;
		explicit TileRegistryEntry( const std::shared_ptr<sf::Texture>& texture );
		explicit TileRegistryEntry( const std::weak_ptr<sf::Texture>& texture );

		const std::weak_ptr<sf::Texture> getTexture();
	};

	class Tile : protected sf::Drawable, sf::Transformable {
		friend class Chunk;
		friend class Level;

		uint8_t flags{};
		std::weak_ptr<TileRegistryEntry> registryObject;
		sf::RectangleShape shape;
	protected:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	public:

		enum Flags {
			Empty = 0b10000000,
			Depreciated = 0b01000000,
			HasData = 0b00100000,
			Ghost = 0b00010000,
			ExcludeCollisionMerge = 0b00001000
		};

		Tile()=delete;
		Tile( uint8_t flags );
		explicit Tile( const std::shared_ptr<TileRegistryEntry>& );
		explicit Tile( const std::shared_ptr<TileRegistryEntry>&, uint8_t );
		Tile( Arcade::Engine&, const std::string& entry );
		~Tile();

		std::weak_ptr<TileRegistryEntry> getRegistryEntry();
	};
	class Chunk : public sf::Drawable, sf::Transformable {
		friend class Level;

		sf::FloatRect bounds;

		uint8_t flags;
		uint32_t size;
		std::vector<Tile> tiles;
	protected:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	public:
		enum Flags {
			HasTiles = 0b10000000,
			DeadChunk = 0b0100000,
			OoB = 0b00100000,
			NoCull = 0b00010000
		};

		Chunk()=default;
		~Chunk();
		Tile& getTileAt( size_t i );
		Tile& getTileAt( unsigned long x, unsigned long y );

		const uint8_t getFlags();
	};

	class Level : public sf::Drawable {
		std::string tl_key;
		sf::Vector2u world_size;
		uint32_t chunk_size;
		std::vector<Chunk> chunks;
	protected:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	
	public:
		Level() = default;
		Level( Arcade::Engine&, std::ifstream& file );
		~Level();
	
		void unload();
		void load( Arcade::Engine&, std::ifstream& file );
		void load( Arcade::Engine&, const std::string& filename );

		const std::string& getName();
		const sf::Vector2u getSize();
		const uint32_t getChunkSize();
	};
};