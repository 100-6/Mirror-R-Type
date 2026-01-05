# client/CMakeLists.txt

# R-Type Network Client - Multiplayer graphical client
set(RTYPE_CLIENT_SOURCES
    src/main.cpp
    src/NetworkClient.cpp
    src/ClientGame.cpp
    src/TextureManager.cpp
    src/InputHandler.cpp
    src/ScreenManager.cpp
    src/EntityManager.cpp
    src/StatusOverlay.cpp
    src/SpaceshipManager.cpp
)

add_executable(r-type_client ${RTYPE_CLIENT_SOURCES})

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

# Link Winsock on Windows
if(WIN32)
    target_link_libraries(r-type_client PRIVATE ws2_32)
endif()

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

# Link Winsock on Windows
if(WIN32)
    target_link_libraries(r-type_test_client PRIVATE ws2_32)
endif()

set_target_properties(r-type_test_client PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)

