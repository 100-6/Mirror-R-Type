/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_movement_system
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/systems/MovementSystem.hpp"
#include "ecs/events/InputEvents.hpp"
#include "core/event/EventBus.hpp"
#include <cmath>

// Helper function to compare floats with tolerance
bool is_approx(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

// ============================================================================
// MOVEMENT SYSTEM TESTS
// ============================================================================

class MovementSystemTest : public ::testing::Test {
protected:
    Registry registry;
    MovementSystem movementSystem;

    void SetUp() override {
        // Register all necessary components
        registry.register_component<Position>();
        registry.register_component<Velocity>();
        registry.register_component<Controllable>();

        // Initialize the movement system (subscribes to events)
        movementSystem.init(registry);
    }

    void TearDown() override {
        movementSystem.shutdown();
    }

    // Helper to publish a move event and process it
    void publishMoveEvent(Entity entity, float dirX, float dirY) {
        auto& eventBus = registry.get_event_bus();
        eventBus.publish(ecs::PlayerMoveEvent(entity, dirX, dirY));
    }
};

// ============================================================================
// BASIC MOVEMENT TESTS
// ============================================================================

TEST_F(MovementSystemTest, NoMovementWhenNoDirection) {
    // Create entity with all components but no direction
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    publishMoveEvent(entity, 0.0f, 0.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Velocity should be zero with no direction
    EXPECT_FLOAT_EQ(velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

TEST_F(MovementSystemTest, MoveUpOnly) {
    // Create entity moving up
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    publishMoveEvent(entity, 0.0f, -1.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Should move up (negative Y direction) at full speed
    EXPECT_FLOAT_EQ(velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.y, -200.0f);
}

TEST_F(MovementSystemTest, MoveDownOnly) {
    // Create entity moving down
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    publishMoveEvent(entity, 0.0f, 1.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Should move down (positive Y direction) at full speed
    EXPECT_FLOAT_EQ(velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.y, 200.0f);
}

TEST_F(MovementSystemTest, MoveLeftOnly) {
    // Create entity moving left
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    publishMoveEvent(entity, -1.0f, 0.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Should move left (negative X direction) at full speed
    EXPECT_FLOAT_EQ(velocity.x, -200.0f);
    EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

TEST_F(MovementSystemTest, MoveRightOnly) {
    // Create entity moving right
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    publishMoveEvent(entity, 1.0f, 0.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Should move right (positive X direction) at full speed
    EXPECT_FLOAT_EQ(velocity.x, 200.0f);
    EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

// ============================================================================
// DIAGONAL MOVEMENT TESTS (NORMALIZATION)
// ============================================================================

TEST_F(MovementSystemTest, DiagonalMovementIsNormalized_UpRight) {
    // Create entity moving diagonally up-right
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    publishMoveEvent(entity, 1.0f, -1.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Diagonal movement should be normalized
    // Expected: speed / sqrt(2) ≈ 141.42
    float expectedSpeed = 200.0f / std::sqrt(2.0f);

    EXPECT_TRUE(is_approx(velocity.x, expectedSpeed))
        << "Expected X=" << expectedSpeed << ", got X=" << velocity.x;
    EXPECT_TRUE(is_approx(velocity.y, -expectedSpeed))
        << "Expected Y=" << -expectedSpeed << ", got Y=" << velocity.y;

    // Verify magnitude is preserved
    float magnitude = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    EXPECT_TRUE(is_approx(magnitude, 200.0f))
        << "Expected magnitude=200.0, got magnitude=" << magnitude;
}

TEST_F(MovementSystemTest, DiagonalMovementIsNormalized_DownLeft) {
    // Create entity moving diagonally down-left
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    publishMoveEvent(entity, -1.0f, 1.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Diagonal movement should be normalized
    float expectedSpeed = 200.0f / std::sqrt(2.0f);

    EXPECT_TRUE(is_approx(velocity.x, -expectedSpeed))
        << "Expected X=" << -expectedSpeed << ", got X=" << velocity.x;
    EXPECT_TRUE(is_approx(velocity.y, expectedSpeed))
        << "Expected Y=" << expectedSpeed << ", got Y=" << velocity.y;

    // Verify magnitude is preserved
    float magnitude = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    EXPECT_TRUE(is_approx(magnitude, 200.0f))
        << "Expected magnitude=200.0, got magnitude=" << magnitude;
}

TEST_F(MovementSystemTest, DiagonalMovementIsNormalized_UpLeft) {
    // Create entity moving diagonally up-left
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{150.0f});

    publishMoveEvent(entity, -1.0f, -1.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Diagonal movement should be normalized with speed 150
    float expectedSpeed = 150.0f / std::sqrt(2.0f);

    EXPECT_TRUE(is_approx(velocity.x, -expectedSpeed));
    EXPECT_TRUE(is_approx(velocity.y, -expectedSpeed));

    // Verify magnitude is preserved
    float magnitude = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    EXPECT_TRUE(is_approx(magnitude, 150.0f));
}

TEST_F(MovementSystemTest, DiagonalMovementIsNormalized_DownRight) {
    // Create entity moving diagonally down-right
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});
    registry.add_component(entity, Controllable{300.0f});

    publishMoveEvent(entity, 1.0f, 1.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Diagonal movement should be normalized with speed 300
    float expectedSpeed = 300.0f / std::sqrt(2.0f);

    EXPECT_TRUE(is_approx(velocity.x, expectedSpeed));
    EXPECT_TRUE(is_approx(velocity.y, expectedSpeed));

    // Verify magnitude is preserved
    float magnitude = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    EXPECT_TRUE(is_approx(magnitude, 300.0f));
}

// ============================================================================
// DIFFERENT SPEED TESTS
// ============================================================================

TEST_F(MovementSystemTest, DifferentSpeedValues) {
    // Test with speed = 100
    Entity entity1 = registry.spawn_entity();
    registry.add_component(entity1, Position{100.0f, 100.0f});
    registry.add_component(entity1, Velocity{0.0f, 0.0f});
    registry.add_component(entity1, Controllable{100.0f});

    // Test with speed = 500
    Entity entity2 = registry.spawn_entity();
    registry.add_component(entity2, Position{200.0f, 200.0f});
    registry.add_component(entity2, Velocity{0.0f, 0.0f});
    registry.add_component(entity2, Controllable{500.0f});

    publishMoveEvent(entity1, 1.0f, 0.0f);
    publishMoveEvent(entity2, 1.0f, 0.0f);

    auto& velocities = registry.get_components<Velocity>();

    // Entity1 should move at speed 100
    EXPECT_FLOAT_EQ(velocities[entity1].x, 100.0f);
    EXPECT_FLOAT_EQ(velocities[entity1].y, 0.0f);

    // Entity2 should move at speed 500
    EXPECT_FLOAT_EQ(velocities[entity2].x, 500.0f);
    EXPECT_FLOAT_EQ(velocities[entity2].y, 0.0f);
}

// ============================================================================
// MISSING COMPONENTS TESTS
// ============================================================================

TEST_F(MovementSystemTest, EntityWithoutVelocityIsIgnored) {
    // Create entity without Velocity component
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Controllable{200.0f});

    // Should not crash
    EXPECT_NO_THROW(publishMoveEvent(entity, 1.0f, 0.0f));
}

TEST_F(MovementSystemTest, EntityWithoutControllableIsIgnored) {
    // Create entity without Controllable component
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Store initial velocity
    float initialVelX = velocity.x;
    float initialVelY = velocity.y;

    publishMoveEvent(entity, 1.0f, 0.0f);

    // Velocity should not change (entity is ignored)
    EXPECT_FLOAT_EQ(velocity.x, initialVelX);
    EXPECT_FLOAT_EQ(velocity.y, initialVelY);
}

// ============================================================================
// MULTIPLE ENTITIES TEST
// ============================================================================

TEST_F(MovementSystemTest, MultipleEntitiesWithDifferentDirections) {
    // Entity 1: moving right
    Entity entity1 = registry.spawn_entity();
    registry.add_component(entity1, Position{100.0f, 100.0f});
    registry.add_component(entity1, Velocity{0.0f, 0.0f});
    registry.add_component(entity1, Controllable{200.0f});

    // Entity 2: moving up
    Entity entity2 = registry.spawn_entity();
    registry.add_component(entity2, Position{200.0f, 200.0f});
    registry.add_component(entity2, Velocity{0.0f, 0.0f});
    registry.add_component(entity2, Controllable{150.0f});

    // Entity 3: moving diagonally down-left
    Entity entity3 = registry.spawn_entity();
    registry.add_component(entity3, Position{300.0f, 300.0f});
    registry.add_component(entity3, Velocity{0.0f, 0.0f});
    registry.add_component(entity3, Controllable{100.0f});

    publishMoveEvent(entity1, 1.0f, 0.0f);
    publishMoveEvent(entity2, 0.0f, -1.0f);
    publishMoveEvent(entity3, -1.0f, 1.0f);

    auto& velocities = registry.get_components<Velocity>();

    // Entity 1: moving right at 200
    EXPECT_FLOAT_EQ(velocities[entity1].x, 200.0f);
    EXPECT_FLOAT_EQ(velocities[entity1].y, 0.0f);

    // Entity 2: moving up at 150
    EXPECT_FLOAT_EQ(velocities[entity2].x, 0.0f);
    EXPECT_FLOAT_EQ(velocities[entity2].y, -150.0f);

    // Entity 3: moving diagonally down-left at 100 (normalized)
    float expectedSpeed3 = 100.0f / std::sqrt(2.0f);
    EXPECT_TRUE(is_approx(velocities[entity3].x, -expectedSpeed3));
    EXPECT_TRUE(is_approx(velocities[entity3].y, expectedSpeed3));
}

// ============================================================================
// SYSTEM LIFECYCLE TESTS
// ============================================================================

TEST_F(MovementSystemTest, UpdateWithEmptyRegistryDoesNotCrash) {
    // Test that update works with no entities
    Registry emptyRegistry;
    emptyRegistry.register_component<Position>();
    emptyRegistry.register_component<Velocity>();
    emptyRegistry.register_component<Controllable>();

    MovementSystem emptyMovementSystem;
    emptyMovementSystem.init(emptyRegistry);

    EXPECT_NO_THROW(emptyMovementSystem.update(emptyRegistry, 0.016f));

    emptyMovementSystem.shutdown();
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
