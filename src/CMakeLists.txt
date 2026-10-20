set(DEUSEX_WARNINGS
  -Wall -Wextra -Wpedantic -Walloca -Wcast-align -Wcast-qual
  -Wconversion -Wdouble-promotion -Werror -Wformat=2
  -Wimplicit-fallthrough -Winit-self -Wnon-virtual-dtor
  -Wold-style-cast -Woverloaded-virtual
  -Wredundant-decls -Wshadow -Wsign-conversion -Wsuggest-override
  -Wswitch-enum -Wundef -Wuninitialized -Wunused
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
  list(APPEND DEUSEX_WARNINGS -Wno-restrict)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  list(APPEND DEUSEX_WARNINGS -Wno-double-promotion -Wno-unneeded-internal-declaration -Wno-unused-but-set-variable)
endif()

# Sanitizers
set(SANITIZER "none" CACHE STRING "Sanitizer to build with: none, asan, tsan")
set_property(CACHE SANITIZER PROPERTY STRINGS none asan tsan)

if(SANITIZER STREQUAL "asan")
  set(SANITIZER_FLAGS -fsanitize=address,undefined -fno-sanitize=vptr)
elseif(SANITIZER STREQUAL "tsan")
  set(SANITIZER_FLAGS -fsanitize=thread,undefined -fno-sanitize=vptr)
elseif(SANITIZER STREQUAL "none")
  set(SANITIZER_FLAGS "")
else()
  message(FATAL_ERROR "SANITIZER must be one of: none, asan, tsan (got '${SANITIZER}')")
endif()

file(GLOB DX_ENGINE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/audio/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/extension/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/deusex/*.cc
)
add_library(dx-engine STATIC ${DX_ENGINE_SOURCES})
target_include_directories(dx-engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dx-engine PUBLIC miniaudio SDL2::SDL2-static)

file(GLOB DX_RENDER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/render/*.cc)
add_library(dx-render STATIC ${DX_RENDER_SOURCES})
target_include_directories(dx-render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dx-render PUBLIC dx-engine glad)

file(GLOB DX_GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/game/*.cc)
add_executable(deusex ${DX_GAME_SOURCES})
target_link_libraries(deusex PRIVATE dx-render SDL2::SDL2-static SDL2::SDL2main)
if(EMSCRIPTEN)
  set(DX_WEB_EXCLUDE *.dll *.exe IpDrv.* IpServer.* D3DDrv.int Galaxy.int GlideDrv.int MeTaLDrv.int
    OpenGlDrv.int OpenGlDrv.ini SGLDrv.int SoftDrv.int WinDrv.int Window.int Setup.int Startup.int)
  list(TRANSFORM DX_WEB_EXCLUDE PREPEND "SHELL:--exclude-file */")
  target_compile_definitions(deusex PRIVATE DEUSEX_GAME_DATA="/game")
  set_target_properties(deusex PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
  set(DX_WEB_SHELL ${CMAKE_SOURCE_DIR}/cmake/emscripten/shell.html)
  set_property(TARGET deusex APPEND PROPERTY LINK_DEPENDS ${DX_WEB_SHELL})
  target_link_options(deusex PRIVATE
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
    -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=256MB -sMAXIMUM_MEMORY=4GB -sSTACK_SIZE=16MB
    -sEXPORTED_RUNTIME_METHODS=FS -lidbfs.js
    -sLZ4=1 --use-preload-cache
    --shell-file ${DX_WEB_SHELL}
    "SHELL:--preload-file ${DEUSEX_GAME_DATA}/System@/game/System"
    ${DX_WEB_EXCLUDE}
    "SHELL:--preload-file ${DEUSEX_GAME_DATA}/Maps@/game/Maps"
    "SHELL:--preload-file ${DEUSEX_GAME_DATA}/Music@/game/Music"
    "SHELL:--preload-file ${DEUSEX_GAME_DATA}/Sounds@/game/Sounds"
    "SHELL:--preload-file ${DEUSEX_GAME_DATA}/Textures@/game/Textures"
  )
else()
  target_compile_definitions(deusex PRIVATE DEUSEX_GAME_DATA="${DEUSEX_GAME_DATA}")
endif()

set(DX_TARGETS dx-engine dx-render deusex)

file(GLOB DX_COMPILER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/compiler/*.cc)
add_library(dx-compiler STATIC ${DX_COMPILER_SOURCES})
target_include_directories(dx-compiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dx-compiler PUBLIC dx-engine)
list(APPEND DX_TARGETS dx-compiler)

if(NOT EMSCRIPTEN)
  file(GLOB DX_UCC_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ucc/*.cc)
  add_executable(ucc ${DX_UCC_SOURCES})
  target_link_libraries(ucc PRIVATE dx-compiler SDL2::SDL2-static)
  target_compile_definitions(ucc PRIVATE DEUSEX_GAME_DATA="${DEUSEX_GAME_DATA}")
  target_link_options(ucc PRIVATE -static-libstdc++ -static-libgcc)
  list(APPEND DX_TARGETS ucc)
endif()

file(GLOB DX_EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/editor/*.cc)
if(DX_EDITOR_SOURCES)
  add_executable(deused ${DX_EDITOR_SOURCES})
  target_link_libraries(deused PRIVATE dx-render imgui SDL2::SDL2-static SDL2::SDL2main)
  target_compile_definitions(deused PRIVATE DEUSEX_GAME_DATA="${DEUSEX_GAME_DATA}")
  target_link_options(deused PRIVATE -static-libstdc++ -static-libgcc)
  list(APPEND DX_TARGETS deused)
endif()

foreach(tgt ${DX_TARGETS})
  target_compile_options(${tgt} PRIVATE ${DEUSEX_WARNINGS})

  if(SANITIZER_FLAGS)
    target_compile_options(${tgt} PRIVATE ${SANITIZER_FLAGS} -fno-omit-frame-pointer -g)
    target_link_options(${tgt} PRIVATE ${SANITIZER_FLAGS})
  endif()

  target_compile_options(${tgt} PRIVATE
    $<$<CONFIG:Debug>:-D_GLIBCXX_ASSERTIONS>
  )
endforeach()

if(NOT EMSCRIPTEN)
  target_link_options(deusex PRIVATE -static-libstdc++ -static-libgcc)
endif()
