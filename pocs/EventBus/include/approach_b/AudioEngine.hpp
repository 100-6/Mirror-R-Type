#pragma once

#include "EventBus.hpp"
#include <string>
#include <vector>

namespace approach_b {

/**
 * @brief Audio engine that subscribes to game events
 */
class AudioEngine {
public:
    explicit AudioEngine(EventBus& eventBus);
    ~AudioEngine();

    /**
     * @brief Get the list of played sounds (for testing)
     * @return Vector of sound file names
     */
    const std::vector<std::string>& getPlayedSounds() const { return playedSounds_; }

    /**
     * @brief Clear the played sounds history
     */
    void clear() { playedSounds_.clear(); }

private:
    void onEnemyDestroyed(const EnemyDestroyedEvent& event);

    EventBus& eventBus_;
    EventBus::SubscriptionId subscriptionId_;
    std::vector<std::string> playedSounds_;
};

}
