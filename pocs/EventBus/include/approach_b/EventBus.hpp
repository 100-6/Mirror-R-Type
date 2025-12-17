#pragma once

#include "Common.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <any>

namespace approach_b {

/**
 * @brief Generic event bus implementation using pub/sub pattern
 *
 * This event bus allows components to communicate without direct dependencies.
 * Components subscribe to events by type and receive callbacks when events are published.
 */
class EventBus {
public:
    using SubscriptionId = size_t;

    EventBus() : nextSubscriptionId_(0) {}
    ~EventBus() = default;

    /**
     * @brief Subscribe to an event type with a callback
     * @tparam EventType The type of event to subscribe to
     * @param callback Function to call when event is published
     * @return Subscription ID that can be used to unsubscribe
     */
    template<typename EventType>
    SubscriptionId subscribe(std::function<void(const EventType&)> callback) {
        std::type_index typeIndex(typeid(EventType));
        SubscriptionId id = nextSubscriptionId_++;

        auto wrapper = [callback](const std::any& event) {
            callback(std::any_cast<const EventType&>(event));
        };

        subscribers_[typeIndex].push_back({id, wrapper});
        return id;
    }

    /**
     * @brief Publish an event to all subscribers
     * @tparam EventType The type of event to publish
     * @param event The event data
     */
    template<typename EventType>
    void publish(const EventType& event) {
        std::type_index typeIndex(typeid(EventType));
        auto it = subscribers_.find(typeIndex);
        if (it != subscribers_.end()) {
            for (auto& [id, callback] : it->second) {
                callback(event);
            }
        }
        eventCount_++;
    }

    /**
     * @brief Unsubscribe from an event
     * @param subscriptionId The subscription ID returned by subscribe
     */
    void unsubscribe(SubscriptionId subscriptionId) {
        for (auto& [typeIndex, subs] : subscribers_) {
            auto it = std::remove_if(subs.begin(), subs.end(),
                [subscriptionId](const auto& sub) {
                    return sub.first == subscriptionId;
                });
            subs.erase(it, subs.end());
        }
    }

    /**
     * @brief Get the total number of events published
     * @return Event count
     */
    size_t getEventCount() const { return eventCount_; }

    /**
     * @brief Reset event counter
     */
    void reset() { eventCount_ = 0; }

    /**
     * @brief Get the number of subscribers for a specific event type
     * @tparam EventType The event type
     * @return Number of subscribers
     */
    template<typename EventType>
    size_t getSubscriberCount() const {
        std::type_index typeIndex(typeid(EventType));
        auto it = subscribers_.find(typeIndex);
        return (it != subscribers_.end()) ? it->second.size() : 0;
    }

private:
    using Callback = std::function<void(const std::any&)>;
    using Subscription = std::pair<SubscriptionId, Callback>;

    std::unordered_map<std::type_index, std::vector<Subscription>> subscribers_;
    SubscriptionId nextSubscriptionId_;
    size_t eventCount_ = 0;
};

}
