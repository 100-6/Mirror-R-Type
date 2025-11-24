#include "approach_a/PhysicsEngine.hpp"

namespace approach_a {

PhysicsEngine::PhysicsEngine(AudioEngine& audio, ScoreManager& score, Renderer& renderer)
    : audioEngine_(audio)
    , scoreManager_(score)
    , renderer_(renderer)
    , collisionCount_(0) {
}

void PhysicsEngine::checkCollision(const CollisionData& collision) {
    // When a collision is detected, directly call all dependent systems
    // This creates tight coupling between components
    audioEngine_.playSound("explosion.wav");
    scoreManager_.addPoints(collision.points);
    renderer_.spawnParticles(collision.position);

    collisionCount_++;
}

}
