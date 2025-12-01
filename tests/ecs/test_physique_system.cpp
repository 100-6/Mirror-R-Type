/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_physique_system
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/systems/PhysiqueSystem.hpp"
#include <cmath>

// Helper function to compare floats with tolerance
bool is_approx(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

// ============================================================================
// PHYSIQUE SYSTEM TESTS
// ============================================================================

class PhysiqueSystemTest : public ::testing::Test {
protected:
    Registry registry;
    PhysiqueSystem physiqueSystem;

    void SetUp() override {
        // Register all necessary components
        registry.register_component<Position>();
        registry.register_component<Velocity>();
        registry.register_component<Controllable>();
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// ============================================================================
// BASIC POSITION UPDATE TESTS
// ============================================================================

TEST_F(PhysiqueSystemTest, PositionUpdatesWithVelocity) {
    // Create entity with velocity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{50.0f, 30.0f});

    // dt = 1.0 second
    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should be updated by velocity * dt
    // After friction: vel *= 0.98, so movement = 50 * 0.98 * 1.0 = 49
    EXPECT_TRUE(is_approx(position.x, 100.0f + 50.0f, 0.1f));
    EXPECT_TRUE(is_approx(position.y, 100.0f + 30.0f, 0.1f));
}

TEST_F(PhysiqueSystemTest, PositionUpdatesWithDeltaTime) {
    // Create entity with velocity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{0.0f, 0.0f});
    registry.add_component(entity, Velocity{100.0f, 50.0f});

    // dt = 0.5 seconds
    physiqueSystem.update(registry, 0.5f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should be updated: pos += vel * dt
    // x: 0 + 100 * 0.5 = 50
    // y: 0 + 50 * 0.5 = 25
    EXPECT_TRUE(is_approx(position.x, 50.0f, 0.1f));
    EXPECT_TRUE(is_approx(position.y, 25.0f, 0.1f));
}

TEST_F(PhysiqueSystemTest, NegativeVelocityMovesBackward) {
    // Create entity with negative velocity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{200.0f, 200.0f});
    registry.add_component(entity, Velocity{-100.0f, -50.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should decrease
    EXPECT_TRUE(is_approx(position.x, 100.0f, 0.1f));
    EXPECT_TRUE(is_approx(position.y, 150.0f, 0.1f));
}

TEST_F(PhysiqueSystemTest, ZeroVelocityNoMovement) {
    // Create entity with zero velocity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{0.0f, 0.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should not change
    EXPECT_FLOAT_EQ(position.x, 100.0f);
    EXPECT_FLOAT_EQ(position.y, 100.0f);
}

// ============================================================================
// FRICTION TESTS
// ============================================================================

TEST_F(PhysiqueSystemTest, FrictionReducesVelocity) {
    // Create entity with velocity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{0.0f, 0.0f});
    registry.add_component(entity, Velocity{100.0f, 100.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& velocity = registry.get_components<Velocity>()[entity];

    // Velocity should be reduced by friction (0.98)
    EXPECT_TRUE(is_approx(velocity.x, 98.0f, 0.1f));
    EXPECT_TRUE(is_approx(velocity.y, 98.0f, 0.1f));
}

TEST_F(PhysiqueSystemTest, FrictionAppliesOverMultipleFrames) {
    // Create entity with velocity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{0.0f, 0.0f});
    registry.add_component(entity, Velocity{100.0f, 0.0f});

    // Update multiple times
    for (int i = 0; i < 10; i++) {
        physiqueSystem.update(registry, 0.016f);
    }

    auto& velocity = registry.get_components<Velocity>()[entity];

    // After 10 frames: 100 * (0.98^10) ≈ 81.7
    float expectedVel = 100.0f * std::pow(0.98f, 10.0f);
    EXPECT_TRUE(is_approx(velocity.x, expectedVel, 0.5f));
}

TEST_F(PhysiqueSystemTest, FrictionEventuallySlowsToNearZero) {
    // Create entity with velocity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{0.0f, 0.0f});
    registry.add_component(entity, Velocity{100.0f, 100.0f});

    // Update many times
    for (int i = 0; i < 1000; i++) {
        physiqueSystem.update(registry, 0.016f);
    }

    auto& velocity = registry.get_components<Velocity>()[entity];

    // After many frames, velocity should be very small
    EXPECT_TRUE(std::abs(velocity.x) < 1.0f);
    EXPECT_TRUE(std::abs(velocity.y) < 1.0f);
}

// ============================================================================
// BOUNDARY TESTS (CONTROLLABLE ENTITIES)
// ============================================================================

TEST_F(PhysiqueSystemTest, ControllableEntityClampedAtLeftBoundary) {
    // Create controllable entity moving left
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{10.0f, 100.0f});
    registry.add_component(entity, Velocity{-50.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should be clamped to 0
    EXPECT_FLOAT_EQ(position.x, 0.0f);
    EXPECT_FLOAT_EQ(position.y, 100.0f);
}

TEST_F(PhysiqueSystemTest, ControllableEntityClampedAtRightBoundary) {
    // Create controllable entity moving right
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{1900.0f, 100.0f});
    registry.add_component(entity, Velocity{100.0f, 0.0f});
    registry.add_component(entity, Controllable{200.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should be clamped to SCREEN_WIDTH (1920)
    EXPECT_FLOAT_EQ(position.x, 1920.0f);
}

TEST_F(PhysiqueSystemTest, ControllableEntityClampedAtTopBoundary) {
    // Create controllable entity moving up
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 10.0f});
    registry.add_component(entity, Velocity{0.0f, -50.0f});
    registry.add_component(entity, Controllable{200.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should be clamped to 0
    EXPECT_FLOAT_EQ(position.x, 100.0f);
    EXPECT_FLOAT_EQ(position.y, 0.0f);
}

TEST_F(PhysiqueSystemTest, ControllableEntityClampedAtBottomBoundary) {
    // Create controllable entity moving down
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 1060.0f});
    registry.add_component(entity, Velocity{0.0f, 100.0f});
    registry.add_component(entity, Controllable{200.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should be clamped to SCREEN_HEIGHT (1080)
    EXPECT_FLOAT_EQ(position.y, 1080.0f);
}

TEST_F(PhysiqueSystemTest, ControllableEntityStaysWithinBounds) {
    // Create controllable entity in the middle
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{500.0f, 500.0f});
    registry.add_component(entity, Velocity{10.0f, 10.0f});
    registry.add_component(entity, Controllable{200.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should move normally (not clamped)
    EXPECT_TRUE(position.x > 500.0f && position.x < 1920.0f);
    EXPECT_TRUE(position.y > 500.0f && position.y < 1080.0f);
}

// ============================================================================
// NON-CONTROLLABLE ENTITIES (NO BOUNDARY CLAMPING)
// ============================================================================

TEST_F(PhysiqueSystemTest, NonControllableEntityCanExceedLeftBoundary) {
    // Create non-controllable entity moving left
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{10.0f, 100.0f});
    registry.add_component(entity, Velocity{-50.0f, 0.0f});
    // No Controllable component!

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should go negative (not clamped)
    EXPECT_TRUE(position.x < 0.0f);
}

TEST_F(PhysiqueSystemTest, NonControllableEntityCanExceedRightBoundary) {
    // Create non-controllable entity moving right
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{1900.0f, 100.0f});
    registry.add_component(entity, Velocity{100.0f, 0.0f});
    // No Controllable component!

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should exceed SCREEN_WIDTH (not clamped)
    EXPECT_TRUE(position.x > 1920.0f);
}

TEST_F(PhysiqueSystemTest, NonControllableEntityCanExceedTopBoundary) {
    // Create non-controllable entity moving up
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 10.0f});
    registry.add_component(entity, Velocity{0.0f, -50.0f});
    // No Controllable component!

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should go negative (not clamped)
    EXPECT_TRUE(position.y < 0.0f);
}

TEST_F(PhysiqueSystemTest, NonControllableEntityCanExceedBottomBoundary) {
    // Create non-controllable entity moving down
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 1060.0f});
    registry.add_component(entity, Velocity{0.0f, 100.0f});
    // No Controllable component!

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should exceed SCREEN_HEIGHT (not clamped)
    EXPECT_TRUE(position.y > 1080.0f);
}

// ============================================================================
// MISSING COMPONENTS TESTS
// ============================================================================

TEST_F(PhysiqueSystemTest, EntityWithoutPositionIsIgnored) {
    // Create entity without Position component
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Velocity{100.0f, 100.0f});

    // Should not crash
    EXPECT_NO_THROW(physiqueSystem.update(registry, 0.016f));
}

TEST_F(PhysiqueSystemTest, EntityWithVelocityButNoPositionDoesNotCrash) {
    // Create multiple entities, one missing position
    Entity entity1 = registry.spawn_entity();
    registry.add_component(entity1, Position{100.0f, 100.0f});
    registry.add_component(entity1, Velocity{10.0f, 10.0f});

    Entity entity2 = registry.spawn_entity();
    registry.add_component(entity2, Velocity{20.0f, 20.0f});
    // No position for entity2

    // Should not crash, entity1 should update normally
    EXPECT_NO_THROW(physiqueSystem.update(registry, 1.0f));

    auto& position1 = registry.get_components<Position>()[entity1];
    EXPECT_TRUE(position1.x > 100.0f);
}

// ============================================================================
// MULTIPLE ENTITIES TESTS
// ============================================================================

TEST_F(PhysiqueSystemTest, MultipleEntitiesUpdateIndependently) {
    // Entity 1: moving right
    Entity entity1 = registry.spawn_entity();
    registry.add_component(entity1, Position{100.0f, 100.0f});
    registry.add_component(entity1, Velocity{50.0f, 0.0f});

    // Entity 2: moving down
    Entity entity2 = registry.spawn_entity();
    registry.add_component(entity2, Position{200.0f, 200.0f});
    registry.add_component(entity2, Velocity{0.0f, 30.0f});

    // Entity 3: moving diagonally
    Entity entity3 = registry.spawn_entity();
    registry.add_component(entity3, Position{300.0f, 300.0f});
    registry.add_component(entity3, Velocity{-20.0f, -40.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& positions = registry.get_components<Position>();

    // Each entity should update independently
    EXPECT_TRUE(is_approx(positions[entity1].x, 150.0f, 0.5f));
    EXPECT_TRUE(is_approx(positions[entity1].y, 100.0f, 0.5f));

    EXPECT_TRUE(is_approx(positions[entity2].x, 200.0f, 0.5f));
    EXPECT_TRUE(is_approx(positions[entity2].y, 230.0f, 0.5f));

    EXPECT_TRUE(is_approx(positions[entity3].x, 280.0f, 0.5f));
    EXPECT_TRUE(is_approx(positions[entity3].y, 260.0f, 0.5f));
}

TEST_F(PhysiqueSystemTest, MixedControllableAndNonControllableEntities) {
    // Controllable entity at boundary
    Entity controllable = registry.spawn_entity();
    registry.add_component(controllable, Position{10.0f, 10.0f});
    registry.add_component(controllable, Velocity{-50.0f, -50.0f});
    registry.add_component(controllable, Controllable{200.0f});

    // Non-controllable entity at same position
    Entity nonControllable = registry.spawn_entity();
    registry.add_component(nonControllable, Position{10.0f, 10.0f});
    registry.add_component(nonControllable, Velocity{-50.0f, -50.0f});

    physiqueSystem.update(registry, 1.0f);

    auto& positions = registry.get_components<Position>();

    // Controllable should be clamped to 0
    EXPECT_FLOAT_EQ(positions[controllable].x, 0.0f);
    EXPECT_FLOAT_EQ(positions[controllable].y, 0.0f);

    // Non-controllable should go negative
    EXPECT_TRUE(positions[nonControllable].x < 0.0f);
    EXPECT_TRUE(positions[nonControllable].y < 0.0f);
}

// ============================================================================
// INTEGRATION TESTS (POSITION + FRICTION + BOUNDARIES)
// ============================================================================

TEST_F(PhysiqueSystemTest, FullUpdateCycle) {
    // Create controllable entity with velocity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{100.0f, 50.0f});
    registry.add_component(entity, Controllable{200.0f});

    float initialPosX = 100.0f;
    float initialPosY = 100.0f;
    float velX = 100.0f;
    float velY = 50.0f;

    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];
    auto& velocity = registry.get_components<Velocity>()[entity];

    // Position should update first: pos += vel * dt
    // Then velocity applies friction: vel *= 0.98
    EXPECT_TRUE(is_approx(position.x, initialPosX + velX * 1.0f, 0.5f));
    EXPECT_TRUE(is_approx(position.y, initialPosY + velY * 1.0f, 0.5f));

    EXPECT_TRUE(is_approx(velocity.x, velX * 0.98f, 0.5f));
    EXPECT_TRUE(is_approx(velocity.y, velY * 0.98f, 0.5f));
}

TEST_F(PhysiqueSystemTest, SimulateProjectileOffScreen) {
    // Simulate a projectile (enemy bullet) moving off screen
    Entity projectile = registry.spawn_entity();
    registry.add_component(projectile, Position{1900.0f, 500.0f});
    registry.add_component(projectile, Velocity{500.0f, 0.0f});
    // No Controllable - it's a projectile

    // Update multiple times with larger dt to simulate movement
    for (int i = 0; i < 3; i++) {
        physiqueSystem.update(registry, 0.1f);
    }

    auto& position = registry.get_components<Position>()[projectile];

    // Projectile should have moved far off screen
    // Starting at 1900, with velocity ~500, dt=0.1, over 3 frames
    // Should move approximately 500*0.1 + 490*0.1 + 480.2*0.1 ≈ 147 pixels
    EXPECT_TRUE(position.x > 1920.0f);
}

// ============================================================================
// SYSTEM LIFECYCLE TESTS
// ============================================================================

TEST_F(PhysiqueSystemTest, InitAndShutdownDoNotCrash) {
    // Test that init and shutdown work correctly
    EXPECT_NO_THROW(physiqueSystem.init(registry));
    EXPECT_NO_THROW(physiqueSystem.shutdown());
}

TEST_F(PhysiqueSystemTest, UpdateWithEmptyRegistryDoesNotCrash) {
    // Test that update works with no entities
    Registry emptyRegistry;
    emptyRegistry.register_component<Position>();
    emptyRegistry.register_component<Velocity>();
    emptyRegistry.register_component<Controllable>();

    EXPECT_NO_THROW(physiqueSystem.update(emptyRegistry, 0.016f));
}

TEST_F(PhysiqueSystemTest, MultipleUpdatesInSequence) {
    // Create entity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{0.0f, 0.0f});
    registry.add_component(entity, Velocity{10.0f, 10.0f});

    // Update 100 times
    for (int i = 0; i < 100; i++) {
        EXPECT_NO_THROW(physiqueSystem.update(registry, 0.016f));
    }

    // Entity should have moved significantly
    auto& position = registry.get_components<Position>()[entity];
    EXPECT_TRUE(position.x > 0.0f);
    EXPECT_TRUE(position.y > 0.0f);
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_F(PhysiqueSystemTest, VerySmallDeltaTime) {
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{100.0f, 100.0f});

    // Very small dt
    physiqueSystem.update(registry, 0.001f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should move very slightly
    EXPECT_TRUE(is_approx(position.x, 100.1f, 0.01f));
    EXPECT_TRUE(is_approx(position.y, 100.1f, 0.01f));
}

TEST_F(PhysiqueSystemTest, VeryLargeDeltaTime) {
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{10.0f, 10.0f});

    // Large dt (1 second)
    physiqueSystem.update(registry, 1.0f);

    auto& position = registry.get_components<Position>()[entity];

    // Position should move significantly
    EXPECT_TRUE(position.x > 105.0f);
    EXPECT_TRUE(position.y > 105.0f);
}

TEST_F(PhysiqueSystemTest, ZeroDeltaTime) {
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{100.0f, 100.0f});

    float initialVelX = 100.0f;
    float initialVelY = 100.0f;

    // dt = 0
    physiqueSystem.update(registry, 0.0f);

    auto& position = registry.get_components<Position>()[entity];
    auto& velocity = registry.get_components<Velocity>()[entity];

    // Position should not change (vel * 0 = 0)
    EXPECT_FLOAT_EQ(position.x, 100.0f);
    EXPECT_FLOAT_EQ(position.y, 100.0f);

    // Friction still applies
    EXPECT_TRUE(is_approx(velocity.x, initialVelX * 0.98f, 0.1f));
    EXPECT_TRUE(is_approx(velocity.y, initialVelY * 0.98f, 0.1f));
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
