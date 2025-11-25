# Add plugin manager
add_subdirectory(plugin_manager)

# Add plugins
add_subdirectory(plugins/graphics/raylib)
add_subdirectory(plugins/audio/miniaudio)
add_subdirectory(plugins/network/asio)

# Engine library
add_library(rtype_engine
    src/core/event/EventBus.cpp
)

target_include_directories(rtype_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)