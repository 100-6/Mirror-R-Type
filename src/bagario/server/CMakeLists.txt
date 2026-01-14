add_executable(bagario_server
    src/main.cpp
    src/BagarioServer.cpp
    src/BagarioSession.cpp
    src/BagarioNetworkHandler.cpp
    src/BagarioPacketSender.cpp
)

target_include_directories(bagario_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bagario_server
    PRIVATE
        game_engine
        bagario::protocol
        bagario::logic
)

set_target_properties(bagario_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS bagario_server
    RUNTIME DESTINATION bin
)
