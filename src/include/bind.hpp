#pragma once

#include "SFML/Window/Mouse.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <unordered_map>

namespace Arcade {

	class Bind {
		friend class BindManager;

		int code;
		bool is_mouse;

		bool shift{};
		bool ctrl{};
		bool alt{};
		bool meta{};

		bool state = false;
		bool prev_state = false;

		sf::Clock timer;

		void eval( std::optional<sf::Event> evt ) {
			if( is_mouse ) {
				if( const auto* mpe = evt->getIf<sf::Event::MouseButtonPressed>() ) {
					if( (int)(mpe->button) == this->code ) {
						state = true;
						timer.start();
					}
				}else if( const auto* mre = evt->getIf<sf::Event::MouseButtonReleased>() ) {
					if( (int)(mre->button) == this->code ) {
						state = false;
						timer.reset();
					}
				}
			}else {
				if( const auto* kpe = evt->getIf<sf::Event::KeyPressed>() ) {
					if( (int)(kpe->code) == this->code && 
						( !shift || kpe->shift ) &&
						( !ctrl || kpe->control ) &&
						( !alt || kpe->alt ) &&
						( !meta || kpe->system ) ) {
						state = true;
						timer.start();
					}
				}else if( const auto* kre = evt->getIf<sf::Event::KeyReleased>() ) {
					if( (int)(kre->code) == this->code && 
						( !shift || kre->shift ) &&
						( !ctrl || kre->control ) &&
						( !alt || kre->alt ) &&
						( !meta || kre->system ) ) {
						state = false;
						timer.reset();
					}
				}
			}
		}
	public:

		Bind( sf::Mouse::Button btn ) : is_mouse(true), code((int)btn) { timer.reset(); }
		Bind( sf::Keyboard::Key key ) : is_mouse(false), code((int)key) { timer.reset(); }

		bool isPressed() {
			return this->state;
		}
		bool startedPressing() {
			return this->state && !this->prev_state;
		}
		bool endedPressing() {
			return !this->state && this->prev_state;
		}
		sf::Time getTimePressed() {
			return this->timer.getElapsedTime();
		}

		int getCode() {
			return code;
		}
		bool isMouse() {
			return is_mouse;
		}

		bool isShiftModified() {
			return shift;
		}
		bool isCtrlModified() {
			return ctrl;
		}
		bool isAltModified() {
			return alt;
		}
		bool isMetaModified() {
			return meta;
		}
		void setModifiers( bool s, bool c, bool a, bool m ) {
			shift = s; ctrl = c; alt = a; meta = m;
		}

		void changeKey( sf::Keyboard::Key key ) {
			code = (int)key;
			is_mouse = false;
		}
		void changeKey( sf::Mouse::Button btn ) {
			code = (int)btn;
			is_mouse = true;
		}
	};

	class BindManager {
		friend class Engine;

		std::unordered_map<std::string, Bind> binds;

		void pre_update() {
			for( auto& pair : binds ) {
				pair.second.prev_state = pair.second.state;
			}
		}
		void update( std::optional<sf::Event> e ) {
			for( auto& pair : binds ) {
				pair.second.eval( e );
			}
		}
	public:
		BindManager() {}

		Bind& bind( std::string name, sf::Keyboard::Key key ) noexcept {
			return (*binds.emplace( name, key ).first).second;
		}
		bool bind( std::string name, sf::Mouse::Button btn ) noexcept {
			if( binds.find( name ) == binds.end() ) {
				binds.emplace( name, btn );
				return true;
			}
			return false;
		}
		bool unbind( std::string name ) {
			if( binds.find( name ) != binds.end() ) {
				binds.erase( name );
			}
			return false;
		}
		std::unordered_map<std::string, Bind> getAllBinds() {
			return this->binds;
		}

		const Bind* getBind( std::string name ) noexcept {
			if( binds.find( name ) != binds.end() ) {
				return &binds.at( name );
			}
			return nullptr;
		}

		[[nodiscard]] bool isPressed( std::string name ) noexcept {
			if( binds.find(name) != binds.end() ) {
				return binds.at( name ).isPressed();
			}
			return false;
		}
		[[nodiscard]] bool startedPressing( std::string name ) noexcept {
			if( binds.find(name) != binds.end() ) {
				return binds.at( name ).startedPressing();
			}
			return false;
		}
		[[nodiscard]] bool endedPressing( std::string name ) noexcept {
			if( binds.find(name) != binds.end() ) {
				return binds.at( name ).endedPressing();
			}
			return false;
		}
		[[nodiscard]] sf::Time getTimePressed( std::string name ) noexcept {
			if( binds.find(name) != binds.end() ) {
				return binds.at( name ).getTimePressed();
			}
			return sf::Time::Zero;
		}
	};
}