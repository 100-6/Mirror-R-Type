cmake_minimum_required(VERSION 3.20)

enable_testing()

# Find Threads (cross-platform pthread/native threads)
find_package(Threads REQUIRED)

# Find GTest via vcpkg (modern CMake target style)
find_package(GTest CONFIG QUIET)
if(GTest_FOUND)
    message(STATUS "GTest found, enabling tests")
else()
    message(WARNING "GTest not found. Skipping GTest-based tests.")
    message(WARNING "Install via vcpkg: vcpkg install gtest")
endif()

# Include engine headers
include_directories(${CMAKE_SOURCE_DIR}/src/engine/include)
include_directories(${CMAKE_SOURCE_DIR}/src/r-type/game-logic/include)

if(GTest_FOUND)
    # Test EventBus
    add_executable(test_eventbus
        core/event/test_eventbus.cpp
    )
    target_link_libraries(test_eventbus
        PRIVATE
            game_engine
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME EventBusTest COMMAND test_eventbus)
    set_property(TARGET test_eventbus PROPERTY CXX_STANDARD 20)
endif()

if(GTest_FOUND)
    # Test SparseSet with GTest
    add_executable(test_sparseset_gtest
        ecs/test_sparseset_gtest.cpp
    )
    target_link_libraries(test_sparseset_gtest
        PRIVATE
            game_engine
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME SparseSetGTestSuite COMMAND test_sparseset_gtest)
    set_property(TARGET test_sparseset_gtest PROPERTY CXX_STANDARD 20)

    # Test Registry with GTest
    add_executable(test_registry
        ecs/test_registry.cpp
    )
    target_link_libraries(test_registry
        PRIVATE
            game_engine
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME RegistryGTestSuite COMMAND test_registry)
    set_property(TARGET test_registry PROPERTY CXX_STANDARD 20)

    # Test CollisionSystem with GTest
    add_executable(test_collision_system
        ecs/test_collision_system.cpp
    )
    target_link_libraries(test_collision_system
        PRIVATE
            game_engine
            rtype_logic
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME CollisionSystemGTestSuite COMMAND test_collision_system)
    set_property(TARGET test_collision_system PROPERTY CXX_STANDARD 20)

    # Test MovementSystem with GTest
    add_executable(test_movement_system
        ecs/test_movement_system.cpp
    )
    target_link_libraries(test_movement_system
        PRIVATE
            game_engine
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME MovementSystemGTestSuite COMMAND test_movement_system)
    set_property(TARGET test_movement_system PROPERTY CXX_STANDARD 20)

    # Test PhysiqueSystem with GTest
    add_executable(test_physique_system
        ecs/test_physique_system.cpp
    )
    target_link_libraries(test_physique_system
        PRIVATE
            game_engine
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME PhysiqueSystemGTestSuite COMMAND test_physique_system)
    set_property(TARGET test_physique_system PROPERTY CXX_STANDARD 20)

    # Test DestroySystem with GTest
    add_executable(test_destroy_system
        ecs/test_destroy_system.cpp
    )
    target_link_libraries(test_destroy_system
        PRIVATE
            game_engine
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME DestroySystemGTestSuite COMMAND test_destroy_system)
    set_property(TARGET test_destroy_system PROPERTY CXX_STANDARD 20)

    # Test ShootingSystem with GTest
    add_executable(test_shooting_system
        ecs/test_shooting_system.cpp
    )
    target_link_libraries(test_shooting_system
        PRIVATE
            game_engine
            rtype_logic
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME ShootingSystemGTestSuite COMMAND test_shooting_system)
    set_property(TARGET test_shooting_system PROPERTY CXX_STANDARD 20)
endif()

# Test Plugin Manager
add_executable(test_plugin_manager
    plugin_manager/test_plugin_manager.cpp
)
target_link_libraries(test_plugin_manager
    PRIVATE
        game_engine
)
add_test(NAME PluginManagerTest COMMAND test_plugin_manager WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TARGET test_plugin_manager PROPERTY CXX_STANDARD 20)

# Test plugin as shared library
add_library(test_plugin SHARED
    plugin_manager/test_plugin.cpp
)
target_link_libraries(test_plugin
    PRIVATE
        game_engine
)
set_target_properties(test_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugin_manager"
    PREFIX ""
    CXX_STANDARD 20
)

# Example Plugin Manager - DISABLED (outdated)
# add_executable(example_plugin_manager
#     plugin_manager/example_usage.cpp
# )
# target_link_libraries(example_plugin_manager
#     PRIVATE
#         game_engine
# )
# set_property(TARGET example_plugin_manager PROPERTY CXX_STANDARD 20)

# Test Miniaudio Plugin
add_executable(test_miniaudio_plugin
    plugins/audio/miniaudio/test_miniaudio_plugin.cpp
)
target_link_libraries(test_miniaudio_plugin
    PRIVATE
        game_engine
)
add_test(NAME MiniaudioPluginTest COMMAND test_miniaudio_plugin)
set_property(TARGET test_miniaudio_plugin PROPERTY CXX_STANDARD 20)

# Example Miniaudio
add_executable(example_miniaudio
    plugins/audio/miniaudio/example_usage.cpp
)
target_link_libraries(example_miniaudio
    PRIVATE
        game_engine
)
set_property(TARGET example_miniaudio PROPERTY CXX_STANDARD 20)

# Test Asio Network Plugin - DISABLED (outdated API)
# add_executable(test_asio_network_plugin
#     plugins/network/asio/test_asio_network_plugin.cpp
# )
# target_link_libraries(test_asio_network_plugin
#     PRIVATE
#         game_engine
# )
# add_test(NAME AsioNetworkPluginTest COMMAND test_asio_network_plugin)
# set_property(TARGET test_asio_network_plugin PROPERTY CXX_STANDARD 20)

# Test Raylib Graphics Plugin
add_executable(test_raylib_plugin
    plugins/graphics/raylib/test_raylib_plugin.cpp
)
target_link_libraries(test_raylib_plugin
    PRIVATE
        game_engine
)
add_test(NAME RaylibPluginTest COMMAND test_raylib_plugin WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TARGET test_raylib_plugin PROPERTY CXX_STANDARD 20)

# Test SFML Graphics Plugin (only if SFML plugins are built)
if(TARGET sfml_graphics)
    add_executable(test_sfml_plugin
        plugins/graphics/sfml/test_sfml_plugin.cpp
    )
    target_link_libraries(test_sfml_plugin
        PRIVATE
            game_engine
    )
    add_test(NAME SFMLPluginTest COMMAND test_sfml_plugin WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_property(TARGET test_sfml_plugin PROPERTY CXX_STANDARD 20)
endif()


if(GTest_FOUND)
    # Test Collision Simple (GTest)
    add_executable(test_collision_simple_gtest
        ecs/test_collision_simple_gtest.cpp
    )
    target_link_libraries(test_collision_simple_gtest
        PRIVATE
            game_engine
            rtype_logic
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME CollisionSimpleTest COMMAND test_collision_simple_gtest)
    set_property(TARGET test_collision_simple_gtest PROPERTY CXX_STANDARD 20)

    # Test Input Plugin (GTest) - DISABLED (outdated Input structure)
    # add_executable(test_input_plugin_gtest
    #     ecs/test_input_plugin_gtest.cpp
    # )
    # target_link_libraries(test_input_plugin_gtest
    #     PRIVATE
    #         game_engine
    #         GTest::gtest
    #         GTest::gtest_main
    #         Threads::Threads
    # )
    # add_test(NAME InputPluginTest COMMAND test_input_plugin_gtest)
    # set_property(TARGET test_input_plugin_gtest PROPERTY CXX_STANDARD 20)

    # Test Shooting Collision (GTest)
    add_executable(test_shooting_collision_gtest
        ecs/test_shooting_collision_gtest.cpp
    )
    target_link_libraries(test_shooting_collision_gtest
        PRIVATE
            game_engine
            rtype_logic
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME ShootingCollisionTest COMMAND test_shooting_collision_gtest)
    set_property(TARGET test_shooting_collision_gtest PROPERTY CXX_STANDARD 20)

    # Test Render System (GTest)
    add_executable(test_render_system
        ecs/test_render_system.cpp
    )
    target_link_libraries(test_render_system
        PRIVATE
            game_engine
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
    )
    add_test(NAME RenderSystemTest COMMAND test_render_system)
    set_property(TARGET test_render_system PROPERTY CXX_STANDARD 20)

    # Test Protocol (GTest) - DISABLED (outdated protocol structure)
    # add_executable(test_protocol
    #     protocol/protocol_test.cpp
    # )
    # target_include_directories(test_protocol
    #     PRIVATE
    #         ${CMAKE_SOURCE_DIR}/src
    # )
    # target_link_libraries(test_protocol
    #     PRIVATE
    #         GTest::gtest
    #         GTest::gtest_main
    #         Threads::Threads
    # )
    # add_test(NAME ProtocolTest COMMAND test_protocol)
    # set_property(TARGET test_protocol PROPERTY CXX_STANDARD 20)
endif()
