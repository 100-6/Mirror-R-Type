#include <gtest/gtest.h>
#include "approach_b/EventBus.hpp"
#include "approach_b/AudioEngine.hpp"
#include "approach_b/ScoreManager.hpp"
#include "approach_b/Renderer.hpp"
#include "approach_b/PhysicsEngine.hpp"

using namespace approach_b;

class ApproachBTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventBus = std::make_unique<EventBus>();
        audio = std::make_unique<AudioEngine>(*eventBus);
        score = std::make_unique<ScoreManager>(*eventBus);
        renderer = std::make_unique<Renderer>(*eventBus);
        physics = std::make_unique<PhysicsEngine>(*eventBus);
    }

    void TearDown() override {
        physics.reset();
        renderer.reset();
        score.reset();
        audio.reset();
        eventBus.reset();
    }

    std::unique_ptr<EventBus> eventBus;
    std::unique_ptr<AudioEngine> audio;
    std::unique_ptr<ScoreManager> score;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<PhysicsEngine> physics;
};

TEST_F(ApproachBTest, SingleCollisionTriggersAllSystems) {
    physics->checkCollision(1, Position(100.0f, 200.0f), 100);

    EXPECT_EQ(physics->getCollisionCount(), 1);
    EXPECT_EQ(eventBus->getEventCount(), 1);
    EXPECT_EQ(audio->getPlayedSounds().size(), 1);
    EXPECT_EQ(audio->getPlayedSounds()[0], "explosion.wav");
    EXPECT_EQ(score->getTotalScore(), 100);
    EXPECT_EQ(renderer->getParticlePositions().size(), 1);
}

TEST_F(ApproachBTest, MultipleCollisionsAccumulate) {
    for (int i = 0; i < 5; ++i) {
        physics->checkCollision(i, Position(i * 10.0f, i * 20.0f), 50);
    }

    EXPECT_EQ(physics->getCollisionCount(), 5);
    EXPECT_EQ(eventBus->getEventCount(), 5);
    EXPECT_EQ(audio->getPlayedSounds().size(), 5);
    EXPECT_EQ(score->getTotalScore(), 250);
    EXPECT_EQ(renderer->getParticlePositions().size(), 5);
}

TEST_F(ApproachBTest, CorrectPositionsPassed) {
    Position pos(123.45f, 678.90f);
    physics->checkCollision(1, pos, 100);

    const auto& positions = renderer->getParticlePositions();
    ASSERT_EQ(positions.size(), 1);
    EXPECT_FLOAT_EQ(positions[0].x, 123.45f);
    EXPECT_FLOAT_EQ(positions[0].y, 678.90f);
}

TEST_F(ApproachBTest, VariablePointsScored) {
    physics->checkCollision(1, Position(0, 0), 100);
    physics->checkCollision(2, Position(0, 0), 250);
    physics->checkCollision(3, Position(0, 0), 50);

    EXPECT_EQ(score->getTotalScore(), 400);

    const auto& history = score->getPointsHistory();
    ASSERT_EQ(history.size(), 3);
    EXPECT_EQ(history[0], 100);
    EXPECT_EQ(history[1], 250);
    EXPECT_EQ(history[2], 50);
}

TEST_F(ApproachBTest, ComponentsCanBeReset) {
    physics->checkCollision(1, Position(100, 200), 100);

    audio->clear();
    score->reset();
    renderer->clear();
    physics->reset();
    eventBus->reset();

    EXPECT_EQ(physics->getCollisionCount(), 0);
    EXPECT_EQ(eventBus->getEventCount(), 0);
    EXPECT_EQ(audio->getPlayedSounds().size(), 0);
    EXPECT_EQ(score->getTotalScore(), 0);
    EXPECT_EQ(renderer->getParticlePositions().size(), 0);
}

TEST_F(ApproachBTest, SubscriberCountCorrect) {
    EXPECT_EQ(eventBus->getSubscriberCount<EnemyDestroyedEvent>(), 3);
}

TEST_F(ApproachBTest, DynamicSubscription) {
    int callCount = 0;
    auto subId = eventBus->subscribe<EnemyDestroyedEvent>(
        [&callCount](const EnemyDestroyedEvent&) {
            callCount++;
        }
    );

    EXPECT_EQ(eventBus->getSubscriberCount<EnemyDestroyedEvent>(), 4);

    physics->checkCollision(1, Position(0, 0), 100);
    EXPECT_EQ(callCount, 1);

    eventBus->unsubscribe(subId);
    EXPECT_EQ(eventBus->getSubscriberCount<EnemyDestroyedEvent>(), 3);

    physics->checkCollision(2, Position(0, 0), 100);
    EXPECT_EQ(callCount, 1); // Should not increment
}

TEST_F(ApproachBTest, EventBusWithoutSubscribers) {
    EventBus bus;
    PhysicsEngine p(bus);

    p.checkCollision(1, Position(0, 0), 100);
    EXPECT_EQ(bus.getEventCount(), 1);
}

TEST_F(ApproachBTest, SelectiveSubscription) {
    EventBus bus;
    ScoreManager s(bus);
    PhysicsEngine p(bus);

    p.checkCollision(1, Position(0, 0), 100);

    EXPECT_EQ(s.getTotalScore(), 100);
    EXPECT_EQ(bus.getEventCount(), 1);
}

TEST_F(ApproachBTest, ComponentDecoupling) {
    {
        AudioEngine tempAudio(*eventBus);
        physics->checkCollision(1, Position(0, 0), 100);
        EXPECT_EQ(tempAudio.getPlayedSounds().size(), 1);
    }

    physics->checkCollision(2, Position(0, 0), 100);
    EXPECT_EQ(physics->getCollisionCount(), 2);
    EXPECT_EQ(score->getTotalScore(), 200);
}
