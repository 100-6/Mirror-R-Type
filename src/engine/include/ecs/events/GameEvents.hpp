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

}
