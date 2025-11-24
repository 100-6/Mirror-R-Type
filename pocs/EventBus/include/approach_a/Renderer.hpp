#pragma once

#include "Common.hpp"
#include <vector>

namespace approach_a {

/**
 * @brief Renderer responsible for visual effects
 */
class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    /**
     * @brief Spawn particle effect at a position
     * @param position The position where particles should spawn
     */
    void spawnParticles(const Position& position);

    /**
     * @brief Get the list of particle positions (for testing)
     * @return Vector of positions
     */
    const std::vector<Position>& getParticlePositions() const { return particlePositions_; }

    /**
     * @brief Clear particle history
     */
    void clear() { particlePositions_.clear(); }

private:
    std::vector<Position> particlePositions_;
};

}
