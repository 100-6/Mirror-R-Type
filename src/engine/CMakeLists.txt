# Engine library (core only, without plugins)
add_library(rtype_engine
    src/ecs/SparseSet.cpp
    src/core/event/EventBus.cpp
    src/plugin_manager/PluginManager.cpp
)

target_include_directories(rtype_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link system libraries for dynamic loading
if(WIN32)
    target_link_libraries(rtype_engine PUBLIC)
else()
    target_link_libraries(rtype_engine PUBLIC ${CMAKE_DL_LIBS})
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

# Set output name based on platform
if(WIN32)
    set_target_properties(raylib_graphics PROPERTIES
        OUTPUT_NAME "raylib_graphics"
        PREFIX ""
        SUFFIX ".dll"
    )
else()
    set_target_properties(raylib_graphics PROPERTIES
        OUTPUT_NAME "raylib_graphics"
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()

# ============================================
# PLUGIN: Miniaudio Audio (Dynamic Library)
# ============================================
add_library(miniaudio_audio SHARED
    src/plugins/audio/miniaudio/MiniaudioPlugin.cpp
)

target_include_directories(miniaudio_audio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(miniaudio_audio PRIVATE PLUGIN_EXPORTS)

if(WIN32)
    set_target_properties(miniaudio_audio PROPERTIES
        OUTPUT_NAME "miniaudio_audio"
        PREFIX ""
        SUFFIX ".dll"
    )
else()
    set_target_properties(miniaudio_audio PROPERTIES
        OUTPUT_NAME "miniaudio_audio"
        PREFIX "lib"
        SUFFIX ".so"
    )
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

target_compile_definitions(asio_network PRIVATE PLUGIN_EXPORTS)

if(WIN32)
    set_target_properties(asio_network PROPERTIES
        OUTPUT_NAME "asio_network"
        PREFIX ""
        SUFFIX ".dll"
    )
else()
    set_target_properties(asio_network PROPERTIES
        OUTPUT_NAME "asio_network"
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()