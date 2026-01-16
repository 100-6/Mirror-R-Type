# client/CMakeLists.txt

# R-Type Network Client - Multiplayer graphical client
set(RTYPE_CLIENT_SOURCES
    src/main.cpp
    src/NetworkClient.cpp
    src/ClientGame.cpp
    src/TextureManager.cpp
    src/InputHandler.cpp
    src/KeyBindings.cpp
    src/ScreenManager.cpp
    src/EntityManager.cpp
    src/StatusOverlay.cpp
    src/MenuManager.cpp
    src/ui/UIButton.cpp
    src/ui/UITextField.cpp
    src/ui/UILabel.cpp
    src/ui/ConsoleOverlay.cpp
    # Screen classes
    src/screens/MainMenuScreen.cpp
    src/screens/CreateRoomScreen.cpp
    src/screens/BrowseRoomsScreen.cpp
    src/screens/RoomLobbyScreen.cpp
    src/screens/PasswordDialog.cpp
    src/screens/SkinSelectorDialog.cpp
    src/screens/SettingsScreen.cpp
    # CreateRoom module
    src/screens/createroom/CreateRoomInput.cpp
    src/screens/createroom/CreateRoomRenderer.cpp
    src/screens/createroom/CreateRoomInitializer.cpp
    src/screens/createroom/CreateRoomUpdater.cpp
    src/screens/createroom/CreateRoomDrawer.cpp
    src/SpaceshipManager.cpp
    src/systems/LocalPredictionSystem.cpp
    src/systems/ColliderDebugSystem.cpp
    src/systems/ClientPredictionSystem.cpp
    src/systems/InterpolationSystem.cpp
    src/systems/ClientHitFlashSystem.cpp
    src/DebugNetworkOverlay.cpp
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
        rtype_protocol
)

# Link Winsock on Windows
if(WIN32)
    target_link_libraries(r-type_client PRIVATE ws2_32)
endif()

# Ensure the client can find shared libraries at runtime
set_target_properties(r-type_client PROPERTIES
    INSTALL_RPATH "$ORIGIN:$ORIGIN/vcpkg_installed/x64-linux-dynamic/lib:$ORIGIN/vcpkg_installed/x64-linux-dynamic/debug/lib"
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
        rtype_protocol
)

# Link Winsock on Windows
if(WIN32)
    target_link_libraries(r-type_test_client PRIVATE ws2_32)
endif()

set_target_properties(r-type_test_client PROPERTIES
    INSTALL_RPATH "$ORIGIN:$ORIGIN/vcpkg_installed/x64-linux-dynamic/lib:$ORIGIN/vcpkg_installed/x64-linux-dynamic/debug/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)
