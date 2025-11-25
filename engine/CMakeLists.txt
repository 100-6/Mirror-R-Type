# Add plugin manager
add_subdirectory(plugin_manager)

# Add plugins
add_subdirectory(plugins/graphics/raylib)
add_subdirectory(plugins/audio/miniaudio)

# Engine library
add_library(rtype_engine
    src/core/event/EventBus.cpp
)

target_include_directories(rtype_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/systems
        ${CMAKE_CURRENT_SOURCE_DIR}/systems/temp
)

# Link with plugin_manager for plugin interfaces
target_link_libraries(rtype_engine
    PUBLIC
        plugin_manager
)

# Build the movement demo executable
add_executable(demo_movement
    tests/demo_movement.cpp
)

target_include_directories(demo_movement
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/systems
        ${CMAKE_CURRENT_SOURCE_DIR}/systems/temp
)

target_link_libraries(demo_movement
    PRIVATE
        rtype_engine
        plugin_manager
)

# Platform-specific settings for cross-platform compatibility
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(demo_movement PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
    # macOS-specific settings
    target_compile_options(demo_movement PRIVATE -Wall -Wextra)
elseif(UNIX)
    # Linux-specific settings
    target_compile_options(demo_movement PRIVATE -Wall -Wextra)
    target_link_libraries(demo_movement PRIVATE pthread)
endif()
