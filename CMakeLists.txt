cmake_minimum_required(VERSION 3.25)
project(deusex LANGUAGES C CXX)

# Compiler configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING
        "Choose the build type (Debug or Release)"
        FORCE
    )
endif()

# Enable ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# Get the number of available logical cores on the system
include(ProcessorCount)
ProcessorCount(CPU_COUNT)

# Project options
option(SKIP_QA "Skip clang-tidy and clang-format for faster builds" OFF)
option(SKIP_UT "Skip unit tests faster builds" OFF)
set(DEUSEX_GAME_DATA "${CMAKE_SOURCE_DIR}/game-data" CACHE PATH "Path to the original Deus Ex game data")

# Dependencies
set(BUILD_SHARED_LIBS OFF)

# SDL2
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL2-2.32.10 SYSTEM)

if(EMSCRIPTEN)
    add_compile_options(-fwasm-exceptions)
    add_link_options(-fwasm-exceptions)
endif()

# glad
if(EMSCRIPTEN)
    add_library(glad INTERFACE)
    target_include_directories(glad SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/cmake/emscripten)
else()
    add_library(glad STATIC ${CMAKE_SOURCE_DIR}/vendor/glad-gl33/src/gl.c)
    target_include_directories(glad SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glad-gl33/include)
endif()

# miniaudio
add_library(miniaudio STATIC ${CMAKE_SOURCE_DIR}/vendor/miniaudio/miniaudio.c)
target_include_directories(miniaudio SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/vendor/miniaudio)
target_compile_definitions(miniaudio PUBLIC MA_NO_ENCODING MA_NO_GENERATION)
find_package(Threads REQUIRED)
target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS} m)

# Dear ImGui
add_library(imgui STATIC
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/vendor/imgui ${CMAKE_SOURCE_DIR}/vendor/imgui/backends)
target_link_libraries(imgui PUBLIC SDL2::SDL2-static)

# Catch
add_library(Catch2 STATIC ${CMAKE_SOURCE_DIR}/vendor/catch-3.16.0/catch_amalgamated.cpp)
target_include_directories(Catch2 SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/vendor/catch-3.16.0)

# Add targets
add_subdirectory(src)

if(NOT SKIP_UT)
    enable_testing()
    add_subdirectory(tests/unit)
endif()

# Quality assurance
file(
    GLOB_RECURSE ALL_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cc"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/tests/*.cc"
)

find_program(CLANG_FORMAT NAMES clang-format-19 clang-format)
if(CLANG_FORMAT)
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Autoformatting"
    )

    add_custom_target(
        format-check
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking code style"
    )

    if(NOT SKIP_QA)
        add_dependencies(deusex format)
    endif()
endif()
