/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CoreComponents
*/

#ifndef CORE_COMPONENTS_HPP_
#define CORE_COMPONENTS_HPP_

#include "plugin_manager/CommonTypes.hpp"

// Physique et Mouvement 

struct Position {
    float x = 0.0f;
    float y = 0.0f;
};

struct Velocity {
    float x = 0.0f;
    float y = 0.0f;
};

// Collision

struct Collider {
    float width = 0;
    float height = 0;
};

// Input

struct Input {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool fire = false;        // Tirer (Espace ou clic gauche)
    bool special = false;     // Action spéciale (Shift)
};

// Rendu

struct Sprite {
    engine::TextureHandle texture = engine::INVALID_HANDLE;
    float width = 0.0f;
    float height = 0.0f;
    float rotation = 0.0f;
    engine::Color tint = engine::Color::White;

    // Origin/Pivot pour centrer le sprite (par défaut coin supérieur gauche)
    float origin_x = 0.0f;
    float origin_y = 0.0f;

    // Layer pour l'ordre de rendu (0=fond, plus élevé=premier plan)
    int layer = 0;
};

// Tags Génériques

struct Controllable {
    float speed = 200.0f;
};

struct NoFriction {};

struct ToDestroy {};

#endif /* !CORE_COMPONENTS_HPP_ */
