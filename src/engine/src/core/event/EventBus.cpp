#include "core/event/EventBus.hpp"
#include <algorithm>

namespace core {

/**
 * @brief Constructor initializing the subscription ID counter
 */
EventBus::EventBus() : nextSubscriptionId_(0) {}

/**
 * @brief Internal implementation of subscription logic
 *
 * Adds a callback to the subscribers map for the given event type.
 * Each subscription is assigned a unique ID for later unsubscription.
 *
 * @param typeIndex The type_index of the event type
 * @param callback The callback function wrapped in type erasure
 * @return Unique subscription ID
 */
EventBus::SubscriptionId EventBus::subscribeImpl(std::type_index typeIndex, Callback callback) {
    SubscriptionId id = nextSubscriptionId_++;

    subscribers_[typeIndex].push_back({id, callback});
    return id;
}

/**
 * @brief Internal implementation of immediate event publishing
 *
 * Finds all subscribers for the given event type and invokes their
 * callbacks synchronously in the order they subscribed.
 *
 * @param typeIndex The type_index of the event type
 * @param event The event data wrapped in std::any
 */
void EventBus::publishImpl(std::type_index typeIndex, const std::any& event) {
    auto it = subscribers_.find(typeIndex);
    if (it != subscribers_.end())
        for (auto& [id, callback] : it->second)
            callback(event);
}

/**
 * @brief Internal implementation of deferred event publishing
 *
 * Creates a lambda that captures the event and type information,
 * then queues it for later execution during process_deferred().
 *
 * The event data is copied into the lambda to ensure it remains
 * valid when the deferred event is processed.
 *
 * @param typeIndex The type_index of the event type
 * @param event The event data wrapped in std::any
 */
void EventBus::publishDeferredImpl(std::type_index typeIndex, const std::any& event) {
    auto publisher = [this, typeIndex, event]() {
        auto it = subscribers_.find(typeIndex);
        if (it != subscribers_.end())
            for (auto& [id, callback] : it->second)
                callback(event);
    };
    deferredEvents_.push(publisher);
}

/**
 * @brief Process all deferred events in the queue
 *
 * This method should typically be called once per frame, at a point where
 * it's safe to handle all pending events (e.g., end of update loop).
 *
 * Events are processed in FIFO order (first queued, first processed).
 * The queue is cleared after processing all events.
 */
void EventBus::process_deferred() {
    while (!deferredEvents_.empty()) {
        auto& publisher = deferredEvents_.front();
        publisher();
        deferredEvents_.pop();
    }
}

/**
 * @brief Unsubscribe from events using a subscription ID
 *
 * Iterates through all event types and removes any subscription
 * matching the given ID. If the ID is not found, this method
 * does nothing (no error is thrown).
 *
 * Uses the erase-remove idiom for efficient removal from vectors.
 *
 * @param subscriptionId The ID returned by subscribe()
 */
void EventBus::unsubscribe(SubscriptionId subscriptionId) {
    for (auto& [typeIndex, subs] : subscribers_) {
        auto it = std::remove_if(subs.begin(), subs.end(),
            [subscriptionId](const auto& sub) {return sub.first == subscriptionId;});
        subs.erase(it, subs.end());
    }
}

/**
 * @brief Clear all subscribers and pending events
 *
 * This is primarily useful for testing or when resetting the game state.
 * After calling this, the EventBus will be in a clean state with:
 * - No subscribers for any event type
 * - No pending deferred events in the queue
 * - Subscription ID counter reset to 0
 */
void EventBus::clear() {
    subscribers_.clear();
    while (!deferredEvents_.empty())
        deferredEvents_.pop();
    nextSubscriptionId_ = 0;
}

/**
 * @brief Internal implementation to get subscriber count
 *
 * Looks up the subscriber list for the given event type and
 * returns the number of active subscribers.
 *
 * @param typeIndex The type_index of the event type
 * @return Number of active subscribers, or 0 if none
 */
size_t EventBus::getSubscriberCountImpl(std::type_index typeIndex) const {
    auto it = subscribers_.find(typeIndex);

    return (it != subscribers_.end()) ? it->second.size() : 0;
}

/**
 * @brief Get the number of deferred events waiting to be processed
 *
 * This can be useful for debugging or monitoring purposes to see
 * how many events are queued and waiting for process_deferred().
 *
 * @return Number of events in the deferred queue
 */
size_t EventBus::getDeferredEventCount() const {
    return deferredEvents_.size();
}

}
