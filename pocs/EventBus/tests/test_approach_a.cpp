#include <gtest/gtest.h>
#include "approach_a/AudioEngine.hpp"
#include "approach_a/ScoreManager.hpp"
#include "approach_a/Renderer.hpp"
#include "approach_a/PhysicsEngine.hpp"

using namespace approach_a;

class ApproachATest : public ::testing::Test {
protected:
    void SetUp() override {
        audio = std::make_unique<AudioEngine>();
        score = std::make_unique<ScoreManager>();
        renderer = std::make_unique<Renderer>();
        physics = std::make_unique<PhysicsEngine>(*audio, *score, *renderer);
    }

    void TearDown() override {
        physics.reset();
        renderer.reset();
        score.reset();
        audio.reset();
    }

    std::unique_ptr<AudioEngine> audio;
    std::unique_ptr<ScoreManager> score;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<PhysicsEngine> physics;
};

TEST_F(ApproachATest, SingleCollisionTriggersAllSystems) {
    CollisionData collision(1, Position(100.0f, 200.0f), 100);
    physics->checkCollision(collision);

    EXPECT_EQ(physics->getCollisionCount(), 1);
    EXPECT_EQ(audio->getPlayedSounds().size(), 1);
    EXPECT_EQ(audio->getPlayedSounds()[0], "explosion.wav");
    EXPECT_EQ(score->getTotalScore(), 100);
    EXPECT_EQ(renderer->getParticlePositions().size(), 1);
}

TEST_F(ApproachATest, MultipleCollisionsAccumulate) {
    for (int i = 0; i < 5; ++i) {
        CollisionData collision(i, Position(i * 10.0f, i * 20.0f), 50);
        physics->checkCollision(collision);
    }

    EXPECT_EQ(physics->getCollisionCount(), 5);
    EXPECT_EQ(audio->getPlayedSounds().size(), 5);
    EXPECT_EQ(score->getTotalScore(), 250);
    EXPECT_EQ(renderer->getParticlePositions().size(), 5);
}

TEST_F(ApproachATest, CorrectPositionsPassed) {
    Position pos(123.45f, 678.90f);
    CollisionData collision(1, pos, 100);
    physics->checkCollision(collision);

    const auto& positions = renderer->getParticlePositions();
    ASSERT_EQ(positions.size(), 1);
    EXPECT_FLOAT_EQ(positions[0].x, 123.45f);
    EXPECT_FLOAT_EQ(positions[0].y, 678.90f);
}

TEST_F(ApproachATest, VariablePointsScored) {
    CollisionData collision1(1, Position(0, 0), 100);
    CollisionData collision2(2, Position(0, 0), 250);
    CollisionData collision3(3, Position(0, 0), 50);

    physics->checkCollision(collision1);
    physics->checkCollision(collision2);
    physics->checkCollision(collision3);

    EXPECT_EQ(score->getTotalScore(), 400);

    const auto& history = score->getPointsHistory();
    ASSERT_EQ(history.size(), 3);
    EXPECT_EQ(history[0], 100);
    EXPECT_EQ(history[1], 250);
    EXPECT_EQ(history[2], 50);
}

TEST_F(ApproachATest, ComponentsCanBeReset) {
    CollisionData collision(1, Position(100, 200), 100);
    physics->checkCollision(collision);

    audio->clear();
    score->reset();
    renderer->clear();
    physics->reset();

    EXPECT_EQ(physics->getCollisionCount(), 0);
    EXPECT_EQ(audio->getPlayedSounds().size(), 0);
    EXPECT_EQ(score->getTotalScore(), 0);
    EXPECT_EQ(renderer->getParticlePositions().size(), 0);
}

TEST_F(ApproachATest, IndependentComponentOperation) {
    // Test that components work independently
    audio->playSound("test.wav");
    score->addPoints(50);
    renderer->spawnParticles(Position(1, 2));

    EXPECT_EQ(audio->getPlayedSounds().size(), 1);
    EXPECT_EQ(score->getTotalScore(), 50);
    EXPECT_EQ(renderer->getParticlePositions().size(), 1);
}
