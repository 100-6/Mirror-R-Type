#include "approach_a/ScoreManager.hpp"

namespace approach_a {

void ScoreManager::addPoints(int points) {
    totalScore_ += points;
    pointsHistory_.push_back(points);
}

}
