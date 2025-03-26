#include "include/level.hpp"
#include <box2d/box2d.h>
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
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

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

#define TILE_SIZE 5.f

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
Arcade::Tile::Tile( Arcade::Engine& engine, const std::string& entry ) {
	engine.getTileEntry( entry );
}
Arcade::Tile::~Tile() {}
void Arcade::Tile::setup_texture() {

	if( registryObject.expired() ) return;
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
	if( this->registryObject.expired() ) return;
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

Arcade::Tile* Arcade::Chunk::getTileAt( sf::Vector2f pos ) {
	size_t x = std::floor( (pos.x - this->bounds.position.x) / TILE_SIZE );
	size_t y = std::floor( (pos.y - this->bounds.position.y) / TILE_SIZE );
	if( x<0||x>=size||y<0||y>=size) return nullptr;
	try {
		return &tiles.at( y * size + x );
	}catch( std::out_of_range e ) {
		std::cout << "Failed to get Tile." << x << ", " << y << " size is " << tiles.size() << std::endl;
		return nullptr;
	}
}

void Arcade::Chunk::doTilePostInit( Engine& engine ) {
	edges.clear();
	for( size_t i = 0; i < tiles.size(); i++ ) {
		size_t x = i % size;
		size_t y = i / size;
		
		// Calculate the tile bounds
		tiles[i].bounds = {
			{
				this->bounds.position.x + (x * TILE_SIZE),
				this->bounds.position.y + (y * TILE_SIZE)
			},
			{TILE_SIZE, TILE_SIZE}	
		};
		if(tiles[i].flags & Arcade::Tile::Flags::Empty) continue;
		// First, set up the neighbor nibble
		tiles[i].neighbor.reset();

		if( y != 0 ) { // Top
			tiles[i].neighbor.set( 0u,
				tiles[((y-1) * size) + x].registryObject.lock()
					==
				tiles[i].registryObject.lock()
			);
			tiles[i].g_neighbor.set( 0u,
				!(tiles[((y-1) * size) + x].flags & Tile::Flags::Empty)
			);
		}
		if( x != size-1 ) { // Right
			tiles[i].neighbor.set( 1u,
				tiles[(y * size) + (x+1)].registryObject.lock()
				==
				tiles[i].registryObject.lock()
			);
			tiles[i].g_neighbor.set( 1u,
				!(tiles[(y * size) + (x+1)].flags & Tile::Flags::Empty)
			);
		}
		if( y != size-1 ) { // Bottom
			tiles[i].neighbor.set( 2u,
				tiles[((y+1) * size) + x].registryObject.lock()
				==
				tiles[i].registryObject.lock()
			);
			tiles[i].g_neighbor.set( 2u,
				!(tiles[((y+1) * size) + x].flags & Tile::Flags::Empty)
			);
		}
		if( x != 0 ) { // Left
			tiles[i].neighbor.set( 3u,
				tiles[(y * size) + (x-1)].registryObject.lock()
				==
				tiles[i].registryObject.lock()
			);
			tiles[i].g_neighbor.set( 3u,
				!(tiles[(y*size) + (x-1)].flags & Tile::Flags::Empty)
			);
		}

		tiles[i].setup_texture();


		if(!tiles[i].g_neighbor.test(0)) {
			edges.emplaceEdge(sf::Vector2f{
				(x * TILE_SIZE),
				(y * TILE_SIZE)
			},
			sf::Vector2f{
				(x * TILE_SIZE) + TILE_SIZE,
				(y * TILE_SIZE)
			});
		}
		if(!tiles[i].g_neighbor.test(1)) {{
			edges.emplaceEdge(sf::Vector2f{
				(x * TILE_SIZE) + TILE_SIZE,
				(y * TILE_SIZE)
			},
			sf::Vector2f{
				(x * TILE_SIZE) + TILE_SIZE,
				(y * TILE_SIZE) + TILE_SIZE
			});
		}}
		if(!tiles[i].g_neighbor.test(2)) {
			edges.emplaceEdge(sf::Vector2f{
				(x * TILE_SIZE) + TILE_SIZE,
				(y * TILE_SIZE) + TILE_SIZE
			},
			sf::Vector2f{
				(x * TILE_SIZE),
				(y * TILE_SIZE) + TILE_SIZE
			});
		}
		if(!tiles[i].g_neighbor.test(3)) {
			edges.emplaceEdge(sf::Vector2f{
				(x * TILE_SIZE),
				(y * TILE_SIZE) + TILE_SIZE
			},
			sf::Vector2f{
				(x * TILE_SIZE),
				(y * TILE_SIZE)
			});
		}
	}

	b2BodyDef def = b2DefaultBodyDef();
	def.position = b2Vec2{ bounds.position.x, bounds.position.y };

	if( b2Body_IsValid( ground ) ) {
		// If we re-init the chunk, destroy the old bodys
		b2DestroyBody( ground );
	}
	ground = b2CreateBody( engine.getWorld(), &def );

	auto lists = edges.getShapes();
	edges.attachChainShapes( ground );
}
const uint8_t Arcade::Chunk::getFlags() const {
	return this->flags;
}
const sf::FloatRect Arcade::Chunk::getBounds() const {
	return this->bounds;
}

Arcade::Level::Level( Arcade::Engine& engine, std::ifstream& file ) : chunks(  ) {
	this->load( engine, file );
}
Arcade::Level::~Level() {}

void Arcade::Level::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	for( auto& [coord, chunk] : chunks ) {
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
		Chunk& chunk = chunks.try_emplace(sf::Vector2u{
			static_cast<unsigned int>(i % world_x),
			static_cast<unsigned int>(i / world_x)
		}).first->second;
		file.read( (char*)&chunk.flags, 1u );

		chunk.bounds = {
			{(i % world_x)*(chunk_size*TILE_SIZE), static_cast<int>(i / world_x)*(chunk_size*TILE_SIZE)},
			{ chunk_size*TILE_SIZE, chunk_size*TILE_SIZE }
		};
		chunk.size = chunk_size;

		if( !(chunk.flags & Chunk::Flags::HasTiles) ) {
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

		chunk.doTilePostInit( engine );

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
	this->world_size = {0,0};
	this->tl_key.clear();
	this->chunks.clear();
}

void Arcade::Level::save( const std::filesystem::path& path ) const {
	std::ofstream file( path, std::ios::binary | std::ios::trunc | std::ios::out );
	if( file ) {
		uint32_t magic = htobe32(ALV_MAGIC);
		file.write((char*)&magic, 4u);
		file.write( tl_key.c_str(), tl_key.length() );
		file.write("\0", 1u);
		uint32_t world_x = htobe32(world_size.x);
		uint32_t world_y = htobe32(world_size.x);
		file.write( (char*)&world_x, 4u );
		file.write( (char*)&world_y, 4u );
		uint32_t c_size = htobe32(chunk_size);
		file.write((char*)&c_size, 4u);

		std::vector<const std::pair<const sf::Vector2u, Chunk>*> sortedChunks;
		for( const auto& pair : this->chunks ) {
			sortedChunks.push_back( &pair );
		}

		std::sort( sortedChunks.begin(), sortedChunks.end(), [this]( const auto* a, const auto* b ) {
			uint32_t indexA = a->first.y * this->world_size.x + a->first.x;
			uint32_t indexB = b->first.y * this->world_size.x + b->first.x;
			return indexA < indexB;
		});

		for( auto chunk : sortedChunks ) {
			auto [k, c] = *chunk;
			file.write( (char*)&c.flags, 1u );
			if( !(c.flags & Chunk::Flags::HasTiles) ) continue;

			// Time to build dictionary!
			std::vector<std::string> added_tiles;
			for( int i = 0; i < c.tiles.size(); i++ ) {
				if( added_tiles.size() == 255 ) break;
				if( c.tiles[i].flags & Tile::Flags::Empty ) continue;
				std::string& rname = c.tiles[i].registryObject.lock()->registry_name;
				if(std::find(added_tiles.begin(), added_tiles.end(), rname) == added_tiles.end()) {
					added_tiles.emplace_back( rname );
				}
			}
			// Now we write the dictionary, up to a byte of entries
			uint8_t size = static_cast<uint8_t>(added_tiles.size());
			file.write((char*)&size, 1u);
			for( uint8_t i = 0; i < size; i++ ) {
				file.write( (char*)&i, 1u );
				file.write( added_tiles[i].c_str(), added_tiles[i].size() );
				file.write("\0", 1u);
			}

			// Time to write tile data!
			for( Tile t : c.tiles ) {
				file.write( (char*)&t.flags, 1u );
				if( t.flags & Tile::Flags::Empty ) continue;
				// TODO: Tile data goes here
				auto it = std::find(added_tiles.begin(), added_tiles.end(), t.registryObject.lock()->registry_name);
				if( it == added_tiles.end() ) {
					file.write("\0", 1u);
				}else {
					uint8_t i = std::distance(added_tiles.begin(), it);
					file.write((char*)&i, 1u);
				}
			}

		}

		file.close();
		
	}else {
		std::cerr << "\e[1;31mFailed to save file to " << path << "\e[0m" << std::endl;
	}
}

void Arcade::Level::resize( uint32_t target_x, uint32_t target_y ) {
	// Here we erase any chunks that are outside our target size
	for( auto& [ coord, chunk ] : chunks ) {
		if( coord.x >= target_x || coord.y >= target_y ) {
			chunks.erase( coord );
		}
	}

	for( uint32_t x = 0; x < target_x; x++ ) {
		for( uint32_t y = 0; y < target_y; y++ ) {
			// If we don't have a chunk at this position, make one
			if( chunks.find(sf::Vector2u{x, y}) == chunks.end() ) {
				auto& chunk = chunks.try_emplace(sf::Vector2u{x, y}).first->second;
				chunk.size = this->chunk_size;
				chunk.flags = Chunk::Flags::HasTiles;
				chunk.bounds = sf::FloatRect{
					{ x*this->chunk_size*TILE_SIZE,y*this->chunk_size*TILE_SIZE },
					{ TILE_SIZE*this->chunk_size, TILE_SIZE*this->chunk_size }
				};
				// Populate it with empty tiles
				for(uint32_t i = 0; i < this->chunk_size*this->chunk_size; i++) {
					chunk.tiles.emplace_back( Tile::Flags::Empty );
				}
			}
		}
	}

	this->world_size.x = target_x;
	this->world_size.y = target_y;
}

void Arcade::Level::drawEditor( Arcade::Engine& engine ) {

	ImGui::Begin("Chunk Debugger");

	for( const auto& [k, chunk] : chunks ) {
		ImGui::PushID( k.y * world_size.x + k.x );

		if( ImGui::TreeNode("", "Chunk (%d, %d)", k.x, k.y) ) {

			ImGui::TextColored( sf::Color::Magenta, "Size %dx%d", chunk.size, chunk.size );
			ImGui::TextColored( sf::Color::Cyan, "(%f, %f) %fx%f", chunk.bounds.position.x, chunk.bounds.position.y, chunk.bounds.size.x, chunk.bounds.size.y );
			ImGui::TextColored( sf::Color::Cyan, "(Flags 0x%X", chunk.flags );
			ImGui::TextColored( sf::Color::Magenta, "#tiles: %zu", chunk.tiles.size() );
			// ImGui::TextColored( sf::Color::Magenta, "#shapes: %zu", chunk.shapes.size() );

			// for( size_t i = 0; i < chunk.shapes.size(); i++ ) {
			// 	ImGui::PushID( &(chunk.shapes[i]) );
			// 	std::string s;
			// 	sprintf( s.data(), "Shape %zu", i );
			// 	ImGui::SeparatorText( s.c_str() );
			// 	for( size_t i2 = 0; i2 < chunk.shapes[i].m_count; i2++ ) {
			// 		const auto& p = chunk.shapes[i].m_vertices[i2];
			// 		ImGui::Text("Point %zu: (%f, %f)", i2, p.x, p.y);
			// 	}
			// 	ImGui::PopID();
			// }

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
	bool resize_popup{};
	if( ImGui::BeginMenuBar() ) {
		if( ImGui::BeginMenu("File") ) {

			if(ImGui::MenuItem("Open File", "Ctrl + O")) {
				auto inf = pfd::open_file("Open Level",
					std::filesystem::absolute("../levels"),
					{"Arcade Level Files", "*.alv *.alvl"}
				).result();
				if( inf.size() > 0 ) {
					this->unload();
					this->load( engine, inf[0] );
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
		if( ImGui::BeginMenu("Level") ) {
			if( ImGui::MenuItem("Resize") ) {
				resize_popup = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	if(resize_popup) ImGui::OpenPopup("LevelEditor_Resize");
	if( ImGui::BeginPopup("LevelEditor_Resize") ) {
		ImGui::Text("World Size: ");
		ImGui::SameLine();
		static int resize_target[2] = {0, 0};
		ImGui::InputInt2("##1", resize_target);
		ImGui::Text("Chunk Size: ");
		ImGui::SameLine();
		if( ImGui::Button("Resize") ) {
			resize( resize_target[0], resize_target[1] );
		}
		ImGui::EndPopup();
	}

	ImGui::Text("%s\n\t-> %s", tl_key.c_str(),	reinterpret_cast<const char*>(Localizer::getTranslation(tl_key).c_str()));
	ImGui::Text( "Current Size: " );
	ImGui::SameLine();
	ImGui::TextColored( sf::Color::Magenta, "%ux%u Chunks", world_size.x, world_size.y );

	ImGui::Text("Chunk Size: ");
	ImGui::SameLine();
	ImGui::TextColored( sf::Color::Magenta, "%ux%u Tiles", chunk_size, chunk_size );

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
					std::string s;
					sprintf(s.data(), "TileButtonLabel_%zu", i);
					if(ImGui::ImageButton( s.c_str(), *it->second->getTexture().lock(), {IMAGE_SIZE, IMAGE_SIZE} )) {
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
			auto chunk = this->getChunkAt( p );
			if( chunk != nullptr ) {
				auto tile = chunk->getTileAt( p );
				if( tile != nullptr ) {
					shape.setPosition( tile->bounds.position );
					shape.setSize( tile->bounds.size );
					engine.getWindow().draw( shape );

					if( engine.bindManager.startedPressing("Editor:Tool:Primary") && !selected_tile_entry.empty() ) {
						tile->flags &= ~Tile::Flags::Empty;
						tile->registryObject = engine.getTileEntry( selected_tile_entry );
						chunk->doTilePostInit( engine );
					}else if ( engine.bindManager.startedPressing("Editor:Tool:Secondary") && !selected_tile_entry.empty()  ) {
						tile->flags |= Tile::Flags::Empty;
						tile->registryObject.reset();
						chunk->doTilePostInit( engine );
					}
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
				selected_outline.setOutlineThickness( 5.0f );

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

Arcade::Chunk* Arcade::Level::getChunkAt( sf::Vector2f pos ) {
	size_t x = std::floor( pos.x / (chunk_size * TILE_SIZE) );
	size_t y = std::floor( pos.y / (chunk_size * TILE_SIZE) );
	if( x < 0 || x >= world_size.x || y < 0 || y >= world_size.y )
		return nullptr;

	try {
		return &chunks.at(sf::Vector2u{
			static_cast<unsigned int>(x), static_cast<unsigned int>(y)
		});
	}catch( std::out_of_range e ) {
		std::cout << "Failed to get chunk." << x << ", " << y << " size is " << chunks.size() << std::endl;
		return nullptr;
	}
}
Arcade::Tile* Arcade::Level::getTileAt( sf::Vector2f pos ) {
	Chunk* chunk = this->getChunkAt( pos );
	if( chunk != nullptr ) {
		return chunk->getTileAt( pos );
	}
	return nullptr;
}