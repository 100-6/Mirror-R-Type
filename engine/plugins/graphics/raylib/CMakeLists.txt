cmake_minimum_required(VERSION 3.20)

# Raylib Graphics Plugin
project(raylib_graphics_plugin)

# Find raylib
find_package(raylib QUIET)

if(NOT raylib_FOUND)
    # Try to use pkg-config to find raylib
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(RAYLIB raylib)
    endif()

    if(NOT RAYLIB_FOUND)
        message(WARNING "Raylib not found. Skipping raylib graphics plugin. Install raylib to build this plugin.")
        return()
    endif()
endif()

# Create the plugin as a shared library
add_library(raylib_graphics SHARED
    src/RaylibGraphicsPlugin.cpp
)

# Include directories
target_include_directories(raylib_graphics PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
)

# Link libraries
if(RAYLIB_FOUND)
    target_include_directories(raylib_graphics PRIVATE ${RAYLIB_INCLUDE_DIRS})
    target_link_directories(raylib_graphics PRIVATE ${RAYLIB_LIBRARY_DIRS})
    target_link_libraries(raylib_graphics PRIVATE ${RAYLIB_LINK_LIBRARIES})
else()
    target_link_libraries(raylib_graphics PRIVATE raylib)
endif()

# Set properties
set_target_properties(raylib_graphics PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    PREFIX ""  # Remove 'lib' prefix on Linux
    OUTPUT_NAME "raylib_graphics"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    # Linux-specific settings
    target_link_libraries(raylib_graphics PRIVATE ${CMAKE_DL_LIBS})
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(raylib_graphics PROPERTIES
        SUFFIX ".dylib"
    )
elseif(WIN32)
    # Windows-specific settings
    set_target_properties(raylib_graphics PROPERTIES
        SUFFIX ".dll"
    )
endif()

# Installation rules
install(TARGETS raylib_graphics
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)

# Tests are now in tests/graphics_plugins/
# See tests/CMakeLists.txt for test configuration

