/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_shooting_system
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/systems/ShootingSystem.hpp"
#include "ecs/events/InputEvents.hpp"

// ============================================================================
// SHOOTING SYSTEM TESTS
// ============================================================================

class ShootingSystemTest : public ::testing::Test {
protected:
    Registry registry;
    ShootingSystem* shootingSystem;
    engine::TextureHandle bulletTex = 1;
    float bulletWidth = 10.0f;
    float bulletHeight = 5.0f;

    void SetUp() override {
        // Register all necessary components
        registry.register_component<Position>();
        registry.register_component<Velocity>();
        registry.register_component<Collider>();
        registry.register_component<Sprite>();
        registry.register_component<Projectile>();
        registry.register_component<FireRate>();
        registry.register_component<Controllable>();

        // Create shooting system
        shootingSystem = new ShootingSystem(bulletTex, bulletWidth, bulletHeight);
        shootingSystem->init(registry);
    }

    void TearDown() override {
        delete shootingSystem;
    }
};

// ============================================================================
// BASIC SHOOTING TESTS
// ============================================================================

TEST_F(ShootingSystemTest, PlayerCanShootWhenCooldownReady) {
    // Create a player entity
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, FireRate{0.5f, 999.0f}); // Ready to shoot
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), 0);

    // Publish fire event
    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerFireEvent{player});

    // Check that a projectile was created
    EXPECT_EQ(projectiles.size(), 1);
}

TEST_F(ShootingSystemTest, PlayerCannotShootWhenCooldownNotReady) {
    // Create a player entity
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, FireRate{0.5f, 0.0f}); // Just shot, cooldown = 0
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), 0);

    // Publish fire event
    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerFireEvent{player});

    // Check that NO projectile was created
    EXPECT_EQ(projectiles.size(), 0);
}

TEST_F(ShootingSystemTest, ProjectileHasCorrectComponents) {
    // Create a player entity
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, FireRate{0.5f, 999.0f});
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    // Publish fire event
    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerFireEvent{player});

    // Get components
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& colliders = registry.get_components<Collider>();
    auto& sprites = registry.get_components<Sprite>();
    auto& projectiles = registry.get_components<Projectile>();

    ASSERT_EQ(projectiles.size(), 1);
    Entity projectile = projectiles.get_entity_at(0);

    // Check that projectile has all necessary components
    EXPECT_TRUE(positions.has_entity(projectile));
    EXPECT_TRUE(velocities.has_entity(projectile));
    EXPECT_TRUE(colliders.has_entity(projectile));
    EXPECT_TRUE(sprites.has_entity(projectile));
    EXPECT_TRUE(projectiles.has_entity(projectile));
}

TEST_F(ShootingSystemTest, ProjectilePositionIsCentered) {
    // Create a player entity with known dimensions
    Entity player = registry.spawn_entity();
    float playerHeight = 64.0f;
    registry.add_component(player, Position{100.0f, 200.0f});
    registry.add_component(player, FireRate{0.5f, 999.0f});
    registry.add_component(player, Sprite{bulletTex, 128.0f, playerHeight, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    // Publish fire event
    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerFireEvent{player});

    // Get projectile position
    auto& positions = registry.get_components<Position>();
    auto& projectiles = registry.get_components<Projectile>();

    ASSERT_EQ(projectiles.size(), 1);
    Entity projectile = projectiles.get_entity_at(0);

    const Position& projPos = positions[projectile];

    // Expected Y position should be centered
    float expectedY = 200.0f + (playerHeight / 2.0f) - (bulletHeight / 2.0f);

    EXPECT_FLOAT_EQ(projPos.x, 100.0f + 50.0f); // playerX + bulletOffsetX
    EXPECT_FLOAT_EQ(projPos.y, expectedY);
}

TEST_F(ShootingSystemTest, ProjectileHasCorrectVelocity) {
    // Create a player entity
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, FireRate{0.5f, 999.0f});
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    // Publish fire event
    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerFireEvent{player});

    // Get projectile velocity
    auto& velocities = registry.get_components<Velocity>();
    auto& projectiles = registry.get_components<Projectile>();

    ASSERT_EQ(projectiles.size(), 1);
    Entity projectile = projectiles.get_entity_at(0);

    const Velocity& vel = velocities[projectile];

    EXPECT_FLOAT_EQ(vel.x, 400.0f); // bulletSpeed
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

// ============================================================================
// COOLDOWN SYSTEM TESTS
// ============================================================================

TEST_F(ShootingSystemTest, CooldownIncrementsOverTime) {
    // Create a player entity
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, FireRate{0.5f, 0.0f}); // Just shot
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& fireRates = registry.get_components<FireRate>();
    FireRate& fireRate = fireRates[player];

    EXPECT_FLOAT_EQ(fireRate.time_since_last_fire, 0.0f);

    // Update system (increments cooldowns)
    float deltaTime = 0.1f;
    shootingSystem->update(registry, deltaTime);

    EXPECT_FLOAT_EQ(fireRate.time_since_last_fire, 0.1f);

    // Update again
    shootingSystem->update(registry, deltaTime);

    EXPECT_FLOAT_EQ(fireRate.time_since_last_fire, 0.2f);
}

TEST_F(ShootingSystemTest, CooldownResetsAfterShooting) {
    // Create a player entity
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, FireRate{0.5f, 999.0f}); // Ready to shoot
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& fireRates = registry.get_components<FireRate>();
    FireRate& fireRate = fireRates[player];

    EXPECT_FLOAT_EQ(fireRate.time_since_last_fire, 999.0f);

    // Publish fire event
    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerFireEvent{player});

    // Cooldown should be reset to 0
    EXPECT_FLOAT_EQ(fireRate.time_since_last_fire, 0.0f);
}

TEST_F(ShootingSystemTest, CanShootAgainAfterCooldownExpires) {
    // Create a player entity
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, FireRate{0.5f, 999.0f}); // Ready to shoot
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    auto& projectiles = registry.get_components<Projectile>();

    // First shot
    eventBus.publish(ecs::PlayerFireEvent{player});
    EXPECT_EQ(projectiles.size(), 1);

    // Try to shoot immediately (should fail)
    eventBus.publish(ecs::PlayerFireEvent{player});
    EXPECT_EQ(projectiles.size(), 1); // Still only 1 projectile

    // Wait for cooldown to expire (6 updates of 0.1s = 0.6s > 0.5s cooldown)
    for (int i = 0; i < 6; i++) {
        shootingSystem->update(registry, 0.1f);
    }

    // Should be able to shoot again
    eventBus.publish(ecs::PlayerFireEvent{player});
    EXPECT_EQ(projectiles.size(), 2); // Now 2 projectiles
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_F(ShootingSystemTest, NoShootWithoutPosition) {
    // Create a player without Position component
    Entity player = registry.spawn_entity();
    registry.add_component(player, FireRate{0.5f, 999.0f});
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& projectiles = registry.get_components<Projectile>();

    // Publish fire event
    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerFireEvent{player});

    // No projectile should be created
    EXPECT_EQ(projectiles.size(), 0);
}

TEST_F(ShootingSystemTest, NoShootWithoutFireRate) {
    // Create a player without FireRate component
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& projectiles = registry.get_components<Projectile>();

    // Publish fire event
    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerFireEvent{player});

    // No projectile should be created
    EXPECT_EQ(projectiles.size(), 0);
}

TEST_F(ShootingSystemTest, MultiplePlayersCanShootIndependently) {
    // Create two players
    Entity player1 = registry.spawn_entity();
    registry.add_component(player1, Position{100.0f, 100.0f});
    registry.add_component(player1, FireRate{0.5f, 999.0f});
    registry.add_component(player1, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    Entity player2 = registry.spawn_entity();
    registry.add_component(player2, Position{200.0f, 200.0f});
    registry.add_component(player2, FireRate{0.5f, 999.0f});
    registry.add_component(player2, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    auto& projectiles = registry.get_components<Projectile>();

    // Player 1 shoots
    eventBus.publish(ecs::PlayerFireEvent{player1});
    EXPECT_EQ(projectiles.size(), 1);

    // Player 2 shoots
    eventBus.publish(ecs::PlayerFireEvent{player2});
    EXPECT_EQ(projectiles.size(), 2);
}
