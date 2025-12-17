#pragma once

#include <string>

namespace approach_b {

/**
 * @brief Represents a 2D position in the game world
 */
struct Position {
    float x;
    float y;

    Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
};

/**
 * @brief Event data for enemy destruction
 */
struct EnemyDestroyedEvent {
    int enemyId;
    Position position;
    int points;

    EnemyDestroyedEvent(int id, Position pos, int pts = 100)
        : enemyId(id), position(pos), points(pts) {}
};

}
