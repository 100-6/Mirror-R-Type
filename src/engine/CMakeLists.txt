# Game Engine library (core only, without plugins)
add_library(game_engine
    src/ecs/SparseSet.cpp
    src/ecs/Registry.cpp
    src/ecs/systems/MovementSystem.cpp
    src/ecs/systems/PhysiqueSystem.cpp
    src/ecs/systems/InputSystem.cpp
    src/ecs/systems/RenderSystem.cpp
    src/ecs/systems/NetworkSystem.cpp
    src/ecs/systems/DestroySystem.cpp
    src/ecs/systems/AudioSystem.cpp
    src/ecs/systems/SpriteAnimationSystem.cpp
    src/core/event/EventBus.cpp
    src/plugin_manager/PluginManager.cpp
)

target_include_directories(game_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/r-type/shared
)

# Link system libraries for dynamic loading
if(WIN32)
    target_link_libraries(game_engine PUBLIC)
else()
    target_link_libraries(game_engine PUBLIC ${CMAKE_DL_LIBS})
endif()

# ============================================
# PLUGIN: Raylib Graphics (Dynamic Library)
# ============================================
find_package(raylib QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
    )
    FetchContent_MakeAvailable(raylib)
endif()

find_package(glfw3 REQUIRED)

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
        glfw
)

target_compile_definitions(raylib_graphics PRIVATE PLUGIN_EXPORTS)

# Ensure the plugin can find shared libraries at runtime
set_target_properties(raylib_graphics PROPERTIES
    INSTALL_RPATH "$ORIGIN:$ORIGIN/../vcpkg_installed/x64-linux-dynamic/lib:$ORIGIN/../vcpkg_installed/x64-linux-dynamic/debug/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Set output to plugins directory with unified naming
set_target_properties(raylib_graphics PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    OUTPUT_NAME "raylib_graphics"
    PREFIX ""
)

# For multi-config generators (Visual Studio, Xcode)
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set_target_properties(raylib_graphics PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
    )
endforeach()

if(WIN32)
    set_target_properties(raylib_graphics PROPERTIES SUFFIX ".dll")
    
    # Copy raylib.dll and glfw3.dll to build directory for runtime loading
    add_custom_command(TARGET raylib_graphics POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:raylib>
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:glfw>
            ${CMAKE_BINARY_DIR}
        COMMENT "Copying raylib and glfw DLLs to build directory"
    )
else()
    set_target_properties(raylib_graphics PROPERTIES SUFFIX ".so")
endif()

# ============================================
# PLUGIN: Miniaudio Audio (Dynamic Library)
# ============================================
find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")

add_library(miniaudio_audio SHARED
    src/plugins/audio/miniaudio/MiniaudioPlugin.cpp
)

target_include_directories(miniaudio_audio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${MINIAUDIO_INCLUDE_DIRS}
)

target_compile_definitions(miniaudio_audio PRIVATE PLUGIN_EXPORTS)

# Set output to plugins directory with unified naming
set_target_properties(miniaudio_audio PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    OUTPUT_NAME "miniaudio_audio"
    PREFIX ""
)

# For multi-config generators (Visual Studio, Xcode)
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set_target_properties(miniaudio_audio PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
    )
endforeach()

if(WIN32)
    set_target_properties(miniaudio_audio PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(miniaudio_audio PROPERTIES SUFFIX ".so")
endif()

# ============================================
# PLUGIN: Asio Network (Dynamic Library)
# ============================================
find_package(Boost REQUIRED COMPONENTS system)

add_library(asio_network SHARED
    src/plugins/network/asio/AsioNetworkPlugin.cpp
)

target_include_directories(asio_network
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(asio_network
    PRIVATE
        Boost::system
)

if(WIN32)
    target_link_libraries(asio_network PRIVATE ws2_32 mswsock)
endif()

target_compile_definitions(asio_network PRIVATE PLUGIN_EXPORTS)

# Set output to plugins directory with unified naming
set_target_properties(asio_network PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    OUTPUT_NAME "asio_network"
    PREFIX ""
)

# For multi-config generators (Visual Studio, Xcode)
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set_target_properties(asio_network PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
    )
endforeach()

if(WIN32)
    set_target_properties(asio_network PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(asio_network PROPERTIES SUFFIX ".so")
endif()

# ============================================
# PLUGIN: Raylib Input (Dynamic Library)
# ============================================
add_library(raylib_input SHARED
    src/plugins/input/raylib/RaylibInputPlugin.cpp
)

target_include_directories(raylib_input
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raylib_input
    PRIVATE
        raylib
        glfw
)

target_compile_definitions(raylib_input PRIVATE PLUGIN_EXPORTS)

# Ensure the plugin can find shared libraries at runtime
set_target_properties(raylib_input PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Set output to plugins directory with unified naming
set_target_properties(raylib_input PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    OUTPUT_NAME "raylib_input"
    PREFIX ""
)

# For multi-config generators (Visual Studio, Xcode)
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set_target_properties(raylib_input PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
    )
endforeach()

if(WIN32)
    set_target_properties(raylib_input PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(raylib_input PROPERTIES SUFFIX ".so")
endif()

find_package(unofficial-enet CONFIG REQUIRED)

add_library(enet_network SHARED
    src/plugins/network/enet/EnetNetworkPlugin.cpp
)

target_include_directories(enet_network
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(enet_network
    PRIVATE
        unofficial::enet::enet
)

if(WIN32)
    target_link_libraries(enet_network PRIVATE ws2_32 winmm)
endif()

target_compile_definitions(enet_network PRIVATE PLUGIN_EXPORTS)

set_target_properties(enet_network PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    OUTPUT_NAME "enet_network"
    PREFIX ""
)

foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set_target_properties(enet_network PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
        ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/plugins"
    )
endforeach()

if(WIN32)
    set_target_properties(enet_network PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(enet_network PROPERTIES SUFFIX ".so")
endif()
