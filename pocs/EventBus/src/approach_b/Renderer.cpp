#include "approach_b/Renderer.hpp"

namespace approach_b {

Renderer::Renderer(EventBus& eventBus)
    : eventBus_(eventBus) {
    // Subscribe to EnemyDestroyed events
    subscriptionId_ = eventBus_.subscribe<EnemyDestroyedEvent>(
        [this](const EnemyDestroyedEvent& event) {
            this->onEnemyDestroyed(event);
        }
    );
}

Renderer::~Renderer() {
    // Unsubscribe when destroyed
    eventBus_.unsubscribe(subscriptionId_);
}

void Renderer::onEnemyDestroyed(const EnemyDestroyedEvent& event) {
    // In a real implementation, this would create particle effects
    // For the POC, we just record the particle spawn position
    particlePositions_.push_back(event.position);
}

}
