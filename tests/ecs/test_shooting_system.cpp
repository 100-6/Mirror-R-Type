/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_shooting_system
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "systems/ShootingSystem.hpp"
#include "ecs/events/InputEvents.hpp"
#include "components/CombatHelpers.hpp"
#include "components/CombatConfig.hpp"
#include <cmath>

// ============================================================================
// SHOOTING SYSTEM TESTS WITH WEAPON SYSTEM
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
        registry.register_component<Weapon>();
        registry.register_component<ToDestroy>();

        // Create shooting system with new constructor
        shootingSystem = new ShootingSystem();
        shootingSystem->init(registry);
    }

    void TearDown() override {
        delete shootingSystem;
    }

    // Helper to create a basic weapon
    Weapon createBasicWeapon() {
        return create_weapon(WeaponType::BASIC, bulletTex);
    }

    Weapon createSpreadWeapon(int count = 5, float angle = 40.0f) {
        return create_weapon(WeaponType::SPREAD, bulletTex);
    }

    Weapon createBurstWeapon(int count = 3) {
        return create_weapon(WeaponType::BURST, bulletTex);
    }
};

// ============================================================================
// BASIC WEAPON TESTS
// ============================================================================

TEST_F(ShootingSystemTest, BasicWeaponCreatesOneProjectile) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBasicWeapon());
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), 0);

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    EXPECT_EQ(projectiles.size(), 1);
}

TEST_F(ShootingSystemTest, BasicWeaponProjectileHasCorrectVelocity) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBasicWeapon());
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& velocities = registry.get_components<Velocity>();
    auto& projectiles = registry.get_components<Projectile>();

    ASSERT_EQ(projectiles.size(), 1);
    Entity projectile = projectiles.get_entity_at(0);

    const Velocity& vel = velocities[projectile];
    EXPECT_FLOAT_EQ(vel.x, WEAPON_BASIC_SPEED);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

TEST_F(ShootingSystemTest, BasicWeaponProjectileHasCorrectAngle) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBasicWeapon());
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    ASSERT_EQ(projectiles.size(), 1);
    Entity projectile = projectiles.get_entity_at(0);

    const Projectile& proj = projectiles[projectile];
    EXPECT_FLOAT_EQ(proj.angle, 0.0f); // BASIC shoots straight (0°)
}

TEST_F(ShootingSystemTest, BasicWeaponProjectileHasCorrectDamage) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBasicWeapon());
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& damages = registry.get_components<Damage>();
    auto& projectiles = registry.get_components<Projectile>();
    
    ASSERT_EQ(projectiles.size(), 1);
    Entity projectile = projectiles.get_entity_at(0);
    
    ASSERT_TRUE(damages.has_entity(projectile));
    EXPECT_EQ(damages[projectile].value, WEAPON_BASIC_DAMAGE);
}

// ============================================================================
// SPREAD WEAPON TESTS
// ============================================================================

TEST_F(ShootingSystemTest, SpreadWeaponCreatesMultipleProjectiles) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createSpreadWeapon(5, 40.0f));
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), WEAPON_SPREAD_PROJECTILES);
}

TEST_F(ShootingSystemTest, SpreadWeaponProjectilesHaveDifferentAngles) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createSpreadWeapon(5, 40.0f));
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    ASSERT_EQ(projectiles.size(), WEAPON_SPREAD_PROJECTILES);

    // Expected angles for 3 projectiles with 30 deg spread: -15, 0, 15
    float expectedAngles[] = {-15.0f, 0.0f, 15.0f};

    for (size_t i = 0; i < WEAPON_SPREAD_PROJECTILES; i++) {
        Entity projEntity = projectiles.get_entity_at(i);
        const Projectile& proj = projectiles[projEntity];
        EXPECT_NEAR(proj.angle, expectedAngles[i], 0.01f);
    }
}

TEST_F(ShootingSystemTest, SpreadWeaponProjectilesHaveCorrectVelocityComponents) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createSpreadWeapon(3, 30.0f)); // 3 projectiles, 30° spread
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& velocities = registry.get_components<Velocity>();
    auto& projectiles = registry.get_components<Projectile>();
    ASSERT_EQ(projectiles.size(), WEAPON_SPREAD_PROJECTILES);

    // Middle projectile should go straight (0°)
    Entity midProjectile = projectiles.get_entity_at(1);
    const Velocity& midVel = velocities[midProjectile];
    EXPECT_NEAR(midVel.x, WEAPON_SPREAD_SPEED, 0.1f);
    EXPECT_NEAR(midVel.y, 0.0f, 0.1f);

    // First projectile (-15°)
    Entity firstProjectile = projectiles.get_entity_at(0);
    const Velocity& firstVel = velocities[firstProjectile];
    float expectedVx = WEAPON_SPREAD_SPEED * std::cos(-15.0f * M_PI / 180.0f);
    float expectedVy = WEAPON_SPREAD_SPEED * std::sin(-15.0f * M_PI / 180.0f);
    EXPECT_NEAR(firstVel.x, expectedVx, 0.1f);
    EXPECT_NEAR(firstVel.y, expectedVy, 0.1f);
}

TEST_F(ShootingSystemTest, SpreadWeaponRespectedConfig) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createSpreadWeapon(1, 40.0f)); // 1 projectile requested but ignored
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), WEAPON_SPREAD_PROJECTILES);
}

// ============================================================================
// BURST WEAPON TESTS
// ============================================================================

TEST_F(ShootingSystemTest, BurstWeaponCreatesOneProjectilePerShot) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBurstWeapon(3));
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), 1); // Burst creates 1 projectile at a time
}

TEST_F(ShootingSystemTest, BurstWeaponReducesCooldownForRapidFire) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBurstWeapon(3));
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& weapons = registry.get_components<Weapon>();
    auto& eventBus = registry.get_event_bus();

    // First shot
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    Weapon& weapon = weapons[player];
    // After first shot in burst, cooldown should be reduced for rapid fire
    EXPECT_LT(weapon.time_since_last_fire, 0.1f); // Should be close to 0.05s or less
}

// ============================================================================
// WEAPON COOLDOWN TESTS
// ============================================================================

TEST_F(ShootingSystemTest, WeaponCooldownIncrementsOverTime) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});

    Weapon weapon = createBasicWeapon();
    weapon.time_since_last_fire = 0.0f; // Just shot
    registry.add_component(player, weapon);
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& weapons = registry.get_components<Weapon>();
    Weapon& weaponRef = weapons[player];

    EXPECT_FLOAT_EQ(weaponRef.time_since_last_fire, 0.0f);

    shootingSystem->update(registry, 0.1f);
    EXPECT_FLOAT_EQ(weaponRef.time_since_last_fire, 0.1f);

    shootingSystem->update(registry, 0.1f);
    EXPECT_FLOAT_EQ(weaponRef.time_since_last_fire, 0.2f);
}

TEST_F(ShootingSystemTest, CannotShootWhenCooldownNotReady) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});

    Weapon weapon = createBasicWeapon();
    weapon.time_since_last_fire = 0.0f; // Just shot, cooldown not ready
    registry.add_component(player, weapon);
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), 0); // No projectile should be created
}

TEST_F(ShootingSystemTest, CanShootAgainAfterCooldownExpires) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBasicWeapon());
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    auto& projectiles = registry.get_components<Projectile>();

    // First shot
    eventBus.publish(ecs::PlayerStartFireEvent{player});
    EXPECT_EQ(projectiles.size(), 1);

    // Try to shoot immediately (should fail)
    eventBus.publish(ecs::PlayerStartFireEvent{player});
    EXPECT_EQ(projectiles.size(), 1);

    // Wait for cooldown (6 * 0.1s = 0.6s > 0.5s fire_rate)
    for (int i = 0; i < 6; i++) {
        shootingSystem->update(registry, 0.1f);
    }

    // Should be able to shoot again
    eventBus.publish(ecs::PlayerStartFireEvent{player});
    EXPECT_EQ(projectiles.size(), 2);
}

// ============================================================================
// PROJECTILE LIFETIME TESTS
// ============================================================================

TEST_F(ShootingSystemTest, ProjectileLifetimeIncrementsOverTime) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBasicWeapon());
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    ASSERT_EQ(projectiles.size(), 1);
    Entity projEntity = projectiles.get_entity_at(0);

    Projectile& proj = projectiles[projEntity];
    EXPECT_FLOAT_EQ(proj.time_alive, 0.0f);

    shootingSystem->update(registry, 0.5f);
    EXPECT_FLOAT_EQ(proj.time_alive, 0.5f);

    shootingSystem->update(registry, 1.0f);
    EXPECT_FLOAT_EQ(proj.time_alive, 1.5f);
}

TEST_F(ShootingSystemTest, ProjectileMarkedForDestructionAfterLifetime) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createBasicWeapon());
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    auto& toDestroy = registry.get_components<ToDestroy>();
    ASSERT_EQ(projectiles.size(), 1);
    Entity projEntity = projectiles.get_entity_at(0);

    // Projectile lifetime is 5.0s by default
    // Update for 6 seconds
    for (int i = 0; i < 60; i++) {
        shootingSystem->update(registry, 0.1f);
    }

    // Projectile should be marked for destruction
    EXPECT_TRUE(toDestroy.has_entity(projEntity));
}

// ============================================================================
// EDGE CASES AND ERROR HANDLING
// ============================================================================

TEST_F(ShootingSystemTest, NoShootWithoutPosition) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, createBasicWeapon());
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), 0);
}

TEST_F(ShootingSystemTest, NoShootWithoutWeapon) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& projectiles = registry.get_components<Projectile>();
    EXPECT_EQ(projectiles.size(), 0);
}

TEST_F(ShootingSystemTest, ProjectileHasCorrectSprite) {
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{100.0f, 100.0f});
    registry.add_component(player, createSpreadWeapon(1, 0.0f));
    registry.add_component(player, Sprite{bulletTex, 64.0f, 32.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});

    auto& eventBus = registry.get_event_bus();
    eventBus.publish(ecs::PlayerStartFireEvent{player});

    auto& sprites = registry.get_components<Sprite>();
    auto& projectiles = registry.get_components<Projectile>();
    ASSERT_EQ(projectiles.size(), WEAPON_SPREAD_PROJECTILES);
    Entity projEntity = projectiles.get_entity_at(0);

    EXPECT_TRUE(sprites.has_entity(projEntity));
    const Sprite& projSprite = sprites[projEntity];

    // Check that sprite was copied correctly
    EXPECT_EQ(projSprite.texture, bulletTex);
    EXPECT_FLOAT_EQ(projSprite.width, WEAPON_SPREAD_WIDTH);
    EXPECT_FLOAT_EQ(projSprite.height, WEAPON_SPREAD_HEIGHT);
    EXPECT_EQ(projSprite.tint.r, WEAPON_SPREAD_COLOR_R);
    EXPECT_EQ(projSprite.tint.g, WEAPON_SPREAD_COLOR_G);
    EXPECT_EQ(projSprite.tint.b, WEAPON_SPREAD_COLOR_B);
}
