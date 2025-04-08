cmake_minimum_required(VERSION 3.16)
project(Gunslinger LANGUAGES CXX)

set( CMAKE_CXX_STANDARD, 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID}")

set(DEBUG ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(FetchContent)
FetchContent_Declare(SFML
	GIT_REPOSITORY https://github.com/SFML/SFML.git
	GIT_TAG 3.0.x
)
FetchContent_Declare(ImGUI
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG master
)
FetchContent_Declare(ImGUI_SFML
	GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
	GIT_TAG master
)
FetchContent_Declare(Box2D
	GIT_REPOSITORY https://github.com/erincatto/box2d.git
	GIT_TAG main
)

FetchContent_MakeAvailable(SFML)
FetchContent_MakeAvailable(ImGUI)

set(IMGUI_SFML_FIND_SFML OFF)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
FetchContent_MakeAvailable(ImGUI_SFML)
FetchContent_MakeAvailable(Box2D)
if(DEFINED DEBUG)
	add_compile_options(-g -fstandalone-debug)
	add_link_options(-g)
endif()

add_executable(Gunslinger src/main.cpp src/level.cpp src/localizer.cpp src/phys_object.cpp src/entity.cpp)
target_link_libraries(Gunslinger PUBLIC
	SFML::Graphics
	ImGui-SFML::ImGui-SFML
	box2d
)
target_compile_features(Gunslinger PRIVATE cxx_std_23)