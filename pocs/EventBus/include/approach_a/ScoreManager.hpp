#pragma once

#include <vector>

namespace approach_a {

/**
 * @brief Manages the game score
 */
class ScoreManager {
public:
    ScoreManager() : totalScore_(0) {}
    ~ScoreManager() = default;

    /**
     * @brief Add points to the score
     * @param points Number of points to add
     */
    void addPoints(int points);

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
    int totalScore_;
    std::vector<int> pointsHistory_;
};

}
