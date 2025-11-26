#include "core/event/EventBus.hpp"
#include "core/event/Event.hpp"
#include <gtest/gtest.h>
#include <string>

struct TestEvent : public core::Event {
    int value;
    explicit TestEvent(int v) : value(v) {}
};

struct AnotherTestEvent : public core::Event {
    std::string message;
    explicit AnotherTestEvent(const std::string& msg) : message(msg) {}
};

struct ComplexEvent : public core::Event {
    int id;
    float x, y;
    ComplexEvent(int i, float px, float py) : id(i), x(px), y(py) {}
};

TEST(EventBusTest, SubscribeAndPublishImmediate) {
    core::EventBus bus;
    int callCount = 0;
    int receivedValue = 0;

    bus.subscribe<TestEvent>([&](const TestEvent& evt) {
        callCount++;
        receivedValue = evt.value;
    });
    bus.publish(TestEvent{42});
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(receivedValue, 42);
}

TEST(EventBusTest, MultipleSubscribersReceiveEvent) {
    core::EventBus bus;
    int callCount1 = 0, callCount2 = 0, callCount3 = 0;
    int value1 = 0, value2 = 0, value3 = 0;

    bus.subscribe<TestEvent>([&](const TestEvent& evt) {
        callCount1++;
        value1 = evt.value;
    });
    bus.subscribe<TestEvent>([&](const TestEvent& evt) {
        callCount2++;
        value2 = evt.value;
    });
    bus.subscribe<TestEvent>([&](const TestEvent& evt) {
        callCount3++;
        value3 = evt.value;
    });
    bus.publish(TestEvent{100});
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
    EXPECT_EQ(callCount3, 1);
    EXPECT_EQ(value1, 100);
    EXPECT_EQ(value2, 100);
    EXPECT_EQ(value3, 100);
}

TEST(EventBusTest, DifferentEventTypesAreIndependent) {
    core::EventBus bus;
    int testEventCount = 0;
    int anotherEventCount = 0;

    bus.subscribe<TestEvent>([&](const TestEvent&) {
        testEventCount++;
    });
    bus.subscribe<AnotherTestEvent>([&](const AnotherTestEvent&) {
        anotherEventCount++;
    });
    bus.publish(TestEvent{1});
    EXPECT_EQ(testEventCount, 1);
    EXPECT_EQ(anotherEventCount, 0);
    bus.publish(AnotherTestEvent{"hello"});
    EXPECT_EQ(testEventCount, 1);
    EXPECT_EQ(anotherEventCount, 1);
    bus.publish(TestEvent{2});
    EXPECT_EQ(testEventCount, 2);
    EXPECT_EQ(anotherEventCount, 1);
}

TEST(EventBusTest, PublishWithoutSubscribersDoesNotCrash) {
    core::EventBus bus;
    EXPECT_NO_THROW(bus.publish(TestEvent{123}));
}

TEST(EventBusTest, DeferredEventIsNotProcessedImmediately) {
    core::EventBus bus;
    int callCount = 0;

    bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount++;
    });
    bus.publish_deferred(TestEvent{42});
    EXPECT_EQ(callCount, 0);
}

TEST(EventBusTest, ProcessDeferredCallsSubscribers) {
    core::EventBus bus;
    int callCount = 0;
    int receivedValue = 0;

    bus.subscribe<TestEvent>([&](const TestEvent& evt) {
        callCount++;
        receivedValue = evt.value;
    });
    bus.publish_deferred(TestEvent{42});
    bus.process_deferred();
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(receivedValue, 42);
}

TEST(EventBusTest, MultipleDeferredEventsProcessedInOrder) {
    core::EventBus bus;
    std::vector<int> receivedValues;

    bus.subscribe<TestEvent>([&](const TestEvent& evt) {
        receivedValues.push_back(evt.value);
    });
    bus.publish_deferred(TestEvent{1});
    bus.publish_deferred(TestEvent{2});
    bus.publish_deferred(TestEvent{3});
    EXPECT_EQ(receivedValues.size(), 0);
    bus.process_deferred();
    ASSERT_EQ(receivedValues.size(), 3);
    EXPECT_EQ(receivedValues[0], 1);
    EXPECT_EQ(receivedValues[1], 2);
    EXPECT_EQ(receivedValues[2], 3);
}

TEST(EventBusTest, MixedImmediateAndDeferredEvents) {
    core::EventBus bus;
    std::vector<int> receivedValues;

    bus.subscribe<TestEvent>([&](const TestEvent& evt) {
        receivedValues.push_back(evt.value);
    });

    bus.publish(TestEvent{1});           // Immediate
    bus.publish_deferred(TestEvent{2});  // Deferred
    bus.publish(TestEvent{3});           // Immediate
    bus.publish_deferred(TestEvent{4});  // Deferred

    ASSERT_EQ(receivedValues.size(), 2);
    EXPECT_EQ(receivedValues[0], 1);
    EXPECT_EQ(receivedValues[1], 3);

    bus.process_deferred();

    ASSERT_EQ(receivedValues.size(), 4);
    EXPECT_EQ(receivedValues[2], 2);
    EXPECT_EQ(receivedValues[3], 4);
}

TEST(EventBusTest, ProcessDeferredMultipleTimes) {
    core::EventBus bus;
    int callCount = 0;

    bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount++;
    });
    bus.publish_deferred(TestEvent{1});
    bus.process_deferred();
    EXPECT_EQ(callCount, 1);
    bus.publish_deferred(TestEvent{2});
    bus.publish_deferred(TestEvent{3});
    bus.process_deferred();
    EXPECT_EQ(callCount, 3);
    bus.process_deferred();
    EXPECT_EQ(callCount, 3);
}

TEST(EventBusTest, UnsubscribeRemovesSubscriber) {
    core::EventBus bus;
    int callCount = 0;
    auto id = bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount++;
    });

    bus.publish(TestEvent{1});
    EXPECT_EQ(callCount, 1);
    bus.unsubscribe(id);
    bus.publish(TestEvent{2});
    EXPECT_EQ(callCount, 1);
}

TEST(EventBusTest, UnsubscribeOneOfMultipleSubscribers) {
    core::EventBus bus;
    int callCount1 = 0, callCount2 = 0;

    auto id1 = bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount1++;
    });
    bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount2++;
    });
    bus.publish(TestEvent{1});
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
    bus.unsubscribe(id1);
    bus.publish(TestEvent{2});
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 2);
}

TEST(EventBusTest, UnsubscribeNonExistentIdDoesNotCrash) {
    core::EventBus bus;
    EXPECT_NO_THROW(bus.unsubscribe(999999));
}

TEST(EventBusTest, GetSubscriberCount) {
    core::EventBus bus;

    EXPECT_EQ(bus.getSubscriberCount<TestEvent>(), 0);
    auto id1 = bus.subscribe<TestEvent>([](const TestEvent&) {});
    EXPECT_EQ(bus.getSubscriberCount<TestEvent>(), 1);
    auto id2 = bus.subscribe<TestEvent>([](const TestEvent&) {});
    EXPECT_EQ(bus.getSubscriberCount<TestEvent>(), 2);
    bus.subscribe<AnotherTestEvent>([](const AnotherTestEvent&) {});
    EXPECT_EQ(bus.getSubscriberCount<TestEvent>(), 2);
    EXPECT_EQ(bus.getSubscriberCount<AnotherTestEvent>(), 1);
    bus.unsubscribe(id1);
    EXPECT_EQ(bus.getSubscriberCount<TestEvent>(), 1);
    bus.unsubscribe(id2);
    EXPECT_EQ(bus.getSubscriberCount<TestEvent>(), 0);
}

TEST(EventBusTest, ClearRemovesAllSubscribers) {
    core::EventBus bus;
    int callCount = 0;

    bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount++;
    });
    bus.subscribe<AnotherTestEvent>([&](const AnotherTestEvent&) {
        callCount++;
    });
    bus.clear();
    bus.publish(TestEvent{1});
    bus.publish(AnotherTestEvent{"test"});
    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(bus.getSubscriberCount<TestEvent>(), 0);
    EXPECT_EQ(bus.getSubscriberCount<AnotherTestEvent>(), 0);
}

TEST(EventBusTest, ClearRemovesDeferredEvents) {
    core::EventBus bus;
    int callCount = 0;

    bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount++;
    });
    bus.publish_deferred(TestEvent{1});
    bus.publish_deferred(TestEvent{2});
    EXPECT_EQ(bus.getDeferredEventCount(), 2);
    bus.clear();
    EXPECT_EQ(bus.getDeferredEventCount(), 0);
    bus.process_deferred();
    EXPECT_EQ(callCount, 0);
}

TEST(EventBusTest, ComplexEventData) {
    core::EventBus bus;
    int receivedId = 0;
    float receivedX = 0.0f, receivedY = 0.0f;

    bus.subscribe<ComplexEvent>([&](const ComplexEvent& evt) {
        receivedId = evt.id;
        receivedX = evt.x;
        receivedY = evt.y;
    });
    bus.publish(ComplexEvent{42, 3.14f, 2.71f});
    EXPECT_EQ(receivedId, 42);
    EXPECT_FLOAT_EQ(receivedX, 3.14f);
    EXPECT_FLOAT_EQ(receivedY, 2.71f);
}

TEST(EventBusTest, StringEventData) {
    core::EventBus bus;
    std::string receivedMessage;

    bus.subscribe<AnotherTestEvent>([&](const AnotherTestEvent& evt) {
        receivedMessage = evt.message;
    });
    bus.publish(AnotherTestEvent{"Hello, EventBus!"});
    EXPECT_EQ(receivedMessage, "Hello, EventBus!");
}

TEST(EventBusTest, GetDeferredEventCount) {
    core::EventBus bus;

    EXPECT_EQ(bus.getDeferredEventCount(), 0);
    bus.publish_deferred(TestEvent{1});
    EXPECT_EQ(bus.getDeferredEventCount(), 1);
    bus.publish_deferred(TestEvent{2});
    bus.publish_deferred(TestEvent{3});
    EXPECT_EQ(bus.getDeferredEventCount(), 3);
    bus.process_deferred();
    EXPECT_EQ(bus.getDeferredEventCount(), 0);
}

TEST(EventBusTest, SubscribeAfterPublishDeferred) {
    core::EventBus bus;
    int callCount = 0;

    bus.publish_deferred(TestEvent{1});
    bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount++;
    });
    bus.process_deferred();
    EXPECT_EQ(callCount, 1);
}

TEST(EventBusTest, MultipleProcessDeferredCallsAreIdempotent) {
    core::EventBus bus;
    int callCount = 0;

    bus.subscribe<TestEvent>([&](const TestEvent&) {
        callCount++;
    });
    bus.publish_deferred(TestEvent{1});
    bus.process_deferred();
    EXPECT_EQ(callCount, 1);
    bus.process_deferred();
    EXPECT_EQ(callCount, 1);
    bus.process_deferred();
    EXPECT_EQ(callCount, 1);
}

TEST(EventBusTest, GameLikeScenario) {
    core::EventBus bus;

    int totalScore = 0;
    bus.subscribe<TestEvent>([&](const TestEvent& evt) {
        totalScore += evt.value;
    });
    std::vector<std::string> soundsPlayed;
    bus.subscribe<AnotherTestEvent>([&](const AnotherTestEvent& evt) {
        soundsPlayed.push_back(evt.message);
    });
    bus.publish(TestEvent{100});
    bus.publish_deferred(AnotherTestEvent{"explosion.wav"});
    bus.publish(TestEvent{50});
    bus.publish_deferred(AnotherTestEvent{"coin.wav"});
    EXPECT_EQ(totalScore, 150);
    EXPECT_EQ(soundsPlayed.size(), 0);
    bus.process_deferred();
    EXPECT_EQ(totalScore, 150);
    ASSERT_EQ(soundsPlayed.size(), 2);
    EXPECT_EQ(soundsPlayed[0], "explosion.wav");
    EXPECT_EQ(soundsPlayed[1], "coin.wav");
}
