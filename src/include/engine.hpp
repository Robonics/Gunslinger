#pragma once

#include "SFML/Graphics/Texture.hpp"
#include "SFML/Window/VideoMode.hpp"
#include "SFML/Window/WindowEnums.hpp"
#include "arcade_errors.hpp"
#include "bind.hpp"
#include "imgui_internal.h"
#include "level.hpp"
#include "localizer.hpp"
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <typeinfo>

namespace Arcade {

	class VEventWrapper {
	public:
		virtual ~VEventWrapper() = default;
		virtual bool isOnce() = 0;
		const virtual std::type_info& getType() = 0;
		virtual bool runIf(std::optional<sf::Event> evt) = 0;
	};

	template <typename EVENT_T>
	class EventWrapper : public VEventWrapper {
		std::function<void(EVENT_T)> callback;
		bool is_once;

		// static std::variant<> EventTypes;

	public:
		EventWrapper( std::function<void(EVENT_T)> cback, bool once=false ) : callback(cback), is_once(once) {
			// TODO: Assert type : EventTypes
		}

		bool isOnce() override {
			return this->is_once;
		}

		const std::type_info& getType() override {
			return typeid( EVENT_T );
		}
		bool runIf( std::optional<sf::Event> evt ) override {
			if( auto* e = evt->getIf<EVENT_T>() ) {
				this->callback( *e );
				return true;
			}
			return false;
		}
	};

	class Engine {
		friend class Level;

		constexpr const static uint8_t default_pixels[]{
			0x00,0x00,0x00,0xFF,	0xC5,0x3D,0xFF,0xFF,
			0xC5,0x3D,0xFF,0xFF,	0x00,0x00,0x00,0xFF
		};
		std::shared_ptr<sf::Texture> default_texture;

		std::string window_name;
		sf::VideoMode window_mode;
		sf::RenderWindow window;

		Level level;

		bool fullscreen = false;

		static sf::Clock g_time;
		sf::Time frame_time;
		sf::View camera;

		std::unordered_map<std::string, std::unique_ptr<VEventWrapper>> events;
		std::unordered_map<std::string, std::shared_ptr<sf::Texture>> texture_registry;
		std::unordered_map<std::string, std::shared_ptr<TileRegistryEntry>> tile_registry;

		bool debug_open{};
		void debug_imgui() {
			ImGui::Begin("Engine Registries");
			if(ImGui::BeginTabBar("EventRegistriesTabBar")) {
				static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;
				if(ImGui::BeginTabItem("Event Registry")) {
					ImGui::BeginTable("EventRegisryTable", 2, flags );
					ImGui::TableSetupColumn("Indentifier");
					ImGui::TableSetupColumn("Bound To");
					ImGui::TableHeadersRow();
					for( auto& pair : this->getRegisteredEvents() ) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text( "%s", pair.first.c_str() );
						ImGui::TableSetColumnIndex(1);
						ImGui::TextColored( sf::Color::Cyan, "%s", pair.second->getType().name() );
					}
					ImGui::EndTable();
					ImGui::EndTabItem();
				}
				if(ImGui::BeginTabItem("Bind Registry")) {
					ImGui::BeginTable("BindRegistryTable", 4, flags);
					ImGui::TableSetupColumn("Identifier");
					ImGui::TableSetupColumn("Modifier Keys");
					ImGui::TableSetupColumn("Key Code");
					ImGui::TableSetupColumn("Time Pressed");
					ImGui::TableHeadersRow();
					for( auto& pair : this->bindManager.getAllBinds() ) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text( "%s", pair.first.c_str() );
						ImGui::TableNextColumn();
						ImGui::Text( "%s%s%s%s", (pair.second.isShiftModified())? "S":"", (pair.second.isCtrlModified())?"C":"", (pair.second.isAltModified())?"A":"", (pair.second.isMetaModified())?"M":"" );
						ImGui::TableNextColumn();
						ImGui::TextColored( (pair.second.isPressed())? sf::Color::Green : sf::Color::Red, "%s%d", (pair.second.isMouse())? "M":"K", pair.second.getCode() );
						ImGui::TableNextColumn();
						ImGui::TextColored( sf::Color::Cyan, "%fs", pair.second.getTimePressed().asSeconds() );
					}
					ImGui::EndTable();
					ImGui::EndTabItem();
				}
				if(ImGui::BeginTabItem("Localization")) {
					ImGui::Text("Loaded %s", Localizer::getLoadedLanguage().c_str());
					ImGui::BeginTable("TranslationRegistryTable", 2, flags);
					ImGui::TableSetupColumn("Key");
					ImGui::TableSetupColumn("Translation");
					ImGui::TableHeadersRow();
					for( auto& pair : Localizer::getAllKeys() ) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text( "'%s'", pair.first.c_str() );
						ImGui::TableNextColumn();
						ImGui::Text( "%s", reinterpret_cast<const char*>(pair.second.c_str()) );
					}
					ImGui::EndTable();
					ImGui::EndTabItem();
				}
				if( ImGui::BeginTabItem("Tile Registry") ) {
					for(auto& [identifier, tile] : tile_registry) {
						ImGui::Text( "%s", identifier.c_str() );
						if( ImGui::IsItemHovered() ) {
							if( ImGui::BeginTooltip() ) {
								float aspect_ratio = (float)tile->getTexture().lock()->getSize().x / (float)tile->getTexture().lock()->getSize().y;
								if( aspect_ratio < 1.0f ) { aspect_ratio = 1.0f / aspect_ratio; } 
								ImGui::Image( *tile->getTexture().lock(), {200.0f * aspect_ratio, 200.0f} );
								ImGui::EndTooltip();
							}
						}
					}
					ImGui::EndTabItem();
				}
				if( ImGui::BeginTabItem("Texture Registry") ) {
					ImGui::BeginTable("TextureTable", 5, flags);
					ImGui::TableSetupColumn("Identifier");
					ImGui::TableSetupColumn("Size");
					ImGui::TableSetupColumn("Smooth?");
					ImGui::TableSetupColumn("Repeated?");
					ImGui::TableSetupColumn("Map offset");
					ImGui::TableHeadersRow();
					for(auto& [identifier, text] : texture_registry ) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text( "%s", identifier.c_str() );
						if( ImGui::TableGetHoveredRow() == ImGui::TableGetRowIndex() ) {
							if( ImGui::BeginTooltip() ) {
								float aspect_ratio = (float)text->getSize().x / (float)text->getSize().y;
								if( aspect_ratio < 1.0f ) { aspect_ratio = 1.0f / aspect_ratio; } 
								ImGui::Image( *text, {200.0f * aspect_ratio, 200.0f} );
								ImGui::EndTooltip();
							}
						}
						ImGui::TableNextColumn();
						ImGui::TextColored( sf::Color::Cyan, "%ux%u", text->getSize().x, text->getSize().y );
						ImGui::TableNextColumn();
						ImGui::TextColored( ((text->isSmooth())?sf::Color::Green:sf::Color::Red), "%s", (text->isSmooth())?"true":"false");
						ImGui::TableNextColumn();
						ImGui::TextColored( ((text->isRepeated())?sf::Color::Green:sf::Color::Red), "%s", (text->isRepeated())?"true":"false");
						ImGui::TableNextColumn();
						ImGui::TextColored( sf::Color::Magenta, "positioned at %ld", std::distance( texture_registry.begin(), texture_registry.find(identifier)));
					}
					ImGui::EndTable();
					ImGui::EndTabItem();
				}
				/// TODO \todo Needs to be fixed. This tab should not be enabled when a level is not loaded!
				if( ImGui::BeginTabItem("Level") ) {
					ImGui::Text( "%s ->\n\t%s", level.getName().data(),  reinterpret_cast<const char*>(Localizer::getTranslation(level.getName()).c_str()) );
					ImGui::Text("World Size: ");
					ImGui::SameLine();
					ImGui::TextColored( sf::Color::Magenta, "%ux%u (%u Chunks, %u Tiles)", level.getSize().x, level.getSize().y, level.getSize().x * level.getSize().y, level.getSize().x * level.getSize().y * level.getChunkSize() * level.getChunkSize() );
					ImGui::Text( "Chunk size: " ); ImGui::SameLine();
					ImGui::TextColored( sf::Color::Magenta, "%ux%u (%u tiles)", level.getChunkSize(), level.getChunkSize(), level.getChunkSize()*level.getChunkSize() );
					ImGui::Spacing();
					ImGui::Text("Engine Camera: %f, %f %fx%f", camera.getCenter().x, camera.getCenter().y, camera.getSize().x, camera.getSize().y);
					ImGui::Text("Window Camera: %f, %f %fx%f", window.getView().getCenter().x, window.getView().getCenter().y, window.getView().getSize().x, window.getView().getSize().y);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			ImGui::End();
		}

		void init_default_events() {
			this->registerEvent<sf::Event::Closed>("DEFAULT:WindowClosed", [this]( auto e ) {
				this->close();
			});
			this->registerEvent<sf::Event::KeyPressed>("DEFAULT:FullScreen", [this](auto e) {
				if( e.code == sf::Keyboard::Key::F11 ) {
					if( fullscreen )
						window.create( window_mode, window_name, sf::State::Windowed );
					else
						window.create( window_mode, window_name, sf::State::Fullscreen );

					fullscreen = !fullscreen;
				}
			});
		}

		static void recursive_load( const std::filesystem::path& path, std::function<void( const std::filesystem::path& )> cback, size_t max_depth=std::numeric_limits<size_t>::max(), size_t depth=0 ) {
			assert( std::filesystem::is_directory(path) );
			for( const auto& f : std::filesystem::directory_iterator(path) ) {
				if( f.is_directory() && depth < max_depth ) recursive_load( f, cback, max_depth, depth + 1 );
				else cback(f);
			}
		}

		void load_static_resources() {
			// Texutres
			const static std::filesystem::path t_path("../textures");
			if( std::filesystem::is_directory(t_path) ) {
				recursive_load( t_path, [this]( auto& f) {
					this->registerTexture( std::filesystem::relative(f, t_path), f );
				});
			}

			const static std::filesystem::path ts_path("../tilesets");
			if( std::filesystem::is_directory(ts_path) ) {
				recursive_load(ts_path, [this]( auto& f ) {
					std::ifstream file(f);
					if(!file) {
						std::cerr << "Failed to open file " << std::filesystem::relative(f, ts_path) << "" << std::endl;
						return;
					}

					uint64_t magic;
					file.read((char*)&magic, 8u);
					magic = be64toh( magic );
					if( magic != ATSET_MAGIC ) {
						std::stringstream err;
						err << "Failed to load tileset because magic was incorrect. Expected ";
						err << std::hex << ATSET_MAGIC;
						err << " but got " << std::hex << magic;
						throw Error<ErrorType::FILE_ERROR>( err.str(), ErrorType::FILE_BAD_MAGIC );
					}
				
					// ATSET is a simple format
					while( file ) {
						std::string tile_name;
						for( char c; file.read(&c, 1u); ) {
							if(c == '\0') break;
							tile_name.append( &c, 1u );
						}

						// Render mode!
						uint8_t r_mode;
						file.read( (char*)&r_mode, 1u );
						// TODO: Validate

						// Now the texture, we need the Engine for this.
						std::string texture_key;
						for( char c; file.read(&c, 1u) && file; ) {
							if(c == '\0') break;
							texture_key.append( &c, 1u );
						}

						this->registerTile( tile_name, texture_key, (TileRenderMode)r_mode );
					}
				});
			}

			ImFontConfig cfg;
			cfg.MergeMode = true;
			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->AddFontDefault( &cfg );
			auto* f = io.Fonts->AddFontFromFileTTF(std::filesystem::absolute("../fonts/NotoSansJP-Regular.ttf").c_str(), 18.0f, &cfg, io.Fonts->GetGlyphRangesJapanese());
			if( !f->IsLoaded() ) {
				std::cerr << "\e[33mFailled to load NotoSansJP\e[0m" << std::endl;
			}
			if( !io.Fonts->Build() ) {
				std::cerr << "\e[1;31mError! ImGui font building failed. The program WILL crash\e[0m" << std::endl;
			}
			if( !ImGui::SFML::UpdateFontTexture() ) {
				std::cerr << "\e[1;31mError! ImGui-SFML failed to update the font texture. The program WILL crash\e[0m" << std::endl;
			}
			
		}

	public:

		BindManager bindManager;

		Engine( std::string window_title ) : window_name( window_title ) {
			window_mode.size = window_mode.getDesktopMode().size.componentWiseDiv({2, 2});
			window.create( window_mode, window_title ); 
			camera = window.getView();

			default_texture = std::make_shared<sf::Texture>();
			if(default_texture->resize({2u,2u})) {
				default_texture->update( default_pixels );
			}

			if( !ImGui::SFML::Init( window ) ) {
				throw std::runtime_error("Failed to init ImGui!");
			};

			init_default_events();
			load_static_resources();

		}
		Engine( std::string window_title, sf::VideoMode dimensions ) : 
			window_name( window_title ),
			window_mode( dimensions ),
			window( dimensions, window_title ),
			camera( window.getView() ) {

			if( !ImGui::SFML::Init( window ) ) {
				throw std::runtime_error("Failed to init ImGui!");
			};

			init_default_events();
			load_static_resources();
		}
		~Engine() {
			close();
		}

		sf::RenderWindow& getWindow() {
			return this->window;
		}

		template <typename EVENT_T>
		bool registerEvent( std::string name, std::function<void(EVENT_T)> callback ) {
			if( events.find( name ) != events.end() ) return false;
			events.emplace( name, std::make_unique<EventWrapper<EVENT_T>>( callback ) );
			return true;
		}
		template <typename EVENT_T>
		bool registerEventOnce( std::string name, std::function<void(EVENT_T)> callback ) {
			if( events.find( name ) != events.end() ) return false;
			events.emplace( name, std::make_unique<EventWrapper<EVENT_T>>( callback, true ) );
			return true;
		}
		bool deregisterEvent( std::string name ) {
			if( events.find(name) == events.end() ) {
				return false;
			}
			events.erase( name );
			return true;
		}

		void handleEvents() {
			bindManager.pre_update();
			while( std::optional e = this->window.pollEvent() ) {
				ImGui::SFML::ProcessEvent( window, *e );
				bindManager.update( e );
				for( auto i = events.begin(); i != events.end(); ) {
					if((*i).second->runIf( e ) && (*i).second->isOnce()) {
						i = events.erase( i );
						continue;
					}
					i++;
				}
			}
		}

		const std::unordered_map<std::string, std::unique_ptr<VEventWrapper>>& getRegisteredEvents() {
			return events;
		}

		void update( sf::Time delta ) {
			frame_time = delta;
			ImGui::SFML::Update( window, delta );
		}

		void render() {
			window.clear();

			window.draw( level );

			if( bindManager.startedPressing("Meta:Debug") ) {
				debug_open = !debug_open;
			}
			if( debug_open ) {
				this->debug_imgui();
			}
			if( bindManager.startedPressing("Meta:Editor") ) {
				level.editor_open = !level.editor_open;
			}
			if( level.editor_open ) {
				level.drawEditor( *this );
			}

			ImGui::SFML::Render( window );
		}
		void display() {
			window.display();
		}

		std::weak_ptr<sf::Texture> getTexture( const std::string& identifier ) {
			if( texture_registry.find( identifier ) != texture_registry.end() ) {
				return texture_registry.at( identifier );
			}
			return default_texture;
		}
		/// Copy texture
		bool registerTexture( const std::string& identifier, const sf::Texture& copy_from ) {
			return texture_registry.emplace( identifier, std::make_shared<sf::Texture>( copy_from ) ).second;
		}
		/// Filename can be any path on the system
		bool registerTexture( const std::string& identifier, const std::filesystem::path& filename ) noexcept {
			auto result = texture_registry.emplace( identifier, std::make_shared<sf::Texture>() );
			if( result.second ) {
				return result.first->second->loadFromFile( filename );
			}
			return false;
		}

		bool registerTile( std::string_view identifier, const std::string& texture, TileRenderMode r_mode=TileRenderMode::Static ) noexcept {
			auto t = tile_registry.emplace( identifier, std::make_shared<TileRegistryEntry>( getTexture(texture), r_mode ) );
			if( t.second ) { t.first->second->registry_name = identifier; }
			return t.second;
		}

		std::weak_ptr<TileRegistryEntry> getTileEntry( const std::string& entry ) const {
			if( tile_registry.find(entry) == tile_registry.end() ) {
				std::stringstream err;
				err << "Failed to find a tile in the registry with name " << entry;
				throw Arcade::Error<Arcade::ErrorType::RESOURCE_ERROR>(err.str(), Arcade::ErrorType::RESOURCE_NOT_FOUND);
			}
			return tile_registry.at( entry );
		}

		sf::View& getCamera() {
			return this->camera;
		}
		void updateCamera() {
			this->window.setView( this->camera );
		}

		const sf::Time& getFrameTime() {
			return this->frame_time;
		}

		void close() {
			window.close();
			ImGui::SFML::Shutdown();
		}

		void loadLevel( const std::filesystem::path& path ) {
			level.~Level();
			std::ifstream file( path );
			if( file ) {
				try {
					level = Level( *this, file );
					std::cout << "Loaded level " << path << "!" << std::endl;
				}catch( Error<ErrorType::FILE_ERROR> e ) {
					std::cerr << "\e[1;31mFailed to load level [" << (int)e.type() << "]: " <<  e.what() << "\e[0m" << std::endl;
				}catch( Error<ErrorType::ARG_ERROR> e ) {
					std::cerr << "\e[1;31mFailed to load level [" << (int)e.type() << "]: " <<  e.what() << "\e[0m" << std::endl;
				}
			}else {
				std::cerr << "Failed to open level file" << std::endl;
			}
			
		}

		Level& getLevel() {
			return level;
		}
	};

}

/// \todo This all needs to be split into `engine.hpp` and `engine.cpp`