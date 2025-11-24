#pragma once

#include "Common.hpp"
#include "AudioEngine.hpp"
#include "ScoreManager.hpp"
#include "Renderer.hpp"

namespace approach_a {

/**
 * @brief Physics engine that detects collisions and coordinates with other systems
 *
 * This approach uses direct method calls to communicate with other components.
 * The PhysicsEngine has direct dependencies on all other systems.
 */
class PhysicsEngine {
public:
    /**
     * @brief Construct a PhysicsEngine with direct references to all systems
     * @param audio Reference to the audio engine
     * @param score Reference to the score manager
     * @param renderer Reference to the renderer
     */
    PhysicsEngine(AudioEngine& audio, ScoreManager& score, Renderer& renderer);
    ~PhysicsEngine() = default;

    /**
     * @brief Check for collisions and trigger appropriate responses
     * @param collision The collision data
     */
    void checkCollision(const CollisionData& collision);

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
    AudioEngine& audioEngine_;
    ScoreManager& scoreManager_;
    Renderer& renderer_;
    int collisionCount_;
};

}
