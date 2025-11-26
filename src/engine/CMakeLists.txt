# Engine library
add_library(rtype_engine
    src/ecs/SparseSet.cpp
    src/ecs/Registry.cpp
    src/core/event/EventBus.cpp
    src/plugin_manager/PluginManager.cpp
    src/plugins/graphics/raylib/RaylibGraphicsPlugin.cpp
    src/plugins/audio/miniaudio/MiniaudioPlugin.cpp
    src/plugins/network/asio/AsioNetworkPlugin.cpp
)

target_include_directories(rtype_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find Boost for Asio network plugin (using vcpkg)
find_package(Boost REQUIRED COMPONENTS system)
target_link_libraries(rtype_engine PUBLIC Boost::system ${CMAKE_DL_LIBS})

# # Find and link raylib
# find_package(raylib QUIET)
# if(NOT raylib_FOUND)
#     include(FetchContent)
#     FetchContent_Declare(
#         raylib
#         GIT_REPOSITORY https://github.com/raysan5/raylib.git
#         GIT_TAG 5.0
#     )
#     FetchContent_MakeAvailable(raylib)
# endif()

# target_link_libraries(rtype_engine
#     PUBLIC
#         raylib
#         ${CMAKE_DL_LIBS}
# )