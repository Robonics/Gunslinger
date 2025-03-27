#pragma once

#include <box2d/box2d.h>
#include "SFML/System/Vector2.hpp"
#include "box2d/id.h"
#include "edge_buffer.hpp"
#include <bitset>

#define ALV_MAGIC 0x00414C56
#define ATSET_MAGIC 0x004154534554AFAF
#define TILE_SIZE 5.f

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
		friend class Level;
		friend class Engine;

		std::bitset<4> neighbor{};
		std::bitset<4> g_neighbor{};
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
		Tile( const Tile& )=default;
		Tile( uint8_t flags );
		explicit Tile( const std::shared_ptr<TileRegistryEntry>& );
		explicit Tile( const std::shared_ptr<TileRegistryEntry>&, uint8_t );
		Tile( Arcade::Engine&, const std::string& entry );
		~Tile();

		std::weak_ptr<TileRegistryEntry> getRegistryEntry();
	};

	enum LevelEditorTool {
		None=0,
		TileDraw,
		TileSelect
	};

	class Level : public sf::Drawable {
		friend class Engine;

		bool editor_open{};

		std::string tl_key;
		sf::Vector2u world_size;
		
		std::vector<Tile> tiles;

		Engine& engine;
		
	protected:
		b2BodyId ground;
		EdgeBuffer<float> edges;
		std::vector<b2ChainId> chains;
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
		void drawEditor();
		void doTilePostInit();

		/// Dynamically resizes the level. You should always use this over modifying the tile data directly.
		void placeTileAt( sf::Vector2u grid_pos, const Tile& tile );
		void removeTileAt( sf::Vector2u grid_pos );
	public:
		Level() = delete;
		Level( Arcade::Engine& );
		Level( Arcade::Engine&, std::ifstream& file );
		~Level();
	
		void save( const std::filesystem::path& path ) const;
		void unload();
		void load( std::ifstream& file );
		void load( const std::string& filename );

		const std::string& getName();
		const sf::Vector2u getSize();

		Tile* getTileAt(sf::Vector2f pos);

	};
};