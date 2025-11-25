# tests/graphics_plugins/CMakeLists.txt

option(BUILD_GRAPHICS_PLUGIN_TESTS "Build test executables for graphics plugins" ON)

# Only build tests if raylib_graphics target exists
if(BUILD_GRAPHICS_PLUGIN_TESTS AND TARGET raylib_graphics)
    # Full test with window (requires X11/display)
    add_executable(test_raylib_plugin
        test_raylib_plugin.cpp
    )

    target_include_directories(test_raylib_plugin PRIVATE
        ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
    )

    target_link_libraries(test_raylib_plugin PRIVATE
        plugin_manager
        ${CMAKE_DL_LIBS}
    )

    set_target_properties(test_raylib_plugin PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    # Make sure the plugin is built before the test
    add_dependencies(test_raylib_plugin raylib_graphics)

    # Minimal test (no window required)
    add_executable(test_minimal_raylib
        test_minimal.cpp
    )

    target_include_directories(test_minimal_raylib PRIVATE
        ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
    )

    target_link_libraries(test_minimal_raylib PRIVATE
        plugin_manager
        ${CMAKE_DL_LIBS}
    )

    set_target_properties(test_minimal_raylib PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_dependencies(test_minimal_raylib raylib_graphics)
else()
    message(WARNING "Skipping raylib graphics tests (raylib_graphics target not available)")
endif()

