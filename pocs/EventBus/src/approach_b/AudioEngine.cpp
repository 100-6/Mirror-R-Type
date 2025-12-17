#include "approach_b/AudioEngine.hpp"

namespace approach_b {

AudioEngine::AudioEngine(EventBus& eventBus)
    : eventBus_(eventBus) {
    // Subscribe to EnemyDestroyed events
    subscriptionId_ = eventBus_.subscribe<EnemyDestroyedEvent>(
        [this](const EnemyDestroyedEvent& event) {
            this->onEnemyDestroyed(event);
        }
    );
}

AudioEngine::~AudioEngine() {
    // Unsubscribe when destroyed
    eventBus_.unsubscribe(subscriptionId_);
}

void AudioEngine::onEnemyDestroyed(const EnemyDestroyedEvent& event) {
    // In a real implementation, this would play actual audio
    // For the POC, we just record that the sound was played
    playedSounds_.push_back("explosion.wav");
}

}
