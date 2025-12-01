/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Component
*/

#ifndef COMPONENT_HPP_
#define COMPONENT_HPP_
#include <iostream>
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

struct Input {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool fire = false;     // Tirer (Espace ou clic gauche)
    bool special = false;  // Action spéciale (Shift)
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

// Tags

struct Controllable {
    float speed = 200.0f;
};
struct Enemy {};
struct Projectile {};
struct Wall {};

// Logique de jeu

struct Health
{
    int max = 100;
    int current = 100;
};

struct Damage
{
    int value = 10;
};


#endif /* !COMPONENT_HPP_ */