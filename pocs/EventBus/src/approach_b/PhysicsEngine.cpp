#include "approach_b/PhysicsEngine.hpp"

namespace approach_b {

PhysicsEngine::PhysicsEngine(EventBus& eventBus)
    : eventBus_(eventBus)
    , collisionCount_(0) {
}

void PhysicsEngine::checkCollision(int enemyId, const Position& position, int points) {
    // When a collision is detected, publish an event
    // The PhysicsEngine doesn't know or care who is listening
    EnemyDestroyedEvent event(enemyId, position, points);
    eventBus_.publish(event);

    collisionCount_++;
}

}
