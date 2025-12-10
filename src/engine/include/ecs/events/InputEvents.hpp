/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InputEvents - Events fired by the input system
*/

#pragma once

#include "core/event/Event.hpp"
#include "ecs/CoreComponents.hpp"

namespace ecs {

/**
 * @brief Event fired when a player wants to move
 */
struct PlayerMoveEvent : public core::Event {
    Entity player;
    float directionX;  // -1 (left), 0, +1 (right)
    float directionY;  // -1 (up), 0, +1 (down)

    PlayerMoveEvent(Entity p, float dx, float dy)
        : player(p), directionX(dx), directionY(dy) {}
};

/**
 * @brief Event fired when a player presses the fire button
 */
struct PlayerFireEvent : public core::Event {
    Entity player;

    PlayerFireEvent(Entity p) : player(p) {}
};

/**
 * @brief Event fired when a player presses the special button
 */
struct PlayerSpecialEvent : public core::Event {
    Entity player;

    PlayerSpecialEvent(Entity p) : player(p) {}
};

/**
 * @brief Event fired when an enemy is killed by a projectile
 */
struct EnemyKilledEvent : public core::Event {
    Entity enemy;
    int scoreValue;

    EnemyKilledEvent(Entity e, int score = 100) : enemy(e), scoreValue(score) {}
};

/**
 * @brief Event fired when an entity takes damage
 */
struct DamageEvent : public core::Event {
    Entity target;      // Entité qui reçoit les dégâts
    Entity source;      // Entité qui cause les dégâts (projectile)
    int damageAmount;

    DamageEvent(Entity t, Entity s, int dmg) : target(t), source(s), damageAmount(dmg) {}
};

/**
 * @brief Event fired when an entity dies (HP <= 0)
 */
struct EntityDeathEvent : public core::Event {
    Entity entity;
    bool isPlayer;

    EntityDeathEvent(Entity e, bool player = false) : entity(e), isPlayer(player) {}
};

}
