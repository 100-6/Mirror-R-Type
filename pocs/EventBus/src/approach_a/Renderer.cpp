#include "approach_a/Renderer.hpp"

namespace approach_a {

void Renderer::spawnParticles(const Position& position) {
    // In a real implementation, this would create particle effects
    // For the POC, we just record the particle spawn position
    particlePositions_.push_back(position);
}

}
