/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InputEvents - Events fired by the input system
*/

#pragma once

#include "core/event/Event.hpp"
#include "ecs/Components.hpp"

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

}
