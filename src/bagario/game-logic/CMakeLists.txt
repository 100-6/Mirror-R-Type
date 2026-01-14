add_library(bagario_logic
    src/systems/FoodSpawnerSystem.cpp
    src/systems/MassSystem.cpp
    src/systems/BagarioCollisionSystem.cpp
    src/systems/MapBoundsSystem.cpp
    src/systems/MovementTargetSystem.cpp
)

target_include_directories(bagario_logic
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bagario_logic
    PUBLIC
        game_engine
        bagario::protocol
)

add_library(bagario::logic ALIAS bagario_logic)
