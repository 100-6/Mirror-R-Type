/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameEvents
*/

#pragma once

#include "core/event/Event.hpp"
#include "ecs/CoreComponents.hpp" // For Entity

namespace ecs {

/**
 * @brief Event fired when a player is hit by something
 */
struct PlayerHitEvent : public core::Event {
    Entity player;
    Entity attacker;

    PlayerHitEvent(Entity p, Entity a = -1) : player(p), attacker(a) {}
};

/**
 * @brief Event fired when a player collects a power-up
 */
struct PowerUpCollectedEvent : public core::Event {
    Entity player;
    Entity powerUp;

    PowerUpCollectedEvent(Entity p, Entity pu) : player(p), powerUp(pu) {}
};

/**
 * @brief Event fired when a projectile is successfully fired
 */
struct ShotFiredEvent : public core::Event {
    Entity shooter;
    Entity projectile;

    ShotFiredEvent(Entity s, Entity p) : shooter(s), projectile(p) {}
};

/**
 * @brief Event fired when an explosion should be spawned (typically when an enemy dies)
 */
struct ExplosionEvent : public core::Event {
    Entity source;
    float x;
    float y;
    float scale;

    ExplosionEvent(Entity src, float px, float py, float s = 1.0f)
        : source(src)
        , x(px)
        , y(py)
        , scale(s) {}
};

/**
 * @brief Event fired when a bonus should be spawned (typically when an enemy dies with bonusDrop)
 */
struct BonusSpawnEvent : public core::Event {
    float x;
    float y;
    int bonusType;  // Cast to BonusType enum

    BonusSpawnEvent(float px, float py, int type)
        : x(px)
        , y(py)
        , bonusType(type) {}
};

/**
 * @brief Event fired when a bonus is collected by a player (for network sync)
 */
struct BonusCollectedEvent : public core::Event {
    Entity player;
    int bonusType;  // Cast to BonusType enum

    BonusCollectedEvent(Entity p, int type)
        : player(p)
        , bonusType(type) {}
};

/**
 * @brief Event fired when a companion turret should be spawned for a player
 */
struct CompanionSpawnEvent : public core::Event {
    Entity player;
    uint32_t playerId;  // Network player ID for client identification

    CompanionSpawnEvent(Entity p, uint32_t id)
        : player(p)
        , playerId(id) {}
};

/**
 * @brief Event fired when a companion turret should be destroyed (player died)
 */
struct CompanionDestroyEvent : public core::Event {
    Entity player;

    explicit CompanionDestroyEvent(Entity p)
        : player(p) {}
};

/**
 * @brief Event fired when a muzzle flash effect should be spawned
 */
struct MuzzleFlashSpawnEvent : public core::Event {
    Entity shooter;       // Entity that fired (player, companion, or enemy)
    float projectileX;    // Position of the projectile
    float projectileY;
    bool isCompanion;     // True if shooter is a companion turret
    bool isEnemy;         // True if shooter is an enemy
    float shooterWidth;   // Width of the shooter for dynamic offset calculation

    MuzzleFlashSpawnEvent(Entity s, float px, float py, bool companion, bool enemy, float width)
        : shooter(s)
        , projectileX(px)
        , projectileY(py)
        , isCompanion(companion)
        , isEnemy(enemy)
        , shooterWidth(width) {}
};

/**
 * @brief Event fired when a muzzle flash should be destroyed (companion destroyed)
 */
struct MuzzleFlashDestroyEvent : public core::Event {
    Entity shooter;  // The shooter whose muzzle flash should be destroyed

    explicit MuzzleFlashDestroyEvent(Entity s)
        : shooter(s) {}
};

}
