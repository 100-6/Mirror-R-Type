cmake_minimum_required(VERSION 3.20)

enable_testing()

# Find GTest via vcpkg
find_package(GTest QUIET)
if(GTest_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
else()
    message(WARNING "GTest not found. Skipping GTest-based tests.")
    message(WARNING "Install via vcpkg: vcpkg install gtest")
endif()

# Include engine headers
include_directories(${CMAKE_SOURCE_DIR}/src/engine/include)

if(GTest_FOUND)
    # Test EventBus
    add_executable(test_eventbus
        core/event/test_eventbus.cpp
    )
    target_link_libraries(test_eventbus
        PRIVATE
            rtype_engine
            ${GTEST_LIBRARIES}
            ${GTEST_MAIN_LIBRARIES}
            pthread
    )
    add_test(NAME EventBusTest COMMAND test_eventbus)
    set_property(TARGET test_eventbus PROPERTY CXX_STANDARD 20)
endif()

# Test SparseSet (no GTest dependency)
add_executable(test_sparseset
    ecs/test_sparseset.cpp
)
target_link_libraries(test_sparseset
    PRIVATE
        rtype_engine
)
add_test(NAME SparseSetTest COMMAND test_sparseset)
set_property(TARGET test_sparseset PROPERTY CXX_STANDARD 20)

# Test Plugin Manager
add_executable(test_plugin_manager
    plugin_manager/test_plugin_manager.cpp
)
target_link_libraries(test_plugin_manager
    PRIVATE
        rtype_engine
)
add_test(NAME PluginManagerTest COMMAND test_plugin_manager WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TARGET test_plugin_manager PROPERTY CXX_STANDARD 20)

# Test plugin as shared library
add_library(test_plugin SHARED
    plugin_manager/test_plugin.cpp
)
target_link_libraries(test_plugin
    PRIVATE
        rtype_engine
)
set_target_properties(test_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugin_manager"
    PREFIX ""
    CXX_STANDARD 20
)

# Example Plugin Manager
add_executable(example_plugin_manager
    plugin_manager/example_usage.cpp
)
target_link_libraries(example_plugin_manager
    PRIVATE
        rtype_engine
)
set_property(TARGET example_plugin_manager PROPERTY CXX_STANDARD 20)

# Test Miniaudio Plugin
add_executable(test_miniaudio_plugin
    plugins/audio/miniaudio/test_miniaudio_plugin.cpp
)
target_link_libraries(test_miniaudio_plugin
    PRIVATE
        rtype_engine
)
add_test(NAME MiniaudioPluginTest COMMAND test_miniaudio_plugin)
set_property(TARGET test_miniaudio_plugin PROPERTY CXX_STANDARD 20)

# Example Miniaudio
add_executable(example_miniaudio
    plugins/audio/miniaudio/example_usage.cpp
)
target_link_libraries(example_miniaudio
    PRIVATE
        rtype_engine
)
set_property(TARGET example_miniaudio PROPERTY CXX_STANDARD 20)

# Test Asio Network Plugin
add_executable(test_asio_network_plugin
    plugins/network/asio/test_asio_network_plugin.cpp
)
target_link_libraries(test_asio_network_plugin
    PRIVATE
        rtype_engine
)
add_test(NAME AsioNetworkPluginTest COMMAND test_asio_network_plugin)
set_property(TARGET test_asio_network_plugin PROPERTY CXX_STANDARD 20)

# Test Raylib Plugin
add_executable(test_raylib_plugin
    ${CMAKE_SOURCE_DIR}/src/engine/test/plugins/graphics/raylib/test_raylib_plugin.cpp
)
target_link_libraries(test_raylib_plugin
    PRIVATE
        rtype_engine
)
add_test(NAME RaylibPluginTest COMMAND test_raylib_plugin WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TARGET test_raylib_plugin PROPERTY CXX_STANDARD 20)
