file(GLOB UNIT_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
add_executable(unit-tests ${UNIT_TEST_SOURCES} ${CMAKE_SOURCE_DIR}/src/editor/script_workspace.cc ${CMAKE_SOURCE_DIR}/src/editor/log_capture.cc)
target_link_libraries(unit-tests PRIVATE dx-render dx-compiler Catch2 SDL2::SDL2-static)
target_compile_definitions(unit-tests PRIVATE DEUSEX_GAME_DATA="${DEUSEX_GAME_DATA}" UCC_PATH="$<TARGET_FILE:ucc>")
add_dependencies(unit-tests ucc)
if(EXISTS "${DEUSEX_GAME_DATA}/System/Core.u")
    add_test(NAME unit-tests COMMAND unit-tests)
    add_test(NAME editor-scripts COMMAND unit-tests "[editor-scripts]")
    if(TARGET deused)
        function(add_editor_test name expected)
            add_test(NAME editor-${name} COMMAND deused --new EditorTest --width 1280 --height 800 ${ARGN})
            set_tests_properties(editor-${name} PROPERTIES ENVIRONMENT SDL_VIDEODRIVER=offscreen PASS_REGULAR_EXPRESSION "expectations: ${expected} passed, 0 failed")
        endfunction()
        add_editor_test(face-extrude 5 --frames 30
            --at "3:exec camera -600 0 0 0 0" --at "4:exec editmode face" --at "6:clickview perspective 0.5 0.5" --at "10:expect status selected 1 face"
            --at "11:exec extrude 128" --at "12:expect vertex -256 128 128" --at "13:expect novertex -128 128 128"
            --at "14:exec undo" --at "15:expect vertex -128 128 128" --at "16:expect status undo done")
        add_editor_test(extrude-drag-snaps 4 --frames 40
            --at "2:exec grid 16 1" --at "3:exec camera -600 -350 0 0 7100" --at "4:exec editmode face" --at "6:clickview perspective 0.5 0.5" --at "9:expect status selected 1 face"
            --at "10:exec gizmo extrude" --at "14:dragview perspective 0.5 0.5 0.4 0.5" --at "28:expect status extruded 1 face" --at "29:expect novertex -128 128 128" --at "30:expect grid 16")
        add_editor_test(vertex-drag-snaps-absolute 5 --frames 50
            --at "2:exec grid 16 0" --at "3:exec camera -628 -528 528 -5825 7038" --at "4:exec editmode vertex" --at "6:clickview perspective 0.5 0.5" --at "9:expect status selected 1 vertex"
            --at "10:dragview perspective 0.5 0.5 0.6 0.45" --at "24:expect offgrid 16" --at "25:expect novertex -128 -128 128"
            --at "26:exec grid 16 1" --at "28:dragview perspective 0.6 0.45 0.7 0.5" --at "42:expect grid 16" --at "43:expect status edited 1 brush vertices")
        add_editor_test(overlapping-handles-cycle 3 --frames 30
            --at "3:exec camera 128 0 400 -5825 7038" --at "4:exec zoom 0.5" --at "5:exec editmode edge"
            --at "6:clickview top 0.5 0.5" --at "10:expect status selected 1 edge" --at "12:clickview top 0.5 0.5" --at "16:expect status cycled to 2 of 2 overlapping"
            --at "18:clickview top 0.5 0.5" --at "22:expect status cycled to 1 of 2 overlapping")
        add_editor_test(escape-ignored-while-popup-open 2 --frames 40
            --at "4:exec editmode vertex" --at "6:rclick 400 200" --at "10:key Escape" --at "14:expect status Vertex mode"
            --at "16:click 600 390" --at "20:key Escape" --at "24:expect status Object mode")
        add_editor_test(scale-gizmo-scales-every-selected-brush 2 --frames 40
            --at "2:exec csgadd" --at "4:exec moveto 512 0 0" --at "5:exec csgadd" --at "6:exec select Brush1" --at "7:exec selectadd Brush2" --at "8:expect status selected 2 actors"
            --at "9:exec camera -500 -400 400 -5825 7038" --at "10:exec gizmo scale" --at "12:dragview perspective 0.5 0.5 0.62 0.45" --at "26:expect mainscale 1.05")
    endif()
else()
    message(STATUS "No game data in ${DEUSEX_GAME_DATA}, tests tagged [data] are skipped")
    add_test(NAME unit-tests COMMAND unit-tests "~[data]")
endif()
