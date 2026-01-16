# Bagario Client

set(BAGARIO_CLIENT_SOURCES
    src/main.cpp
    src/BagarioGame.cpp
    src/ScreenManager.cpp
    src/ClientGameState.cpp
    src/Camera.cpp
    src/screens/WelcomeScreen.cpp
    src/screens/SettingsScreen.cpp
    src/screens/SkinScreen.cpp
    src/screens/PlayingScreen.cpp
    src/network/NetworkManager.cpp
    src/network/BagarioClientNetworkHandler.cpp
    src/network/BagarioClientPacketSender.cpp
    src/ui/UIButton.cpp
    src/ui/UILabel.cpp
    src/ui/UITextField.cpp
    src/ui/UIColorPicker.cpp
)

add_executable(bagario_client ${BAGARIO_CLIENT_SOURCES})

target_include_directories(bagario_client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/bagario/shared
        ${CMAKE_SOURCE_DIR}/src/bagario/shared/protocol
)

target_link_libraries(bagario_client
    PRIVATE
        game_engine
)

# Link Winsock on Windows
if(WIN32)
    target_link_libraries(bagario_client PRIVATE ws2_32)
endif()

# Output to bin directory
set_target_properties(bagario_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    INSTALL_RPATH "$ORIGIN:$ORIGIN/vcpkg_installed/x64-linux-dynamic/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

install(TARGETS bagario_client
    RUNTIME DESTINATION bin
)
