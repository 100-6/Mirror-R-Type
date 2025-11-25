cmake_minimum_required(VERSION 3.20)

# Asio Network Plugin
project(asio_network_plugin)

# Find Boost.Asio (works with both vcpkg and system packages)
find_package(Boost REQUIRED COMPONENTS system)

if(NOT Boost_FOUND)
    message(WARNING "Boost not found. Skipping Asio network plugin.")
    message(WARNING "Install via vcpkg: vcpkg install boost-asio boost-system")
    message(WARNING "Or via apt: sudo apt-get install libboost-all-dev")
    return()
endif()

message(STATUS "Boost found for asio_network plugin")
message(STATUS "  Boost version: ${Boost_VERSION}")
message(STATUS "  Boost include dirs: ${Boost_INCLUDE_DIRS}")

# Create the plugin as a shared library
add_library(asio_network SHARED
    src/AsioNetworkPlugin.cpp
)

# Include directories
target_include_directories(asio_network PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
    ${Boost_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(asio_network PRIVATE
    ${Boost_LIBRARIES}
)

# Set properties
set_target_properties(asio_network PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    PREFIX ""  # Remove 'lib' prefix on Linux
    OUTPUT_NAME "asio_network"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    # Linux-specific settings
    target_link_libraries(asio_network PRIVATE ${CMAKE_DL_LIBS} pthread)
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(asio_network PROPERTIES
        SUFFIX ".dylib"
    )
elseif(WIN32)
    # Windows-specific settings
    set_target_properties(asio_network PROPERTIES
        SUFFIX ".dll"
    )
    target_link_libraries(asio_network PRIVATE ws2_32 mswsock)
endif()

# Installation rules
install(TARGETS asio_network
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)
