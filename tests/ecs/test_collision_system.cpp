/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_collision_system
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/systems/CollisionSystem.hpp"
#include <cmath>

// Helper function to compare floats with tolerance
bool is_approx(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

// ============================================================================
// COLLISION SYSTEM TESTS
// ============================================================================

class CollisionSystemTest : public ::testing::Test {
protected:
    Registry registry;
    CollisionSystem collisionSystem;

    void SetUp() override {
        // Register all necessary components
        registry.register_component<Position>();
        registry.register_component<Collider>();
        registry.register_component<Controllable>();
        registry.register_component<Enemy>();
        registry.register_component<Projectile>();
        registry.register_component<Wall>();
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// ============================================================================
// COLLISION DETECTION TESTS
// ============================================================================

TEST_F(CollisionSystemTest, NoCollisionWhenEntitiesAreSeparated) {
    // Create two entities that do not overlap
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{200.0f, 200.0f}); // Far away
    registry.add_component(player, Collider{10.0f, 10.0f});
    registry.add_component(player, Controllable{});

    Position initialPos = registry.get_components<Position>()[player];

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Position should not change (no collision)
    EXPECT_FLOAT_EQ(initialPos.x, finalPos.x);
    EXPECT_FLOAT_EQ(initialPos.y, finalPos.y);
}

TEST_F(CollisionSystemTest, CollisionDetectedWhenEntitiesOverlap) {
    // Create two entities that overlap
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{95.0f, 105.0f}); // Overlapping
    registry.add_component(player, Collider{10.0f, 10.0f});
    registry.add_component(player, Controllable{});

    Position initialPos = registry.get_components<Position>()[player];

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Position should change (collision resolved)
    EXPECT_TRUE(initialPos.x != finalPos.x || initialPos.y != finalPos.y);
}

// ============================================================================
// COLLISION RESOLUTION TESTS - 4 DIRECTIONS
// ============================================================================

TEST_F(CollisionSystemTest, CollisionFromLeft) {
    // Wall at position 100,100 with size 50x50
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    // Player approaching from the left
    // Player at X=92, right edge at 92+10=102 (2 pixels overlap)
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{92.0f, 120.0f});
    registry.add_component(player, Collider{10.0f, 10.0f});
    registry.add_component(player, Controllable{});

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Should be pushed back to X=90 (right edge at 100)
    EXPECT_TRUE(is_approx(finalPos.x, 90.0f))
        << "Expected X=90.0, got X=" << finalPos.x;
}

TEST_F(CollisionSystemTest, CollisionFromRight) {
    // Wall at position 100,100 with size 50x50 (ends at X=150)
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    // Player approaching from the right
    // Player at X=148, left edge at 148 (2 pixels overlap)
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{148.0f, 120.0f});
    registry.add_component(player, Collider{10.0f, 10.0f});
    registry.add_component(player, Controllable{});

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Should be pushed to X=150 (left edge at wall's right)
    EXPECT_TRUE(is_approx(finalPos.x, 150.0f))
        << "Expected X=150.0, got X=" << finalPos.x;
}

TEST_F(CollisionSystemTest, CollisionFromTop) {
    // Wall at position 100,100 with size 50x50
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    // Player approaching from the top
    // Player at Y=92, bottom edge at 92+10=102 (2 pixels overlap)
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{120.0f, 92.0f});
    registry.add_component(player, Collider{10.0f, 10.0f});
    registry.add_component(player, Controllable{});

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Should be pushed back to Y=90 (bottom edge at 100)
    EXPECT_TRUE(is_approx(finalPos.y, 90.0f))
        << "Expected Y=90.0, got Y=" << finalPos.y;
}

TEST_F(CollisionSystemTest, CollisionFromBottom) {
    // Wall at position 100,100 with size 50x50 (ends at Y=150)
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    // Player approaching from the bottom
    // Player at Y=148, top edge at 148 (2 pixels overlap)
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{120.0f, 148.0f});
    registry.add_component(player, Collider{10.0f, 10.0f});
    registry.add_component(player, Controllable{});

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Should be pushed to Y=150 (top edge at wall's bottom)
    EXPECT_TRUE(is_approx(finalPos.y, 150.0f))
        << "Expected Y=150.0, got Y=" << finalPos.y;
}

// ============================================================================
// PROJECTILE vs ENEMY COLLISION TESTS
// ============================================================================

TEST_F(CollisionSystemTest, ProjectileDestroyEnemy) {
    // Create enemy
    Entity enemy = registry.spawn_entity();
    registry.add_component(enemy, Position{100.0f, 100.0f});
    registry.add_component(enemy, Collider{20.0f, 20.0f});
    registry.add_component(enemy, Enemy{});

    // Create projectile overlapping with enemy
    Entity projectile = registry.spawn_entity();
    registry.add_component(projectile, Position{105.0f, 105.0f});
    registry.add_component(projectile, Collider{5.0f, 5.0f});
    registry.add_component(projectile, Projectile{});

    collisionSystem.update(registry);

    auto& positions = registry.get_components<Position>();

    // Both should be destroyed
    EXPECT_FALSE(positions.has_entity(enemy)) << "Enemy should be destroyed";
    EXPECT_FALSE(positions.has_entity(projectile)) << "Projectile should be destroyed";
}

TEST_F(CollisionSystemTest, ProjectileDoesNotDestroyEnemyWhenSeparated) {
    // Create enemy
    Entity enemy = registry.spawn_entity();
    registry.add_component(enemy, Position{100.0f, 100.0f});
    registry.add_component(enemy, Collider{20.0f, 20.0f});
    registry.add_component(enemy, Enemy{});

    // Create projectile far from enemy
    Entity projectile = registry.spawn_entity();
    registry.add_component(projectile, Position{200.0f, 200.0f});
    registry.add_component(projectile, Collider{5.0f, 5.0f});
    registry.add_component(projectile, Projectile{});

    collisionSystem.update(registry);

    auto& positions = registry.get_components<Position>();

    // Both should still exist
    EXPECT_TRUE(positions.has_entity(enemy)) << "Enemy should still exist";
    EXPECT_TRUE(positions.has_entity(projectile)) << "Projectile should still exist";
}

// ============================================================================
// MULTIPLE COLLISIONS TESTS
// ============================================================================

TEST_F(CollisionSystemTest, MultipleWallsDoNotInterfere) {
    // Create multiple walls
    Entity wall1 = registry.spawn_entity();
    registry.add_component(wall1, Position{100.0f, 100.0f});
    registry.add_component(wall1, Collider{50.0f, 50.0f});
    registry.add_component(wall1, Wall{});

    Entity wall2 = registry.spawn_entity();
    registry.add_component(wall2, Position{200.0f, 100.0f});
    registry.add_component(wall2, Collider{50.0f, 50.0f});
    registry.add_component(wall2, Wall{});

    // Player only colliding with wall1
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{92.0f, 120.0f});
    registry.add_component(player, Collider{10.0f, 10.0f});
    registry.add_component(player, Controllable{});

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Should only be affected by wall1
    EXPECT_TRUE(is_approx(finalPos.x, 90.0f))
        << "Expected X=90.0, got X=" << finalPos.x;
}

TEST_F(CollisionSystemTest, MultipleProjectilesDestroyMultipleEnemies) {
    // Create 2 enemies
    Entity enemy1 = registry.spawn_entity();
    registry.add_component(enemy1, Position{100.0f, 100.0f});
    registry.add_component(enemy1, Collider{20.0f, 20.0f});
    registry.add_component(enemy1, Enemy{});

    Entity enemy2 = registry.spawn_entity();
    registry.add_component(enemy2, Position{200.0f, 200.0f});
    registry.add_component(enemy2, Collider{20.0f, 20.0f});
    registry.add_component(enemy2, Enemy{});

    // Create 2 projectiles, each hitting one enemy
    Entity projectile1 = registry.spawn_entity();
    registry.add_component(projectile1, Position{105.0f, 105.0f});
    registry.add_component(projectile1, Collider{5.0f, 5.0f});
    registry.add_component(projectile1, Projectile{});

    Entity projectile2 = registry.spawn_entity();
    registry.add_component(projectile2, Position{205.0f, 205.0f});
    registry.add_component(projectile2, Collider{5.0f, 5.0f});
    registry.add_component(projectile2, Projectile{});

    collisionSystem.update(registry);

    auto& positions = registry.get_components<Position>();

    // All should be destroyed
    EXPECT_FALSE(positions.has_entity(enemy1)) << "Enemy1 should be destroyed";
    EXPECT_FALSE(positions.has_entity(enemy2)) << "Enemy2 should be destroyed";
    EXPECT_FALSE(positions.has_entity(projectile1)) << "Projectile1 should be destroyed";
    EXPECT_FALSE(positions.has_entity(projectile2)) << "Projectile2 should be destroyed";
}

// ============================================================================
// EDGE CASES TESTS
// ============================================================================

TEST_F(CollisionSystemTest, EntityWithoutColliderIsIgnored) {
    // Create wall
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    // Create player without collider
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{92.0f, 120.0f});
    registry.add_component(player, Controllable{});

    Position initialPos = registry.get_components<Position>()[player];

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Position should not change (no collider)
    EXPECT_FLOAT_EQ(initialPos.x, finalPos.x);
    EXPECT_FLOAT_EQ(initialPos.y, finalPos.y);
}

TEST_F(CollisionSystemTest, ZeroSizeColliderDoesNotCollide) {
    // Create wall
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    // Create player with zero-size collider
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{105.0f, 105.0f});
    registry.add_component(player, Collider{0.0f, 0.0f});
    registry.add_component(player, Controllable{});

    Position initialPos = registry.get_components<Position>()[player];

    collisionSystem.update(registry);

    Position finalPos = registry.get_components<Position>()[player];

    // Position should not change (zero size collider)
    EXPECT_FLOAT_EQ(initialPos.x, finalPos.x);
    EXPECT_FLOAT_EQ(initialPos.y, finalPos.y);
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
