# Engine library
add_library(rtype_engine
    src/ecs/SparseSet.cpp
    src/core/event/EventBus.cpp
    src/plugin_manager/PluginManager.cpp
    src/plugins/graphics/raylib/RaylibGraphicsPlugin.cpp
    src/plugins/audio/miniaudio/MiniaudioPlugin.cpp
    src/plugins/network/asio/AsioNetworkPlugin.cpp
)

target_include_directories(rtype_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find Boost for Asio network plugin
find_package(Boost REQUIRED COMPONENTS system)
target_link_libraries(rtype_engine PUBLIC ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})

# # Find and link raylib
# find_package(raylib QUIET)
# if(NOT raylib_FOUND)
#     include(FetchContent)
#     FetchContent_Declare(
#         raylib
#         GIT_REPOSITORY https://github.com/raysan5/raylib.git
#         GIT_TAG 5.0
#     )
#     FetchContent_MakeAvailable(raylib)
# endif()

# target_link_libraries(rtype_engine
#     PUBLIC
#         raylib
#         ${CMAKE_DL_LIBS}
# )

option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(test_eventbus
        test/core/event/test_eventbus.cpp
    )
    target_link_libraries(test_eventbus
        PRIVATE
            rtype_engine
            GTest::gtest_main
    )
    add_test(NAME EventBusTest COMMAND test_eventbus)

    add_executable(test_sparseset
        test/ecs/test_sparseset.cpp
    )
    target_link_libraries(test_sparseset
        PRIVATE
            rtype_engine
    )
    add_test(NAME SparseSetTest COMMAND test_sparseset)

    add_executable(test_plugin_manager
        test/plugin_manager/test_plugin_manager.cpp
    )
    target_link_libraries(test_plugin_manager
        PRIVATE
            rtype_engine
    )
    add_test(NAME PluginManagerTest COMMAND test_plugin_manager WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    add_executable(test_raylib_plugin
        test/plugins/graphics/raylib/test_raylib_plugin.cpp
    )
    target_link_libraries(test_raylib_plugin
        PRIVATE
            rtype_engine
    )
    add_test(NAME RaylibPluginTest COMMAND test_raylib_plugin WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    # Raylib Graphics Plugin as shared library
    add_library(raylib_graphics SHARED
        src/plugins/graphics/raylib/RaylibGraphicsPlugin.cpp
    )
    target_include_directories(raylib_graphics
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(raylib_graphics
        PRIVATE
            raylib
    )
    set_target_properties(raylib_graphics PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        PREFIX ""
    )

    # Miniaudio Audio Plugin as shared library
    add_library(miniaudio_audio SHARED
        src/plugins/audio/miniaudio/MiniaudioPlugin.cpp
    )
    target_include_directories(miniaudio_audio
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    set_target_properties(miniaudio_audio PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        PREFIX ""
    )

    # Test plugin as shared library
    add_library(test_plugin SHARED
        test/plugin_manager/test_plugin.cpp
    )
    target_link_libraries(test_plugin
        PRIVATE
            rtype_engine
    )
    set_target_properties(test_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugin_manager"
        PREFIX ""
    )

    add_executable(example_plugin_manager
        test/plugin_manager/example_usage.cpp
    )
    target_link_libraries(example_plugin_manager
        PRIVATE
            rtype_engine
    )

    add_executable(test_miniaudio_plugin
        test/plugins/audio/miniaudio/test_miniaudio_plugin.cpp
    )
    target_link_libraries(test_miniaudio_plugin
        PRIVATE
            rtype_engine
    )
    add_test(NAME MiniaudioPluginTest COMMAND test_miniaudio_plugin)

    add_executable(example_miniaudio
        test/plugins/audio/miniaudio/example_usage.cpp
    )
    target_link_libraries(example_miniaudio
        PRIVATE
            rtype_engine
    )
endif()