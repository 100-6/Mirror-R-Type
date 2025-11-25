/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Temporary Events for System Testing
*/

#pragma once

#include "TempComponents.hpp"
#include "../../include/core/event/Event.hpp"
#include <string>

namespace rtype {

// Use the official Event base class from core
using Event = core::Event;

/**
 * @brief Enemy killed event
 */
struct EnemyKilledEvent : public Event {
    EntityId enemy_id;
    EntityId killer_id;
    Vector2f position;
    int score_value;

    EnemyKilledEvent(EntityId enemy, EntityId killer, Vector2f pos, int score)
        : enemy_id(enemy), killer_id(killer), position(pos), score_value(score) {}
};

/**
 * @brief Player hit event
 */
struct PlayerHitEvent : public Event {
    EntityId player_id;
    EntityId attacker_id;
    Vector2f position;
    int damage;

    PlayerHitEvent(EntityId player, EntityId attacker, Vector2f pos, int dmg)
        : player_id(player), attacker_id(attacker), position(pos), damage(dmg) {}
};

/**
 * @brief Power-up collected event
 */
struct PowerUpCollectedEvent : public Event {
    EntityId player_id;
    EntityId powerup_id;
    Vector2f position;
    std::string powerup_type;

    PowerUpCollectedEvent(EntityId player, EntityId powerup, Vector2f pos, std::string type)
        : player_id(player), powerup_id(powerup), position(pos), powerup_type(type) {}
};

/**
 * @brief Score changed event
 */
struct ScoreChangedEvent : public Event {
    EntityId player_id;
    int old_score;
    int new_score;
    int delta;

    ScoreChangedEvent(EntityId player, int old_s, int new_s, int d)
        : player_id(player), old_score(old_s), new_score(new_s), delta(d) {}
};

/**
 * @brief Collision event
 */
struct CollisionEvent : public Event {
    EntityId entity_a;
    EntityId entity_b;
    Vector2f position;
    Vector2f normal;

    CollisionEvent(EntityId a, EntityId b, Vector2f pos, Vector2f norm)
        : entity_a(a), entity_b(b), position(pos), normal(norm) {}
};

} // namespace rtype
