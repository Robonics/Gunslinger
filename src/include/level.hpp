#pragma once

#include "SFML/System/Vector2.hpp"
#include <bitset>
#include <unordered_map>
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

	enum class TileRenderMode {
		Static=0,
		Autotile,
		Animated,
		Random,
		Autotile_Random,
	};

	class TileRegistryEntry {
		friend class Tile;
		friend class Chunk;
		friend class Level;
		friend class Engine;
		TileRenderMode r_mode{TileRenderMode::Static};
		std::string registry_name;
		std::weak_ptr<sf::Texture> texture;

	public:
		TileRegistryEntry()=delete;
		explicit TileRegistryEntry( const std::shared_ptr<sf::Texture>& texture, TileRenderMode render_mode=TileRenderMode::Static );
		explicit TileRegistryEntry( const std::weak_ptr<sf::Texture>& texture, TileRenderMode render_mode=TileRenderMode::Static );

		const std::weak_ptr<sf::Texture> getTexture();
		const TileRenderMode getRenderMode() {
			return r_mode;
		}

		std::string getRegistryName();
		static const char* RModeString( TileRenderMode m );
	};

	class Tile : protected sf::Drawable, sf::Transformable {
		friend class Chunk;
		friend class Level;
		std::bitset<4> neighbor;
		uint8_t flags{};
		std::weak_ptr<TileRegistryEntry> registryObject;
		sf::RectangleShape shape;
	protected:
		void setup_texture();
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
		sf::FloatRect bounds;
	public:

		enum Flags {
			Empty = 0b10000000,
			Depreciated = 0b01000000,
			HasData = 0b00100000,
			Ghost = 0b00010000,
			ExcludeCollisionMerge = 0b00001000
		};

		const sf::FloatRect& getBounds();

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
		Tile* getTileAt( sf::Vector2f pos );

		/// This calculates EVERYTHING. Autotile indecies, collison, all of it.
		/// This should never ever EVER be called regularly. 
		void doTilePostInit();

		const uint8_t getFlags() const;
		const sf::FloatRect getBounds() const;
	};

	enum LevelEditorTool {
		None=0,
		TileDraw,
		TileSelect
	};

	struct Vector2uHash {
		std::size_t operator()(const sf::Vector2u& v) const noexcept {
			const uint64_t a = static_cast<uint64_t>(v.x);
			const uint64_t b = static_cast<uint64_t>(v.y);
		
			const uint64_t h0 = (b << 32) | a;
			const uint64_t h1 = (a << 32) | b;
		
			return (v.x < v.y) ? h0 : h1; 
		}
	};

	class Level : public sf::Drawable {
		friend class Engine;

		bool editor_open{};

		std::string tl_key;
		sf::Vector2u world_size;
		uint32_t chunk_size;
		std::unordered_map<sf::Vector2u, Chunk, Vector2uHash> chunks;
	protected:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
		void drawEditor(  Arcade::Engine& engine  );
	
	public:
		Level() = default;
		Level( Arcade::Engine&, std::ifstream& file );
		~Level();
	
		void save( const std::filesystem::path& path ) const;
		void unload();
		void load( Arcade::Engine&, std::ifstream& file );
		void load( Arcade::Engine&, const std::string& filename );

		const std::string& getName();
		const sf::Vector2u getSize();
		const uint32_t getChunkSize();

		Chunk* getChunkAt(sf::Vector2f pos);
		Tile* getTileAt(sf::Vector2f pos);

	};
};