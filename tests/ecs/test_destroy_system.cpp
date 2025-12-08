/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_destroy_system
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/systems/DestroySystem.hpp"

// ============================================================================
// DESTROY SYSTEM TESTS
// ============================================================================

class DestroySystemTest : public ::testing::Test {
protected:
    Registry registry;
    DestroySystem destroySystem;

    void SetUp() override {
        // Register all necessary components
        registry.register_component<Position>();
        registry.register_component<Velocity>();
        registry.register_component<Collider>();
        registry.register_component<Enemy>();
        registry.register_component<Projectile>();
        registry.register_component<ToDestroy>();
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// ============================================================================
// BASIC DESTRUCTION TESTS
// ============================================================================

TEST_F(DestroySystemTest, EntityWithToDestroyIsDestroyed) {
    // Create an entity
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Enemy{});

    // Mark it for destruction
    registry.add_component(entity, ToDestroy{});

    auto& positions = registry.get_components<Position>();
    EXPECT_TRUE(positions.has_entity(entity)) << "Entity should exist before destruction";

    // Run destroy system
    destroySystem.update(registry, 0.016f);

    // Entity should be destroyed
    EXPECT_FALSE(positions.has_entity(entity)) << "Entity should be destroyed";
}

TEST_F(DestroySystemTest, EntityWithoutToDestroyIsNotDestroyed) {
    // Create an entity without ToDestroy
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Enemy{});

    auto& positions = registry.get_components<Position>();
    EXPECT_TRUE(positions.has_entity(entity));

    // Run destroy system
    destroySystem.update(registry, 0.016f);

    // Entity should still exist
    EXPECT_TRUE(positions.has_entity(entity)) << "Entity without ToDestroy should not be destroyed";
}

// ============================================================================
// MULTIPLE ENTITIES TESTS
// ============================================================================

TEST_F(DestroySystemTest, MultipleEntitiesMarkedAreDestroyed) {
    // Create 3 entities
    Entity entity1 = registry.spawn_entity();
    registry.add_component(entity1, Position{100.0f, 100.0f});
    registry.add_component(entity1, Enemy{});
    registry.add_component(entity1, ToDestroy{});

    Entity entity2 = registry.spawn_entity();
    registry.add_component(entity2, Position{200.0f, 200.0f});
    registry.add_component(entity2, Projectile{});
    registry.add_component(entity2, ToDestroy{});

    Entity entity3 = registry.spawn_entity();
    registry.add_component(entity3, Position{300.0f, 300.0f});
    registry.add_component(entity3, Enemy{});
    registry.add_component(entity3, ToDestroy{});

    // Run destroy system
    destroySystem.update(registry, 0.016f);

    // All should be destroyed
    auto& positions = registry.get_components<Position>();
    EXPECT_FALSE(positions.has_entity(entity1)) << "Entity1 should be destroyed";
    EXPECT_FALSE(positions.has_entity(entity2)) << "Entity2 should be destroyed";
    EXPECT_FALSE(positions.has_entity(entity3)) << "Entity3 should be destroyed";
}

TEST_F(DestroySystemTest, OnlySomeEntitiesMarkedAreDestroyed) {
    // Create 3 entities, only 2 marked for destruction
    Entity entity1 = registry.spawn_entity();
    registry.add_component(entity1, Position{100.0f, 100.0f});
    registry.add_component(entity1, Enemy{});
    registry.add_component(entity1, ToDestroy{});

    Entity entity2 = registry.spawn_entity();
    registry.add_component(entity2, Position{200.0f, 200.0f});
    registry.add_component(entity2, Projectile{});
    // Not marked for destruction

    Entity entity3 = registry.spawn_entity();
    registry.add_component(entity3, Position{300.0f, 300.0f});
    registry.add_component(entity3, Enemy{});
    registry.add_component(entity3, ToDestroy{});

    // Run destroy system
    destroySystem.update(registry, 0.016f);

    // Check results
    auto& positions = registry.get_components<Position>();
    EXPECT_FALSE(positions.has_entity(entity1)) << "Entity1 should be destroyed";
    EXPECT_TRUE(positions.has_entity(entity2)) << "Entity2 should still exist";
    EXPECT_FALSE(positions.has_entity(entity3)) << "Entity3 should be destroyed";
}

// ============================================================================
// COMPONENT REMOVAL TESTS
// ============================================================================

TEST_F(DestroySystemTest, AllComponentsAreRemovedWhenEntityDestroyed) {
    // Create an entity with multiple components
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, Velocity{50.0f, 50.0f});
    registry.add_component(entity, Collider{20.0f, 20.0f});
    registry.add_component(entity, Enemy{});
    registry.add_component(entity, ToDestroy{});

    // Verify all components exist
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& colliders = registry.get_components<Collider>();
    auto& enemies = registry.get_components<Enemy>();

    EXPECT_TRUE(positions.has_entity(entity));
    EXPECT_TRUE(velocities.has_entity(entity));
    EXPECT_TRUE(colliders.has_entity(entity));
    EXPECT_TRUE(enemies.has_entity(entity));

    // Run destroy system
    destroySystem.update(registry, 0.016f);

    // All components should be removed
    EXPECT_FALSE(positions.has_entity(entity)) << "Position should be removed";
    EXPECT_FALSE(velocities.has_entity(entity)) << "Velocity should be removed";
    EXPECT_FALSE(colliders.has_entity(entity)) << "Collider should be removed";
    EXPECT_FALSE(enemies.has_entity(entity)) << "Enemy should be removed";
}

// ============================================================================
// INTEGRATION TEST WITH COLLISION SYSTEM
// ============================================================================

TEST_F(DestroySystemTest, WorksWithCollisionSystemMarking) {
    // Simulate what CollisionSystem does: mark entities with ToDestroy
    Entity projectile = registry.spawn_entity();
    registry.add_component(projectile, Position{100.0f, 100.0f});
    registry.add_component(projectile, Collider{5.0f, 5.0f});
    registry.add_component(projectile, Projectile{});

    Entity enemy = registry.spawn_entity();
    registry.add_component(enemy, Position{102.0f, 102.0f});
    registry.add_component(enemy, Collider{20.0f, 20.0f});
    registry.add_component(enemy, Enemy{});

    // Simulate collision detection: mark both for destruction
    registry.add_component(projectile, ToDestroy{});
    registry.add_component(enemy, ToDestroy{});

    auto& positions = registry.get_components<Position>();
    EXPECT_TRUE(positions.has_entity(projectile));
    EXPECT_TRUE(positions.has_entity(enemy));

    // Run destroy system
    destroySystem.update(registry, 0.016f);

    // Both should be destroyed
    EXPECT_FALSE(positions.has_entity(projectile)) << "Projectile should be destroyed";
    EXPECT_FALSE(positions.has_entity(enemy)) << "Enemy should be destroyed";
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_F(DestroySystemTest, EmptyRegistryDoesNotCrash) {
    // Run destroy system on empty registry
    EXPECT_NO_THROW(destroySystem.update(registry, 0.016f));
}

TEST_F(DestroySystemTest, RunningTwiceDoesNotCrash) {
    Entity entity = registry.spawn_entity();
    registry.add_component(entity, Position{100.0f, 100.0f});
    registry.add_component(entity, ToDestroy{});

    // Run twice
    destroySystem.update(registry, 0.016f);
    EXPECT_NO_THROW(destroySystem.update(registry, 0.016f));
}

TEST_F(DestroySystemTest, DestroyingEntityDoesNotAffectOthers) {
    // Create 10 entities, mark only one for destruction
    std::vector<Entity> entities;
    for (int i = 0; i < 10; i++) {
        Entity e = registry.spawn_entity();
        registry.add_component(e, Position{float(i * 10), float(i * 10)});
        entities.push_back(e);
    }

    // Mark only the 5th entity for destruction
    registry.add_component(entities[5], ToDestroy{});

    // Run destroy system
    destroySystem.update(registry, 0.016f);

    // Check results
    auto& positions = registry.get_components<Position>();
    for (size_t i = 0; i < entities.size(); i++) {
        if (i == 5) {
            EXPECT_FALSE(positions.has_entity(entities[i])) << "Entity 5 should be destroyed";
        } else {
            EXPECT_TRUE(positions.has_entity(entities[i])) << "Entity " << i << " should still exist";
        }
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
