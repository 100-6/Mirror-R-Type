#include "approach_b/ScoreManager.hpp"

namespace approach_b {

ScoreManager::ScoreManager(EventBus& eventBus)
    : eventBus_(eventBus)
    , totalScore_(0) {
    // Subscribe to EnemyDestroyed events
    subscriptionId_ = eventBus_.subscribe<EnemyDestroyedEvent>(
        [this](const EnemyDestroyedEvent& event) {
            this->onEnemyDestroyed(event);
        }
    );
}

ScoreManager::~ScoreManager() {
    // Unsubscribe when destroyed
    eventBus_.unsubscribe(subscriptionId_);
}

void ScoreManager::onEnemyDestroyed(const EnemyDestroyedEvent& event) {
    totalScore_ += event.points;
    pointsHistory_.push_back(event.points);
}

}
