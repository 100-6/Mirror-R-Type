/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Temporary Components for System Testing
*/

#pragma once

#include "../../plugin_manager/include/CommonTypes.hpp"
#include <cstdint>

namespace rtype {

/**
 * @brief Unique identifier for entities
 */
using EntityId = uint32_t;
constexpr EntityId INVALID_ENTITY = 0;

/**
 * @brief Transform component - position, rotation, scale
 */
struct TransformComponent {
    Vector2f position{0.0f, 0.0f};
    float rotation = 0.0f;
    Vector2f scale{1.0f, 1.0f};

    TransformComponent() = default;
    TransformComponent(float x, float y) : position(x, y) {}
};

/**
 * @brief Velocity component - movement speed
 */
struct VelocityComponent {
    Vector2f velocity{0.0f, 0.0f};
    float max_speed = 300.0f; // pixels per second

    VelocityComponent() = default;
    VelocityComponent(float vx, float vy) : velocity(vx, vy) {}
};

/**
 * @brief Input component - stores input state
 */
struct InputComponent {
    bool move_up = false;
    bool move_down = false;
    bool move_left = false;
    bool move_right = false;
    bool shoot = false;

    InputComponent() = default;
};

/**
 * @brief Sprite component - visual representation
 */
struct SpriteComponent {
    TextureHandle texture = INVALID_HANDLE;
    Vector2f size{32.0f, 32.0f};
    Color tint = Color::White;
    int z_order = 0;

    SpriteComponent() = default;
    SpriteComponent(TextureHandle tex, Vector2f sz) : texture(tex), size(sz) {}
};

/**
 * @brief Collider component - AABB collision box
 */
struct ColliderComponent {
    Vector2f offset{0.0f, 0.0f};
    Vector2f size{32.0f, 32.0f};
    bool is_trigger = false;
    uint32_t collision_layer = 0;
    uint32_t collision_mask = 0xFFFFFFFF;

    ColliderComponent() = default;
    ColliderComponent(Vector2f sz) : size(sz) {}
};

/**
 * @brief Player tag component
 */
struct PlayerComponent {
    uint32_t player_id = 0;
    int health = 100;

    PlayerComponent() = default;
    explicit PlayerComponent(uint32_t id) : player_id(id) {}
};

/**
 * @brief Enemy tag component
 */
struct EnemyComponent {
    int health = 50;
    int score_value = 100;

    EnemyComponent() = default;
};

/**
 * @brief Particle component
 */
struct ParticleComponent {
    float lifetime = 1.0f;
    float current_time = 0.0f;
    Color start_color = Color::White;
    Color end_color = Color::Transparent;

    ParticleComponent() = default;
    explicit ParticleComponent(float life) : lifetime(life) {}
};

/**
 * @brief Score component
 */
struct ScoreComponent {
    int score = 0;

    ScoreComponent() = default;
    explicit ScoreComponent(int s) : score(s) {}
};

/**
 * @brief Network component - marks entity as networked
 */
struct NetworkComponent {
    uint32_t network_id = 0;
    bool is_owner = false;

    NetworkComponent() = default;
    explicit NetworkComponent(uint32_t id) : network_id(id), is_owner(false) {}
};

} // namespace rtype
