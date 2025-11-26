#pragma once

namespace core {

/**
 * @brief Base interface for all events in the ECS architecture
 *
 * All event types should inherit from this interface to be used
 * with the EventBus system. This ensures type safety while allowing
 * runtime polymorphism when needed.
 *
 * Events should be lightweight data structures containing only
 * the information needed to communicate between systems.
 */
struct Event {
    virtual ~Event() = default;
};

}
