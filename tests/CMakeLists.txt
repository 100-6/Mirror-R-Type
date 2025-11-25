cmake_minimum_required(VERSION 3.20)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Include engine headers
include_directories(${CMAKE_SOURCE_DIR}/engine/include)

# EventBus tests
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
