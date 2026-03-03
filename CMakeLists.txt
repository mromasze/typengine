cmake_minimum_required(VERSION 3.10)
project(Typengine)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Include directories
include_directories(include)
include_directories(vendor/duktape)
include_directories(${SDL2_INCLUDE_DIRS})

# Source files grouped by system
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE RENDERER_SOURCES "src/renderer/*.cpp")
file(GLOB_RECURSE INPUT_SOURCES "src/input/*.cpp")
file(GLOB_RECURSE SCRIPTING_SOURCES "src/scripting/*.cpp")
file(GLOB_RECURSE UI_SOURCES "src/ui/*.cpp")

set(DUKTAPE_SOURCE "vendor/duktape/duktape.c")

add_library(Typengine STATIC 
    ${CORE_SOURCES} 
    ${RENDERER_SOURCES} 
    ${INPUT_SOURCES} 
    ${SCRIPTING_SOURCES} 
    ${UI_SOURCES}
    ${DUKTAPE_SOURCE}
)

target_link_libraries(Typengine ${SDL2_LIBRARIES})

target_include_directories(Typengine PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/duktape>
)
