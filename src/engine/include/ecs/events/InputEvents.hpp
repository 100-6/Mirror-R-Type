/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InputEvents - Events fired by the input system
*/

#pragma once

#include "core/event/Event.hpp"
#include "ecs/CoreComponents.hpp"
#include "plugin_manager/IInputPlugin.hpp"

namespace ecs {

// ========== RAW INPUT EVENTS (Generic, published by InputSystem) ==========

/**
 * @brief Raw event fired when any key is pressed (generic, no game logic)
 */
struct RawKeyPressedEvent : public core::Event {
    Entity entity;           // Entity with Input component
    engine::Key key;         // Which key was pressed

    RawKeyPressedEvent(Entity e, engine::Key k) : entity(e), key(k) {}
};

/**
 * @brief Raw event fired when any key is released (generic, no game logic)
 */
struct RawKeyReleasedEvent : public core::Event {
    Entity entity;
    engine::Key key;

    RawKeyReleasedEvent(Entity e, engine::Key k) : entity(e), key(k) {}
};

/**
 * @brief Raw event for mouse button press
 */
struct RawMouseButtonPressedEvent : public core::Event {
    Entity entity;
    engine::MouseButton button;
    engine::Vector2f position;

    RawMouseButtonPressedEvent(Entity e, engine::MouseButton b, engine::Vector2f pos)
        : entity(e), button(b), position(pos) {}
};

// ========== GAME-SPECIFIC EVENTS (R-Type specific, published by PlayerInputSystem) ==========

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
 * @brief Event fired when a player starts holding the fire button
 */
struct PlayerStartFireEvent : public core::Event {
    Entity player;
    PlayerStartFireEvent(Entity p) : player(p) {}
};

/**
 * @brief Event fired when a player releases the fire button
 */
struct PlayerStopFireEvent : public core::Event {
    Entity player;
    PlayerStopFireEvent(Entity p) : player(p) {}
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
    Entity killer;  // Entity that killed the enemy (owner of the projectile)

    EnemyKilledEvent(Entity e, int score = 100, Entity k = 0)
        : enemy(e), scoreValue(score), killer(k) {}
};

/**
 * @brief Event fired when an enemy takes damage but survives
 */
struct EnemyHitEvent : public core::Event {
    Entity enemy;
    Entity source;

    EnemyHitEvent(Entity e, Entity src = Entity{0}) : enemy(e), source(src) {}
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
