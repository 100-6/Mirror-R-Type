find_package(nlohmann_json CONFIG REQUIRED)
find_package(Lua REQUIRED)

add_library(rtype_logic
    src/systems/AISystem.cpp
    src/systems/HealthSystem.cpp
    src/systems/HitEffectSystem.cpp
    src/systems/ScoreSystem.cpp
    src/systems/LevelUpSystem.cpp
    src/systems/ScrollingSystem.cpp
    src/systems/ShootingSystem.cpp
    src/systems/CollisionSystem.cpp
    src/systems/PlayerInputSystem.cpp

    src/systems/LevelSystem.cpp
    src/systems/BossSystem.cpp
    src/systems/CheckpointSystem.cpp
    src/utils/WaveConfigLoader.cpp
    src/systems/BonusSystem.cpp
    src/systems/BonusWeaponSystem.cpp
    src/systems/CompanionSystem.cpp
    src/systems/MuzzleFlashSystem.cpp
    src/systems/HUDSystem.cpp
    src/systems/GameStateSystem.cpp
    src/systems/AttachmentSystem.cpp
    src/systems/LuaSystem.cpp
    # Map system
    src/AutoTiler.cpp
    src/MapConfigLoader.cpp
    src/ChunkManagerSystem.cpp
    src/ParallaxBackgroundSystem.cpp
    src/MapCollisionManager.cpp
    src/ProceduralMapGenerator.cpp
)

target_include_directories(rtype_logic
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/r-type/shared
        ${SOL2_INCLUDE_DIR}
)

target_link_libraries(rtype_logic
    PUBLIC
        game_engine
        nlohmann_json::nlohmann_json
        rtype_protocol
        ${LUA_LIBRARIES}
)
