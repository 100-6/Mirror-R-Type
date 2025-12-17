#pragma once

#include "EventBus.hpp"
#include <vector>

namespace approach_b {

/**
 * @brief Manages the game score by subscribing to game events
 */
class ScoreManager {
public:
    explicit ScoreManager(EventBus& eventBus);
    ~ScoreManager();

    /**
     * @brief Get the current total score
     * @return The total score
     */
    int getTotalScore() const { return totalScore_; }

    /**
     * @brief Get all point additions (for testing)
     * @return Vector of point values
     */
    const std::vector<int>& getPointsHistory() const { return pointsHistory_; }

    /**
     * @brief Reset the score
     */
    void reset() {
        totalScore_ = 0;
        pointsHistory_.clear();
    }

private:
    void onEnemyDestroyed(const EnemyDestroyedEvent& event);

    EventBus& eventBus_;
    EventBus::SubscriptionId subscriptionId_;
    int totalScore_;
    std::vector<int> pointsHistory_;
};

}
