/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test Collision Simple - GTest version
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/systems/CollisionSystem.hpp"

class CollisionSimpleTest : public ::testing::Test {
protected:
    Registry registry;
    CollisionSystem collisionSystem;

    void SetUp() override {
        registry.register_component<Position>();
        registry.register_component<Collider>();
        registry.register_component<Controllable>();
    }
};

TEST_F(CollisionSimpleTest, TwoPlayersCollideAfterMovement) {
    // Créer deux joueurs
    Entity playerA = registry.spawn_entity();
    registry.add_component<Position>(playerA, Position{0.0f, 0.0f});
    registry.add_component<Collider>(playerA, Collider{50.0f, 50.0f});
    registry.add_component<Controllable>(playerA, Controllable{});

    Entity playerB = registry.spawn_entity();
    registry.add_component<Position>(playerB, Position{100.0f, 0.0f});
    registry.add_component<Collider>(playerB, Collider{50.0f, 50.0f});
    registry.add_component<Controllable>(playerB, Controllable{});

    // Vérifier qu'ils ne sont pas en collision initialement
    bool collisionDetected = false;
    collisionSystem.scan_collisions<Controllable, Controllable>(registry, [&](Entity e1, Entity e2) {
        if (e1 != e2) collisionDetected = true;
    });
    EXPECT_FALSE(collisionDetected) << "Les joueurs ne devraient pas être en collision au début";

    // Déplacer playerA vers la droite (2 fois)
    auto& positions = registry.get_components<Position>();
    auto& posA = positions[playerA];
    posA.x += 30.0f;  // 1er mouvement
    posA.x += 30.0f;  // 2ème mouvement

    // Position finale de A: (60, 0)
    // Position de B: (100, 0)
    // Distance entre centres: 40 pixels
    // Somme des rayons: 50 pixels
    // Donc ils devraient être en collision

    // Vérifier la collision
    collisionDetected = false;
    collisionSystem.scan_collisions<Controllable, Controllable>(registry, [&](Entity e1, Entity e2) {
        if (e1 != e2) collisionDetected = true;
    });
    EXPECT_TRUE(collisionDetected) << "Les joueurs devraient être en collision après le mouvement";
}

TEST_F(CollisionSimpleTest, NoCollisionWhenFarApart) {
    Entity playerA = registry.spawn_entity();
    registry.add_component<Position>(playerA, Position{0.0f, 0.0f});
    registry.add_component<Collider>(playerA, Collider{10.0f, 10.0f});
    registry.add_component<Controllable>(playerA, Controllable{});

    Entity playerB = registry.spawn_entity();
    registry.add_component<Position>(playerB, Position{200.0f, 200.0f});
    registry.add_component<Collider>(playerB, Collider{10.0f, 10.0f});
    registry.add_component<Controllable>(playerB, Controllable{});

    bool collisionDetected = false;
    collisionSystem.scan_collisions<Controllable, Controllable>(registry, [&](Entity e1, Entity e2) {
        if (e1 != e2) collisionDetected = true;
    });
    EXPECT_FALSE(collisionDetected) << "Les joueurs éloignés ne devraient pas être en collision";
}

TEST_F(CollisionSimpleTest, CollisionWithExactOverlap) {
    Entity playerA = registry.spawn_entity();
    registry.add_component<Position>(playerA, Position{50.0f, 50.0f});
    registry.add_component<Collider>(playerA, Collider{30.0f, 30.0f});
    registry.add_component<Controllable>(playerA, Controllable{});

    Entity playerB = registry.spawn_entity();
    registry.add_component<Position>(playerB, Position{50.0f, 50.0f});
    registry.add_component<Collider>(playerB, Collider{30.0f, 30.0f});
    registry.add_component<Controllable>(playerB, Controllable{});

    bool collisionDetected = false;
    collisionSystem.scan_collisions<Controllable, Controllable>(registry, [&](Entity e1, Entity e2) {
        if (e1 != e2) collisionDetected = true;
    });
    EXPECT_TRUE(collisionDetected) << "Les joueurs superposés devraient être en collision";
}
