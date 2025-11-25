cmake_minimum_required(VERSION 3.20)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest QUIET)
if(GTest_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
else()
    message(WARNING "GTest not found. Skipping GTest-based tests.")
endif()

# Include engine headers
include_directories(${CMAKE_SOURCE_DIR}/engine/include)

# Plugin Manager Tests
add_subdirectory(plugin_manager)

# Graphics Plugins Tests
add_subdirectory(graphics_plugins)

# Audio Plugins Tests
add_subdirectory(audio_plugins)

# EventBus tests (only if GTest is found)
if(GTest_FOUND)
    add_executable(test_eventbus
        core/event/test_eventbus.cpp
    )

    target_link_libraries(test_eventbus
        rtype_engine
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
        pthread
    )

    # Add test to CTest
    add_test(NAME EventBusTests COMMAND test_eventbus)

    # Set C++ standard
    set_property(TARGET test_eventbus PROPERTY CXX_STANDARD 20)
endif()