cmake_minimum_required(VERSION 3.20)
project(PluginManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files for plugin manager
set(PLUGIN_MANAGER_SOURCES
    src/PluginManager.cpp
)

# Header files
set(PLUGIN_MANAGER_HEADERS
    include/IPlugin.hpp
    include/PluginManager.hpp
)

# Create static library
add_library(plugin_manager STATIC
    ${PLUGIN_MANAGER_SOURCES}
    ${PLUGIN_MANAGER_HEADERS}
)

# Include directories
target_include_directories(plugin_manager
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link with dl library for dynamic loading on Linux
if(UNIX AND NOT APPLE)
    target_link_libraries(plugin_manager PUBLIC dl)
endif()

# Set compiler flags
target_compile_options(plugin_manager PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
)

# Installation rules
install(TARGETS plugin_manager
    EXPORT PluginManagerTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/plugin_manager
)

# Optional: Build tests
option(BUILD_PLUGIN_MANAGER_TESTS "Build plugin manager tests" OFF)
if(BUILD_PLUGIN_MANAGER_TESTS)
    # Test plugin (shared library)
    add_library(test_plugin SHARED
        tests/test_plugin.cpp
    )
    target_include_directories(test_plugin PRIVATE include)
    set_target_properties(test_plugin PROPERTIES PREFIX "")
    
    # Test executable
    add_executable(test_plugin_manager
        tests/test_plugin_manager.cpp
    )
    target_link_libraries(test_plugin_manager PRIVATE plugin_manager)
    
    message(STATUS "Plugin Manager tests enabled")
endif()

# Optional: Build example
option(BUILD_PLUGIN_MANAGER_EXAMPLES "Build plugin manager examples" OFF)
if(BUILD_PLUGIN_MANAGER_EXAMPLES)
    add_executable(plugin_manager_example
        examples/example_usage.cpp
    )
    target_link_libraries(plugin_manager_example PRIVATE plugin_manager)
    
    message(STATUS "Plugin Manager examples enabled")
endif()
