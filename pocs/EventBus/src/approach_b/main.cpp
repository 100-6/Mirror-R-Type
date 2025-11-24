#include "approach_b/EventBus.hpp"
#include "approach_b/AudioEngine.hpp"
#include "approach_b/ScoreManager.hpp"
#include "approach_b/Renderer.hpp"
#include "approach_b/PhysicsEngine.hpp"
#include <iostream>
#include <iomanip>

using namespace approach_b;

int main() {
    std::cout << "=== Approach B: Event Bus Demo ===\n\n";
    EventBus eventBus;
    AudioEngine audio(eventBus);
    ScoreManager score(eventBus);
    Renderer renderer(eventBus);
    PhysicsEngine physics(eventBus);

    std::cout << "Event bus subscribers: "
              << eventBus.getSubscriberCount<EnemyDestroyedEvent>() << "\n";
    std::cout << "Simulating enemy destructions...\n";
    const int NUM_COLLISIONS = 5;
    for (int i = 0; i < NUM_COLLISIONS; ++i) {
        physics.checkCollision(i, Position(100.0f * i, 200.0f * i), 100);
    }
    std::cout << "\n=== Results ===\n";
    std::cout << "Collisions processed: " << physics.getCollisionCount() << "\n";
    std::cout << "Events published: " << eventBus.getEventCount() << "\n";
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
