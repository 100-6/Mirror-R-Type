#pragma once

#include "EventBus.hpp"
#include "Common.hpp"

namespace approach_b {

/**
 * @brief Physics engine that publishes collision events
 *
 * This approach uses the event bus for communication.
 * The PhysicsEngine only depends on the EventBus, not on other systems.
 */
class PhysicsEngine {
public:
    explicit PhysicsEngine(EventBus& eventBus);
    ~PhysicsEngine() = default;

    /**
     * @brief Check for collisions and publish events
     * @param enemyId The ID of the destroyed enemy
     * @param position The position of the collision
     * @param points The points to award
     */
    void checkCollision(int enemyId, const Position& position, int points = 100);

    /**
     * @brief Get the number of collisions processed
     * @return Number of collisions
     */
    int getCollisionCount() const { return collisionCount_; }

    /**
     * @brief Reset collision counter
     */
    void reset() { collisionCount_ = 0; }

private:
    EventBus& eventBus_;
    int collisionCount_;
};

}
