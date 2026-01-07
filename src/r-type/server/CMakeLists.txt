add_executable(r-type_server
    src/main.cpp
    src/Server.cpp
    src/NetworkHandler.cpp
    src/PacketSender.cpp
    src/GameSessionManager.cpp
    src/SessionThreadPool.cpp
    src/LobbyManager.cpp
    src/RoomManager.cpp
    src/GameSession.cpp
    src/WaveManager.cpp
    src/ServerNetworkSystem.cpp
)

target_include_directories(r-type_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/r-type/shared
        ${CMAKE_SOURCE_DIR}/src/r-type/game-logic/include
)

find_package(Threads REQUIRED)

target_link_libraries(r-type_server
    PRIVATE
        game_engine
        rtype_logic
        Threads::Threads
)

if(WIN32)
    target_link_libraries(r-type_server PRIVATE ws2_32)
endif()

set_target_properties(r-type_server PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)
