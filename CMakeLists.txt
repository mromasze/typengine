cmake_minimum_required(VERSION 3.16)
project(typengine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TYPENGINE_BUILD_EXAMPLES "Build typengine examples" ${PROJECT_IS_TOP_LEVEL})

include(FetchContent)

# --- SDL2 (zlib license) ------------------------------------------------------
# Prefer an installed SDL2; fall back to building it from source.
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    message(STATUS "typengine: SDL2 not found, fetching from source")
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.11
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL2)
endif()

if(TARGET SDL2::SDL2)
    set(TYPENGINE_SDL2_TARGET SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(TYPENGINE_SDL2_TARGET SDL2::SDL2-static)
else()
    message(FATAL_ERROR "typengine: no usable SDL2 target found")
endif()

# --- EnTT (MIT license) --------------------------------------------------------
find_package(EnTT CONFIG QUIET)
if(NOT EnTT_FOUND)
    message(STATUS "typengine: EnTT not found, fetching from source")
    FetchContent_Declare(
        EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.2
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(EnTT)
endif()

# --- engine library -----------------------------------------------------------
add_library(typengine STATIC
    src/Engine.cpp
    src/Renderer.cpp
    src/Input.cpp
    src/IsoTilemap.cpp
    src/ECS.cpp
    src/JobSystem.cpp
    src/Skill.cpp
    src/UI.cpp
)
add_library(typengine::typengine ALIAS typengine)

target_include_directories(typengine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
)

# SDL2 stays PRIVATE: no SDL types appear in public headers.
# EnTT is PUBLIC: engine/ECS.hpp includes <entt/entt.hpp>.
target_link_libraries(typengine
    PRIVATE ${TYPENGINE_SDL2_TARGET}
    PUBLIC EnTT::EnTT
)

# Game code defines a plain main(); no SDL_main entry-point rewriting.
target_compile_definitions(typengine PRIVATE SDL_MAIN_HANDLED)

# --- optional Lua scripting module (Lua 5.4 MIT + sol2 MIT) ---------------------
option(TYPENGINE_SCRIPTING "Enable the Lua scripting module (ScriptHost)" ON)
if(TYPENGINE_SCRIPTING)
    # Lua has no upstream CMake — fetch the sources and build a static lib.
    FetchContent_Declare(
        lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG v5.4.7
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(lua)
    file(GLOB LUA_SOURCES ${lua_SOURCE_DIR}/*.c)
    list(FILTER LUA_SOURCES EXCLUDE REGEX "(lua|luac|onelua|ltests)\\.c$")
    add_library(lua_static STATIC ${LUA_SOURCES})
    target_include_directories(lua_static PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)

    find_package(sol2 CONFIG QUIET)
    if(NOT sol2_FOUND)
        message(STATUS "typengine: sol2 not found, fetching from source")
        FetchContent_Declare(
            sol2
            GIT_REPOSITORY https://github.com/ThePhD/sol2.git
            GIT_TAG v3.3.0
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(sol2)
    endif()

    target_sources(typengine PRIVATE src/Script.cpp)
    target_link_libraries(typengine PUBLIC sol2::sol2 lua_static)
    target_compile_definitions(typengine PUBLIC TYPENGINE_HAS_SCRIPTING SOL_ALL_SAFETIES_ON=1)
endif()

if(WIN32)
    # SDL2 static needs these on MinGW/MSVC when linked into the final exe.
    target_link_libraries(typengine PRIVATE winmm imm32 version setupapi)
endif()

# --- examples ------------------------------------------------------------------
if(TYPENGINE_BUILD_EXAMPLES)
    add_subdirectory(examples/hello-world)
    if(TYPENGINE_SCRIPTING)
        add_subdirectory(examples/scripting)
    endif()
endif()

# --- install (optional: use as an installed package instead of a subdirectory) --
# Only possible against a system-installed SDL2 — a FetchContent-built SDL2
# static target cannot be referenced from an export set.
option(TYPENGINE_INSTALL "Generate install rules (requires system SDL2)" ${SDL2_FOUND})
if(TYPENGINE_INSTALL AND SDL2_FOUND)
    include(GNUInstallDirs)
    install(TARGETS typengine
        EXPORT typengine-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(DIRECTORY include/engine DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT typengine-targets
        NAMESPACE typengine::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/typengine
    )
endif()
