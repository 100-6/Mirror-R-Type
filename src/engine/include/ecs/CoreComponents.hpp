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
#include <vector>

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

    // Rectangle source pour découper une partie de la texture (spritesheet)
    // Si width/height = 0, utilise la texture complète
    struct SourceRect {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        
        bool is_valid() const { return width > 0.0f && height > 0.0f; }
    } source_rect;
};

// Animation de sprite (alternance entre plusieurs textures)
struct SpriteAnimation {
    std::vector<engine::TextureHandle> frames;  // Liste des textures
    float frameTime = 0.1f;                      // Temps par frame en secondes
    float elapsedTime = 0.0f;                    // Temps écoulé depuis le dernier changement
    size_t currentFrame = 0;                     // Index de la frame actuelle
    bool loop = true;                            // Boucler l'animation
    bool playing = true;                         // Animation en cours
};

// Attachement d'entité à une autre (pour effets visuels liés)
struct Attached {
    size_t parentEntity = 0;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float smoothFactor = 0.0f;  // 0.0 = suivi direct, > 0.0 = suivi avec latence (valeur typique: 5.0-15.0)
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

// UI Components for HUD elements

// Panneau rectangulaire avec bordure
struct UIPanel {
    float width = 100.0f;
    float height = 50.0f;
    engine::Color backgroundColor = engine::Color{20, 20, 30, 200};
    engine::Color borderColor = engine::Color{100, 100, 120, 255};
    float borderThickness = 2.0f;
    bool active = true;
    int layer = 100;  // HUD elements on top layer
};

// Barre de progression (santé, mana, etc.)
struct UIBar {
    float width = 200.0f;
    float height = 30.0f;
    float currentValue = 100.0f;
    float maxValue = 100.0f;
    engine::Color backgroundColor = engine::Color{40, 40, 50, 255};
    engine::Color fillColor = engine::Color{0, 255, 0, 255};
    engine::Color borderColor = engine::Color{150, 150, 180, 255};
    float borderThickness = 2.0f;
    bool active = true;
    int layer = 101;  // Slightly above panels
};

// Texte UI (différent de TextEffect qui est pour les effets temporaires)
struct UIText {
    std::string text = "";
    engine::Color color = engine::Color::White;
    engine::Color shadowColor = engine::Color{0, 0, 0, 180};
    int fontSize = 20;
    bool hasShadow = true;
    float shadowOffsetX = 2.0f;
    float shadowOffsetY = 2.0f;
    bool active = true;
    int layer = 102;  // Text on top of everything
};

#endif /* !CORE_COMPONENTS_HPP_ */
