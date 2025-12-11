/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CoreComponents
*/

#ifndef CORE_COMPONENTS_HPP_
#define CORE_COMPONENTS_HPP_

#include "plugin_manager/CommonTypes.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <unordered_map>
#include <string>

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
    int playerId = 0;
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

// Effet visuel : cercle autour d'une entité (ex: bouclier)
struct CircleEffect {
    static constexpr float DEFAULT_RADIUS = 30.0f;
    static constexpr int DEFAULT_LAYER = 10;

    float radius = DEFAULT_RADIUS;
    engine::Color color = engine::Color::ShieldViolet;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    bool active = true;
    int layer = DEFAULT_LAYER;
};

// Effet visuel : texte flottant (ex: indicateur de boost)
struct TextEffect {
    static constexpr float DEFAULT_POS_X = 10.0f;
    static constexpr float DEFAULT_POS_Y = 150.0f;
    static constexpr int DEFAULT_FONT_SIZE = 25;

    std::string text = "";
    float posX = DEFAULT_POS_X;
    float posY = DEFAULT_POS_Y;
    engine::Color color = engine::Color::SpeedBlue;
    int fontSize = DEFAULT_FONT_SIZE;
    bool active = true;
};

#endif /* !CORE_COMPONENTS_HPP_ */
