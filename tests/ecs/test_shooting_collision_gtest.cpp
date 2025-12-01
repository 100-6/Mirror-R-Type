/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test Shooting Collision - GTest version
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/systems/CollisionSystem.hpp"

class ShootingCollisionTest : public ::testing::Test {
protected:
    Registry registry;

    void SetUp() override {
        registry.register_component<Position>();
        registry.register_component<Velocity>();
        registry.register_component<Collider>();
        registry.register_component<Controllable>();
        registry.register_component<Projectile>();
        registry.register_component<Enemy>();
        registry.register_component<Wall>();
        
        registry.register_system<CollisionSystem>();
    }
};

TEST_F(ShootingCollisionTest, ProjectileDestroysEnemyOnCollision) {
    // Créer un projectile
    Entity projectile = registry.spawn_entity();
    registry.add_component<Position>(projectile, Position{50.0f, 50.0f});
    registry.add_component<Collider>(projectile, Collider{10.0f, 5.0f});
    registry.add_component<Projectile>(projectile, Projectile{});

    // Créer un ennemi
    Entity enemy = registry.spawn_entity();
    registry.add_component<Position>(enemy, Position{55.0f, 50.0f});
    registry.add_component<Collider>(enemy, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy, Enemy{});

    // Vérifier qu'ils existent avant la collision
    auto& projectiles = registry.get_components<Projectile>();
    auto& enemies = registry.get_components<Enemy>();
    EXPECT_TRUE(projectiles.has_entity(projectile));
    EXPECT_TRUE(enemies.has_entity(enemy));

    // Exécuter le système de collision
    registry.run_systems();

    // Après collision, les deux devraient être détruits
    EXPECT_FALSE(projectiles.has_entity(projectile)) << "Le projectile devrait être détruit";
    EXPECT_FALSE(enemies.has_entity(enemy)) << "L'ennemi devrait être détruit";
}

TEST_F(ShootingCollisionTest, ProjectileDoesNotDestroyEnemyWhenFarApart) {
    // Créer un projectile loin
    Entity projectile = registry.spawn_entity();
    registry.add_component<Position>(projectile, Position{0.0f, 0.0f});
    registry.add_component<Collider>(projectile, Collider{10.0f, 5.0f});
    registry.add_component<Projectile>(projectile, Projectile{});

    // Créer un ennemi loin
    Entity enemy = registry.spawn_entity();
    registry.add_component<Position>(enemy, Position{200.0f, 200.0f});
    registry.add_component<Collider>(enemy, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy, Enemy{});

    // Exécuter le système de collision
    registry.run_systems();

    // Ils devraient toujours exister
    auto& projectiles = registry.get_components<Projectile>();
    auto& enemies = registry.get_components<Enemy>();
    EXPECT_TRUE(projectiles.has_entity(projectile));
    EXPECT_TRUE(enemies.has_entity(enemy));
}

TEST_F(ShootingCollisionTest, PlayerCollidesWithWall) {
    // Créer un joueur
    Entity player = registry.spawn_entity();
    registry.add_component<Position>(player, Position{100.0f, 100.0f});
    registry.add_component<Collider>(player, Collider{30.0f, 30.0f});
    registry.add_component<Controllable>(player, Controllable{});

    // Créer un mur proche
    Entity wall = registry.spawn_entity();
    registry.add_component<Position>(wall, Position{125.0f, 100.0f});
    registry.add_component<Collider>(wall, Collider{20.0f, 100.0f});
    registry.add_component<Wall>(wall, Wall{});

    // Position initiale
    auto& positions = registry.get_components<Position>();
    float initialX = positions[player].x;

    // Exécuter le système de collision (devrait repousser le joueur)
    registry.run_systems();

    // Le joueur devrait avoir été repoussé
    float finalX = positions[player].x;
    EXPECT_LT(finalX, initialX) << "Le joueur devrait être repoussé par le mur";
}

TEST_F(ShootingCollisionTest, MultipleProjectilesDestroyMultipleEnemies) {
    // Créer 3 projectiles
    Entity proj1 = registry.spawn_entity();
    registry.add_component<Position>(proj1, Position{50.0f, 50.0f});
    registry.add_component<Collider>(proj1, Collider{10.0f, 5.0f});
    registry.add_component<Projectile>(proj1, Projectile{});

    Entity proj2 = registry.spawn_entity();
    registry.add_component<Position>(proj2, Position{150.0f, 150.0f});
    registry.add_component<Collider>(proj2, Collider{10.0f, 5.0f});
    registry.add_component<Projectile>(proj2, Projectile{});

    Entity proj3 = registry.spawn_entity();
    registry.add_component<Position>(proj3, Position{250.0f, 250.0f});
    registry.add_component<Collider>(proj3, Collider{10.0f, 5.0f});
    registry.add_component<Projectile>(proj3, Projectile{});

    // Créer 3 ennemis aux mêmes positions
    Entity enemy1 = registry.spawn_entity();
    registry.add_component<Position>(enemy1, Position{52.0f, 50.0f});
    registry.add_component<Collider>(enemy1, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy1, Enemy{});

    Entity enemy2 = registry.spawn_entity();
    registry.add_component<Position>(enemy2, Position{152.0f, 150.0f});
    registry.add_component<Collider>(enemy2, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy2, Enemy{});

    Entity enemy3 = registry.spawn_entity();
    registry.add_component<Position>(enemy3, Position{252.0f, 250.0f});
    registry.add_component<Collider>(enemy3, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy3, Enemy{});

    // Exécuter le système de collision
    registry.run_systems();

    // Tous devraient être détruits
    auto& projectiles = registry.get_components<Projectile>();
    auto& enemies = registry.get_components<Enemy>();
    
    EXPECT_FALSE(projectiles.has_entity(proj1));
    EXPECT_FALSE(projectiles.has_entity(proj2));
    EXPECT_FALSE(projectiles.has_entity(proj3));
    EXPECT_FALSE(enemies.has_entity(enemy1));
    EXPECT_FALSE(enemies.has_entity(enemy2));
    EXPECT_FALSE(enemies.has_entity(enemy3));
}

TEST_F(ShootingCollisionTest, PlayerDoesNotDestroyEnemy) {
    // Créer un joueur
    Entity player = registry.spawn_entity();
    registry.add_component<Position>(player, Position{50.0f, 50.0f});
    registry.add_component<Collider>(player, Collider{30.0f, 30.0f});
    registry.add_component<Controllable>(player, Controllable{});

    // Créer un ennemi proche (mais pas de collision Controllable vs Enemy dans le système)
    Entity enemy = registry.spawn_entity();
    registry.add_component<Position>(enemy, Position{60.0f, 50.0f});
    registry.add_component<Collider>(enemy, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy, Enemy{});

    // Exécuter le système
    registry.run_systems();

    // L'ennemi devrait toujours exister (pas de collision player vs enemy)
    auto& enemies = registry.get_components<Enemy>();
    EXPECT_TRUE(enemies.has_entity(enemy)) << "L'ennemi ne devrait pas être détruit par un joueur";
}
