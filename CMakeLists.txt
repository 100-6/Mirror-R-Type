cmake_minimum_required(VERSION 3.20)
project(rtype LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CLIENT "Build client executable" ON)
option(BUILD_SERVER "Build server executable" ON)

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
# include(CPM)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Add engine (includes plugin_manager and plugins)
add_subdirectory(engine)

# Add tests
add_subdirectory(tests)

if (BUILD_SERVER)
    add_subdirectory(server)
endif()

if (BUILD_CLIENT)
    add_subdirectory(client)
endif()
