add_library(rtype_game
    src/systems/AISystem.cpp
    src/systems/HealthSystem.cpp
    src/systems/HitEffectSystem.cpp
    src/systems/ScoreSystem.cpp
    src/systems/ScrollingSystem.cpp
    src/systems/ShootingSystem.cpp
    src/systems/CollisionSystem.cpp
)

target_include_directories(rtype_game
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
)

target_link_libraries(rtype_game
    PUBLIC
        rtype_engine
)
