cmake_minimum_required(VERSION 3.20)

enable_testing()

find_package(GTest QUIET)
if(GTest_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
else()
    message(WARNING "GTest not found. Skipping GTest-based tests.")
endif()

include_directories(${CMAKE_SOURCE_DIR}/engine/include)

add_subdirectory(plugin_manager)

add_subdirectory(graphics_plugins)

add_subdirectory(audio_plugins)

add_subdirectory(network_plugins)

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

    add_test(NAME EventBusTests COMMAND test_eventbus)

    set_property(TARGET test_eventbus PROPERTY CXX_STANDARD 20)
endif()