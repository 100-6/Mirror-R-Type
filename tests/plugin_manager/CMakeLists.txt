# tests/plugin_manager/CMakeLists.txt

# Test for plugin manager
add_executable(test_plugin_manager
    test_plugin_manager.cpp
)

target_include_directories(test_plugin_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
)

target_link_libraries(test_plugin_manager PRIVATE
    plugin_manager
    ${CMAKE_DL_LIBS}
)

set_target_properties(test_plugin_manager PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

# Test plugin for plugin manager testing
add_library(test_plugin SHARED
    test_plugin.cpp
)

target_include_directories(test_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
)

set_target_properties(test_plugin PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/plugins"
)

# Make sure test_plugin is built before test_plugin_manager
add_dependencies(test_plugin_manager test_plugin)

