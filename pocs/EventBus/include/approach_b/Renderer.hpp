#pragma once

#include "EventBus.hpp"
#include "Common.hpp"
#include <vector>

namespace approach_b {

/**
 * @brief Renderer that subscribes to game events for visual effects
 */
class Renderer {
public:
    explicit Renderer(EventBus& eventBus);
    ~Renderer();

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
    void onEnemyDestroyed(const EnemyDestroyedEvent& event);

    EventBus& eventBus_;
    EventBus::SubscriptionId subscriptionId_;
    std::vector<Position> particlePositions_;
};

}
