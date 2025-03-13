#pragma once

#include "SFML/Window/VideoMode.hpp"
#include "SFML/Window/WindowEnums.hpp"
#include "bind.hpp"
#include "level.hpp"
#include "localizer.hpp"
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
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
		std::string window_name;
		sf::VideoMode window_mode;
		sf::RenderWindow window;

		Level level;

		bool fullscreen = false;

		static sf::Clock g_time;

		std::unordered_map<std::string, std::unique_ptr<VEventWrapper>> events;

		bool debug_open{};
		void debug_imgui() {
			ImGui::Begin("Event Registries");
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
					ImGui::BeginTable("TranslationRegistryTable", 2, flags);
					ImGui::TableSetupColumn("Key");
					ImGui::TableSetupColumn("Translation");
					ImGui::TableHeadersRow();
					for( auto& pair : Localizer::getAllKeys() ) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text( "'%s'", pair.first.c_str() );
						ImGui::TableNextColumn();
						ImGui::Text( "%s", pair.second.c_str() );
					}
					ImGui::EndTable();
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

	public:

		BindManager bindManager;

		Engine( std::string window_title ) : window_name( window_title ) {
			window_mode.size = window_mode.getDesktopMode().size.componentWiseDiv({2, 2});
			window.create( window_mode, window_title ); 

			if( !ImGui::SFML::Init( window ) ) {
				throw std::runtime_error("Failed to init ImGui!");
			};

			init_default_events();

		}
		Engine( std::string window_title, sf::VideoMode dimensions ) : window_name( window_title ), window_mode( dimensions ), window( dimensions, window_title ) {

			if( !ImGui::SFML::Init( window ) ) {
				throw std::runtime_error("Failed to init ImGui!");
			};

			init_default_events();
		}
		~Engine() {
			close();
		}

		const sf::RenderWindow& getWindow() {
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
			ImGui::SFML::Update( window, delta );
		}

		void render() {
			window.clear();

			if( bindManager.startedPressing("Meta:Debug") ) {
				debug_open = !debug_open;
			}
			if( debug_open ) {
				this->debug_imgui();
			}

			ImGui::SFML::Render( window );
			window.display();
		}

		void close() {
			window.close();
			ImGui::SFML::Shutdown();
		}

		void loadLevel() {
			
		}
	};

}

/// \todo This all needs to be split into `engine.hpp` and `engine.cpp`