find_package(nlohmann_json CONFIG REQUIRED)

add_library(rtype_logic
    src/systems/AISystem.cpp
    src/systems/HealthSystem.cpp
    src/systems/HitEffectSystem.cpp
    src/systems/ScoreSystem.cpp
    src/systems/ScrollingSystem.cpp
    src/systems/ShootingSystem.cpp
    src/systems/CollisionSystem.cpp
    src/systems/PlayerInputSystem.cpp
    src/systems/WaveSpawnerSystem.cpp
    src/utils/WaveConfigLoader.cpp
    src/systems/BonusSystem.cpp
    src/systems/HUDSystem.cpp
    src/systems/GameStateSystem.cpp
    src/systems/AttachmentSystem.cpp
    # Map system
    src/AutoTiler.cpp
    src/MapConfigLoader.cpp
    src/ChunkManagerSystem.cpp
    src/ParallaxBackgroundSystem.cpp
    src/MapCollisionManager.cpp
)

target_include_directories(rtype_logic
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/r-type/shared
)

target_link_libraries(rtype_logic
    PUBLIC
        game_engine
        nlohmann_json::nlohmann_json
        rtype_protocol
)
