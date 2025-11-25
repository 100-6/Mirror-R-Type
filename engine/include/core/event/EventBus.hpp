#pragma once

#include "Event.hpp"
#include <functional>
#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <any>

namespace core {

/**
 * @brief Generic event bus implementation using publish/subscribe pattern
 *
 * The EventBus provides a decoupled communication mechanism between systems.
 * Systems can publish events without knowing who consumes them, and subscribe
 * to events they're interested in without knowing the publisher.
 */
class EventBus {
    public:
        using SubscriptionId = size_t;

        EventBus();
        ~EventBus() = default;

        /**
         * @brief Subscribe to an event type with a callback function
         */
        template<typename EventType>
        SubscriptionId subscribe(std::function<void(const EventType&)> callback);

        /**
         * @brief Publish an event immediately to all subscribers
         */
        template<typename EventType>
        void publish(const EventType& event);

        /**
         * @brief Queue an event for deferred processing
         */
        template<typename EventType>
        void publish_deferred(const EventType& event);

        /**
         * @brief Process all deferred events in the queue
         */
        void process_deferred();

        /**
         * @brief Unsubscribe from events using a subscription ID
         */
        void unsubscribe(SubscriptionId subscriptionId);

        /**
         * @brief Clear all subscribers and pending events
         */
        void clear();

        /**
         * @brief Get the number of subscribers for a specific event type
         */
        template<typename EventType>
        size_t getSubscriberCount() const;

        /**
         * @brief Get the number of deferred events waiting to be processed
         */
        size_t getDeferredEventCount() const;

    private:
        using Callback = std::function<void(const std::any&)>;
        using Subscription = std::pair<SubscriptionId, Callback>;
        using DeferredPublisher = std::function<void()>;

        SubscriptionId subscribeImpl(std::type_index typeIndex, Callback callback);
        void publishImpl(std::type_index typeIndex, const std::any& event);
        void publishDeferredImpl(std::type_index typeIndex, const std::any& event);
        size_t getSubscriberCountImpl(std::type_index typeIndex) const;

        std::unordered_map<std::type_index, std::vector<Subscription>> subscribers_;
        std::queue<DeferredPublisher> deferredEvents_;
        SubscriptionId nextSubscriptionId_;
    };

    template<typename EventType>
    EventBus::SubscriptionId EventBus::subscribe(std::function<void(const EventType&)> callback) {
        auto wrapper = [callback](const std::any& event) {
            callback(std::any_cast<const EventType&>(event));
        };
        return subscribeImpl(typeid(EventType), wrapper);
    }

    template<typename EventType>
    void EventBus::publish(const EventType& event) {
        publishImpl(typeid(EventType), event);
    }

    template<typename EventType>
    void EventBus::publish_deferred(const EventType& event) {
        publishDeferredImpl(typeid(EventType), event);
    }

    template<typename EventType>
    size_t EventBus::getSubscriberCount() const {
        return getSubscriberCountImpl(typeid(EventType));
    }

}
