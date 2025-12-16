# client/CMakeLists.txt

add_executable(r-type_client
    src/main.cpp
    src/NetworkClient.cpp
)

target_include_directories(r-type_client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/r-type/shared
)

target_link_libraries(r-type_client
    PRIVATE
        game_engine
        rtype_logic
)

# Ensure the client can find shared libraries at runtime
set_target_properties(r-type_client PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# ============================================================
# TEST CLIENT - Network testing only
# ============================================================
# This is a separate executable for testing the TCP/UDP
# networking implementation without the game graphics.
# Usage: ./r-type_test_client [host] [port]
# ============================================================
add_executable(r-type_test_client
    src/test_network.cpp
    src/NetworkClient.cpp
)

target_include_directories(r-type_test_client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/r-type/shared
)

target_link_libraries(r-type_test_client
    PRIVATE
        game_engine
)

set_target_properties(r-type_test_client PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# ============================================================
# MULTIPLAYER CLIENT - Full game with network support
# ============================================================
# Usage: ./r-type_multiplayer [host] [port] [player_name]
# Default: localhost 4242 Player
# ============================================================
add_executable(r-type_multiplayer
    src/main_multiplayer.cpp
    src/NetworkClient.cpp
)

target_include_directories(r-type_multiplayer
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/r-type/shared
)

target_link_libraries(r-type_multiplayer
    PRIVATE
        game_engine
        rtype_logic
)

set_target_properties(r-type_multiplayer PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)
