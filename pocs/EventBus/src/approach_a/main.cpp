#include "approach_a/AudioEngine.hpp"
#include "approach_a/ScoreManager.hpp"
#include "approach_a/Renderer.hpp"
#include "approach_a/PhysicsEngine.hpp"
#include <iostream>
#include <iomanip>

using namespace approach_a;

int main() {
    std::cout << "=== Approach A: Direct Communication Demo ===\n\n";
    AudioEngine audio;
    ScoreManager score;
    Renderer renderer;
    PhysicsEngine physics(audio, score, renderer);

    std::cout << "Simulating enemy destructions...\n";
    const int NUM_COLLISIONS = 5;
    for (int i = 0; i < NUM_COLLISIONS; ++i) {
        CollisionData collision(i, Position(100.0f * i, 200.0f * i), 100);
        physics.checkCollision(collision);
    }
    std::cout << "\n=== Results ===\n";
    std::cout << "Collisions processed: " << physics.getCollisionCount() << "\n";
    std::cout << "Total score: " << score.getTotalScore() << "\n";
    std::cout << "Sounds played: " << audio.getPlayedSounds().size() << "\n";
    std::cout << "Particles spawned: " << renderer.getParticlePositions().size() << "\n";
    std::cout << "\nSound history:\n";
    for (const auto& sound : audio.getPlayedSounds()) {
        std::cout << "  - " << sound << "\n";
    }
    std::cout << "\nParticle positions:\n";
    for (const auto& pos : renderer.getParticlePositions()) {
        std::cout << "  - (" << std::fixed << std::setprecision(1)
                  << pos.x << ", " << pos.y << ")\n";
    }
    return 0;
}
