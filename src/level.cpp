#include "include/level.hpp"
#include <box2d/box2d.h>
#include "SFML/Graphics/RectangleShape.hpp"
#include "box2d/types.h"
#include "imgui.h"
#include "include/engine.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/System/Vector2.hpp"
#include "include/arcade_errors.hpp"
#include "include/localizer.hpp"

#include "include/portable-file-dialogs.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

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

using Arcade::Error;

Arcade::TileRegistryEntry::TileRegistryEntry( const std::shared_ptr<sf::Texture>& texture, TileRenderMode render_mode ) : texture(texture), r_mode(render_mode) {}
Arcade::TileRegistryEntry::TileRegistryEntry( const std::weak_ptr<sf::Texture>& texture, TileRenderMode render_mode ) : texture( texture ), r_mode(render_mode) {}

const std::weak_ptr<sf::Texture> Arcade::TileRegistryEntry::getTexture() {
	return this->texture;
}
const char* Arcade::TileRegistryEntry::RModeString(TileRenderMode m) {
	static const char* RenderModes[] = {
		"Static",
		"Autotile",
		"Animated",
		"Random",
		"AutoTile_Random"
	};
	if( static_cast<int>(m) >= 0 || static_cast<int>(m) <= 4 ) {
		return RenderModes[static_cast<int>(m)];
	}
	return "";
}

Arcade::Tile::Tile( uint8_t flags ) : flags(flags) {}
Arcade::Tile::Tile( const std::shared_ptr<TileRegistryEntry>& entry ) : registryObject(entry), shape({TILE_SIZE,TILE_SIZE}) {}
Arcade::Tile::Tile( const std::shared_ptr<TileRegistryEntry>& entry, uint8_t flags) : flags(flags), registryObject(entry), shape({TILE_SIZE,TILE_SIZE}) {}
Arcade::Tile::Tile( Arcade::Engine& engine, const std::string& entry ) : shape({TILE_SIZE,TILE_SIZE}) {
	registryObject = engine.getTileEntry( entry );
}
Arcade::Tile::~Tile() {}
void Arcade::Tile::setup_texture() {

	if( registryObject.expired() || (this->flags & Tile::Flags::Empty) ) return;
	shape.setSize({TILE_SIZE, TILE_SIZE});
	shape.setTexture( registryObject.lock()->texture.lock().get() );

	switch( registryObject.lock()->r_mode ) {
        case TileRenderMode::Static:
			// Simply set the rect to the entire texture
			this->shape.setTextureRect(sf::Rect<int>(
				{ 0,0 },
				static_cast<sf::Vector2i>(registryObject.lock()->texture.lock()->getSize())
			));
			break;
        case TileRenderMode::Autotile: {
			// We need to use the neighbor calculation here
			uint8_t t_index = this->neighbor.to_ulong();
			sf::Vector2u t_size = this->registryObject.lock()->texture.lock()->getSize();
			this->shape.setTextureRect({
				{
					static_cast<int>((t_index % 4) * (t_size.x / 4)),
					static_cast<int>((t_index / 4) * (t_size.y / 4))
				},
				{
					static_cast<int>(t_size.x / 4),
					static_cast<int>(t_size.y / 4)
				}
			});

			break;
		}
		case TileRenderMode::Animated:
        case TileRenderMode::Random:
        case TileRenderMode::Autotile_Random:
    		break;
	}
}
const sf::FloatRect& Arcade::Tile::getBounds() {
	return this->bounds;
}
void Arcade::Tile::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	if( this->registryObject.expired() || (this->flags & Tile::Flags::Empty) ) return;
	target.draw( this->shape, states );
}
std::weak_ptr<Arcade::TileRegistryEntry> Arcade::Tile::getRegistryEntry() {
	return this->registryObject;
}

void Arcade::Level::doTilePostInit() {
	edges.clear();
	for( size_t i = 0; i < tiles.size(); i++ ) {
		uint32_t x = i % world_size.x;
		uint32_t y = i / world_size.x;
		
		// Calculate the tile bounds
		tiles[i].bounds = {
			{
				(x * TILE_SIZE),
				(y * TILE_SIZE)
			},
			{TILE_SIZE, TILE_SIZE}	
		};
		// First, set up the neighbor nibble
		tiles[i].neighbor.reset();
		if(tiles[i].flags & Arcade::Tile::Flags::Empty) continue;
		
		for(int it = 0; it < 4; it++) {
			uint32_t it_x = (it % 2)*(std::signbit(2-it)?-1:1) + x;
			uint32_t it_y = ((it+1) % 2)*(std::signbit(1-it)?1:-1) + y;
			
			uint32_t index = (it_y * world_size.x) + it_x;
			
			if( it_x < world_size.x && it_y < world_size.y && index < tiles.size() ) {
				if( !tiles[index].registryObject.expired() && !tiles[i].registryObject.expired() ) {
					tiles[i].neighbor.set( it,
						tiles[index].registryObject.lock()
						==
						tiles[i].registryObject.lock()
					);
				}
				tiles[i].g_neighbor.set( it,
					!(tiles[index].flags & Tile::Flags::Empty)
				);
			}else { // Everything outside the map is empty space
				tiles[i].g_neighbor.reset( it );
			}
		}
		
		tiles[i].setup_texture();

		if(!tiles[i].g_neighbor.test(0)) {
			edges.emplaceEdge(sf::Vector2f{
				tiles[i].bounds.position.x,
				tiles[i].bounds.position.y
			},
			sf::Vector2f{
				tiles[i].bounds.position.x + TILE_SIZE,
				tiles[i].bounds.position.y
			});
		}
		if(!tiles[i].g_neighbor.test(1)) {{
			edges.emplaceEdge(sf::Vector2f{
				tiles[i].bounds.position.x + TILE_SIZE,
				tiles[i].bounds.position.y
			},
			sf::Vector2f{
				tiles[i].bounds.position.x + TILE_SIZE,
				tiles[i].bounds.position.y + TILE_SIZE
			});
		}}
		if(!tiles[i].g_neighbor.test(2)) {
			edges.emplaceEdge(sf::Vector2f{
				tiles[i].bounds.position.x + TILE_SIZE,
				tiles[i].bounds.position.y + TILE_SIZE
			},
			sf::Vector2f{
				tiles[i].bounds.position.x,
				tiles[i].bounds.position.y + TILE_SIZE
			});
		}
		if(!tiles[i].g_neighbor.test(3)) {
			edges.emplaceEdge(sf::Vector2f{
				tiles[i].bounds.position.x,
				tiles[i].bounds.position.y + TILE_SIZE
			},
			sf::Vector2f{
				tiles[i].bounds.position.x,
				tiles[i].bounds.position.y
			});
		}
	}

	if( b2Body_IsValid(ground) ) {
		b2DestroyBody( ground );
	}
	b2BodyDef def = b2DefaultBodyDef();
	ground = b2CreateBody( engine.getWorld(), &def );
	
	auto lists = edges.getShapes();
	edges.attachChainShapes( ground, this->chains );
}

Arcade::Level::Level( Arcade::Engine& engine ) : engine(engine) {}
Arcade::Level::Level( Arcade::Engine& engine, std::ifstream& file ) : engine(engine) {
	this->load( file );
}
Arcade::Level::~Level() {}

void Arcade::Level::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	for( auto& tile : tiles ) {
		sf::RenderStates n_state( states );
		n_state.transform.translate( tile.bounds.position );
		tile.draw( target, n_state );
	}
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

	std::cout << "\e[32mWorld Size is " << world_x << "x" << world_y << "\e[0m" << std::endl;
	
	std::unordered_map<uint16_t, std::string> tile_registry_dict;
	uint16_t dict_size;
	file.read( (char*)&dict_size, 2u );
	dict_size = be16toh( dict_size );

	std::cout << "\e[32mDictionary has " << dict_size << " entries\e[0m" << std::endl;

	for( uint16_t it = 0; it < dict_size; it++ ) {
		uint16_t n;
		file.read( (char*)&n, 2u );
		n = be16toh( n );
		std::string identifier;
		char c;
		while( file.read(&c, 1u) ) {
			if(c == '\0') break;
			identifier.insert( identifier.end(), c );
		}

		std::cout << "\t\e[32mEntry " << n << " refers to " << identifier << "\e[0m" << std::endl;

		// Final sanity checks
		if( identifier.length() < 1 ) continue;
		tile_registry_dict.emplace( n, identifier );
	}

	// Now we sequentially read in each tile
	for( size_t i = 0; i < world_x*world_y; i++ ) {

		uint8_t tile_header;
		file.read( (char*)&tile_header, 1u );

		if( (tile_header & Tile::Flags::Empty) ) {
			Tile& tile = tiles.emplace_back( Tile(tile_header) );
			continue;
		}

		uint16_t tile;
		file.read( (char*)&tile, 2u );
		tile = be16toh( tile );

		try {
			tiles.emplace_back(
				Tile(engine.getTileEntry(tile_registry_dict.at(tile)).lock(), tile_header)
			);
		}catch(Error<ErrorType::RESOURCE_ERROR> e) {
			std::cerr << "\e[1;33mFailed to load resource: " << tile << " (aka) '" << tile_registry_dict.at(tile) << "'. Defaulting to empty tile\e[0m" << std::endl;
			tiles.emplace_back( Tile(tile_header) );
			assert(false);
		}catch( std::out_of_range e ) {
			std::cerr << "\e[1;31mHey stupid! You screwed up your chunk dictionary. " << (int)tile << " wasn't in the dictionary" << "\n\t" << "Defaulting to empty tile\e[0m" << std::endl;
			tiles.emplace_back( Tile(tile_header) );
			assert(false);
		}
		
	}

	doTilePostInit();
}
void Arcade::Level::load( const std::string& filename ) {
	std::ifstream file( filename );
	if( file ) {
		this->load( file );
	}
}
void Arcade::Level::unload() {
	this->world_size = {0,0};
	this->tl_key.clear();
	this->tiles.clear();
}

void Arcade::Level::save( const std::filesystem::path& path ) const {
	std::ofstream file( path, std::ios::binary | std::ios::trunc | std::ios::out );
	if( file ) {
		uint32_t magic = htobe32(ALV_MAGIC);
		file.write((char*)&magic, 4u);
		file.write( tl_key.c_str(), tl_key.length() );
		file.write("\0", 1u);
		uint32_t world_x = htobe32(world_size.x);
		uint32_t world_y = htobe32(world_size.y);
		file.write( (char*)&world_x, 4u );
		file.write( (char*)&world_y, 4u );

		// Time to build dictionary!
		std::vector<std::string> added_tiles;
		for( unsigned int i = 0; i < tiles.size(); i++ ) {
			if( added_tiles.size() == std::numeric_limits<uint16_t>::max() ) break;
			if( tiles.at(i).flags & Tile::Flags::Empty ) continue;
			std::string& rname = tiles.at(i).registryObject.lock()->registry_name;
			if(std::find(added_tiles.begin(), added_tiles.end(), rname) == added_tiles.end()) {
				added_tiles.emplace_back( rname );
			}
		}
		// Now we write the dictionary
		uint16_t dict_size = static_cast<uint16_t>(added_tiles.size());
		assert( dict_size == added_tiles.size() && "Lossy conversion! Tile dictionary is too large!" );
		dict_size = htobe16(dict_size);
		file.write((char*)&dict_size, 2u);
		for( uint16_t i = 0; i < added_tiles.size(); i++ ) {
			std::cout << "Writing entry " << i << " of " << added_tiles.size() << std::endl;
			uint16_t be_i = htobe16(i);
			file.write( (char*)&be_i, 2u );
			file.write( added_tiles[i].c_str(), added_tiles[i].size() );
			file.write("\0", 1u);
		}

		for( auto t : tiles ) {
			file.write( (char*)&t.flags, 1u );
			if( (t.flags & Tile::Flags::Empty) ) continue;

			auto it = std::find(added_tiles.begin(), added_tiles.end(), t.registryObject.lock()->registry_name);
			if( it == added_tiles.end() ) {
				file.write("\0", 1u);
			}else {
				uint16_t i = htobe16(std::distance(added_tiles.begin(), it));
				file.write((char*)&i, 2u);
			}

		}

		file.close();
		
	}else {
		std::cerr << "\e[1;31mFailed to save file to " << path << "\e[0m" << std::endl;
	}
}

void Arcade::Level::placeTileAt( sf::Vector2u grid_pos, const Tile& tile ) {

	if( grid_pos.x >= world_size.x ) {
		// Calculate the insert positions:
		for( long y = world_size.y - 1; y >= 0; y-- ) {
			tiles.insert( tiles.begin() + (y * world_size.x + world_size.x), grid_pos.x - world_size.x + 1, Tile(Tile::Flags::Empty) );
		}
		world_size.x = grid_pos.x + 1;
	}
	if( grid_pos.y >= world_size.y ) {
		tiles.insert( tiles.end(), world_size.x * (grid_pos.y - world_size.y + 1), Tile(Tile::Flags::Empty) );
		world_size.y = grid_pos.y + 1;
	}

	tiles[grid_pos.y * world_size.x + grid_pos.x] = tile;

	doTilePostInit();
}
void Arcade::Level::removeTileAt( sf::Vector2u grid_pos ) {
	size_t i = (grid_pos.y * world_size.x + grid_pos.x);
	if( i > tiles.size() ) return;

	tiles[i] = Tile(Tile::Flags::Empty);

	uint32_t max_x = 1;
	uint32_t max_y = 1;
	for (uint32_t y = 0; y < world_size.y; ++y) {
		for (uint32_t x = 0; x < world_size.x; ++x) {
			// Assume that a tile equal to Tile(Tile::Flags::Empty) is empty.
			// Adjust this comparison if you have a different mechanism.
			if (!(tiles[y * world_size.x + x].flags & Tile::Flags::Empty)) {
				max_x = std::max(max_x, x + 1);
				max_y = std::max(max_y, y + 1);
			}
		}
	}

	if( max_x < world_size.x || max_y < world_size.y ) {
		std::vector<Tile> n_tiles(max_x * max_y, Tile(Tile::Flags::Empty));
	
		for(uint32_t y = 0; y < max_y; y++) {
			for(uint32_t x = 0; x < max_x; x++) {
				n_tiles[y * max_x + x] = tiles[y * world_size.x + x];
			}
		}

		tiles.swap( n_tiles );
		world_size = sf::Vector2u{max_x, max_y};
	}

	doTilePostInit();
}

void Arcade::Level::drawEditor() {

	ImGui::Begin("Tilemap Debugger");

	for( size_t i = 0; i < tiles.size(); i++ ) {
		ImGui::PushID(i);
		if( ImGui::TreeNode(std::format("Tile {}", i).c_str()) ) {

			ImGui::Text("(%f, %f) %fx%f", tiles[i].bounds.position.x, tiles[i].bounds.position.y, tiles[i].bounds.size.x, tiles[i].bounds.size.y );

			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	ImGui::End();

	static sf::RectangleShape shape(sf::Vector2f({TILE_SIZE, TILE_SIZE}));
	shape.setFillColor( sf::Color::Transparent );
	shape.setOutlineColor( sf::Color::Green );
	shape.setOutlineThickness( 1.0f );

	ImGui::Begin("Level Editor", NULL, ImGuiWindowFlags_MenuBar );
	if( ImGui::BeginMenuBar() ) {
		if( ImGui::BeginMenu("File") ) {

			if(ImGui::MenuItem("Open File", "Ctrl + O")) {
				auto inf = pfd::open_file("Open Level",
					std::filesystem::absolute("../levels"),
					{"Arcade Level Files", "*.alv *.alvl"}
				).result();
				if( inf.size() > 0 ) {
					this->unload();
					this->load( inf[0] );
				}
			}
			if(ImGui::MenuItem("Save As...", "Ctrl + S")) {
				auto of = pfd::save_file("Saving level...",
				std::filesystem::absolute("../levels"),
				{
					"Arcade Level Files", "*.alv *.alvl"
				}).result();
				if( !of.empty() ) {
					this->save( of );
					std::cout << "Saved " << of << " successfully!" << std::endl;
				}
			}

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Text("%s\n\t-> %s", tl_key.c_str(),	reinterpret_cast<const char*>(Localizer::getTranslation(tl_key).c_str()));
	ImGui::Text( "Current Size: " );
	ImGui::SameLine();
	ImGui::TextColored( sf::Color::Magenta, "%ux%u Chunks", world_size.x, world_size.y );

	static LevelEditorTool selected_tool = LevelEditorTool::None;

	if(ImGui::Button("None")) {
		selected_tool = LevelEditorTool::None;
	}
	ImGui::SameLine();
	if(ImGui::Button("Draw")) {
		selected_tool = LevelEditorTool::TileDraw;
	}
	ImGui::SameLine();
	if(ImGui::Button("Select")) {
		selected_tool = LevelEditorTool::TileSelect;
	}

	switch( selected_tool ) {
		case None:
			break;
		case TileDraw: {
			ImGui::SeparatorText("Tile Selection");
			static auto cflags = ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY;
			ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f,2.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
			ImGui::BeginChild("Tile Selector", ImVec2(), cflags );
				
				constexpr static float IMAGE_SIZE = 30.0f;
				int n_tiles = static_cast<int>((ImGui::GetWindowWidth() - 4.0f - 10.0f) / (IMAGE_SIZE + 4.0f));
		
				static std::string selected_tile_entry;
				for( auto it = engine.tile_registry.begin(); it != engine.tile_registry.end(); it++ ) {
					size_t i = std::distance( engine.tile_registry.begin(), it );
					if( i % n_tiles ) ImGui::SameLine(); 
					ImGui::PushID(i);
					if(ImGui::ImageButton( std::format("TileButtonLabel_%zu", i).c_str(), *it->second->getTexture().lock(), {IMAGE_SIZE, IMAGE_SIZE} )) {
						selected_tile_entry = it->first;
					}
					static ImU32 col_default = IM_COL32(255, 255, 255, 255);
					static ImU32 col_selected = IM_COL32(255, 55, 55, 255);
					if(ImGui::IsItemHovered()) {
						if(ImGui::BeginItemTooltip()) {
							ImGui::Text("%s", it->first.c_str());
							ImGui::Text("Render Mode: ");
							ImGui::SameLine();
							ImGui::TextColored( sf::Color::Blue, "%s", TileRegistryEntry::RModeString( it->second->getRenderMode() ) );
							
							ImGui::NewLine();
							auto im = it->second->getTexture().lock();
							float aspect_ratio = static_cast<float>((*im).getSize().x) / static_cast<float>((*im).getSize().y);
							ImGui::Image( *im, {
								140.0f,
								140.0f * aspect_ratio
							} );
							
							ImGui::EndTooltip();
						}
					}
					ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 
						(selected_tile_entry.compare(it->first) == 0)? col_selected:col_default 
					);
					ImGui::PopID();
				}
			
			ImGui::EndChild();
			ImGui::PopStyleVar(3);

			auto p = engine.getWindow().mapPixelToCoords( static_cast<sf::Vector2i>(ImGui::GetMousePos()) );
			if( p.x > 0.f && p.y > 0.f ) {
				sf::Vector2u g_pos{static_cast<unsigned int>(std::floor( p.x / TILE_SIZE )), static_cast<unsigned int>(std::floor( p.y / TILE_SIZE ))};
				shape.setPosition({ g_pos.x * TILE_SIZE, g_pos.y * TILE_SIZE });
				shape.setSize({ TILE_SIZE, TILE_SIZE });
				engine.getWindow().draw( shape );

				if( engine.bindManager.startedPressing("Editor:Tool:Primary") && !selected_tile_entry.empty() ) {
					Tile t( engine, selected_tile_entry );
					placeTileAt( g_pos, t );
				}else if ( engine.bindManager.startedPressing("Editor:Tool:Secondary") && !selected_tile_entry.empty()  ) {
					removeTileAt( g_pos );
				}
			}
		
			break;
		}
		case TileSelect: {
			ImGui::SeparatorText("Tile Properties");

			static Tile* selected_tile;

			if( selected_tile != nullptr ) {
				ImGui::Text("Flags: 0x%X", selected_tile->flags);

				static sf::RectangleShape selected_outline({TILE_SIZE, TILE_SIZE});
				selected_outline.setFillColor( sf::Color::Transparent );
				selected_outline.setOutlineColor( sf::Color::Blue );
				selected_outline.setOutlineThickness( .5f );

				selected_outline.setPosition( selected_tile->bounds.position );

				engine.getWindow().draw( selected_outline );
			}

			if( engine.bindManager.startedPressing("Editor:Tool:Primary") ) {
				auto p = engine.getWindow().mapPixelToCoords( static_cast<sf::Vector2i>(ImGui::GetMousePos()) );
				selected_tile = getTileAt( p );
			}
		}
	};
	
	ImGui::End();

	// Helper
	static sf::RectangleShape outline;
	outline.setFillColor( sf::Color::Transparent );
	outline.setOutlineColor( sf::Color::White );
	outline.setOutlineThickness( 0.5f );

	outline.setPosition({0.f,0.f});
	outline.setSize( {world_size.x * TILE_SIZE, world_size.y * TILE_SIZE } );

	engine.getWindow().draw( outline );

}

const std::string& Arcade::Level::getName() {
	return this->tl_key;
}
const sf::Vector2u Arcade::Level::getSize() {
	return this->world_size;
}

Arcade::Tile* Arcade::Level::getTileAt( sf::Vector2f pos ) {
	size_t x = std::floor( pos.x / TILE_SIZE );
	size_t y = std::floor( pos.y / TILE_SIZE );
	if( x < 0 || x >= world_size.x || y < 0 || y >= world_size.y )
		return nullptr;
	try {
		return &tiles.at( y * world_size.x + x );
	}catch( std::out_of_range e ) {
		std::cout << "Failed to get chunk." << x << ", " << y << std::endl;
		return nullptr;
	}
}