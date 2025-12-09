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

// Tags

struct Controllable {
    float speed = 200.0f;
};
struct Enemy {};
struct Projectile {
    float angle = 0.0f;
    float lifetime = 5.0f;
    float time_alive = 0.0f;
};
struct EnemyProjectile {};  // Projectile tiré par un ennemi
struct IsEnemyProjectile {};  // Alias pour AISystem
struct Wall {};
struct ToDestroy {};
struct Background {};

// AI

enum class EnemyType {
    Basic,
    Fast,
    Tank,
    Boss
};

struct AI {
    EnemyType type = EnemyType::Basic;
    float detectionRange = 800.0f;
    float shootCooldown = 2.0f;
    float timeSinceLastShot = 0.0f;
    float moveSpeed = 100.0f;
};

// Scrolling

struct Scrollable {
    float speedMultiplier = 1.0f;  // Multiplier for scroll speed (1.0 = normal, 2.0 = twice as fast)
    bool wrap = false;              // If true, entity wraps around for infinite scrolling
    bool destroyOffscreen = false;  // If true, entity is destroyed when scrolling offscreen
};

// Combat

enum class WeaponType {
    BASIC,      // 1 projectile, tout droit
    SPREAD,     // Plusieurs projectiles en éventail
    BURST,      // Rafale rapide
    LASER       // Ligne continue (futur)
};

struct Weapon {
    WeaponType type = WeaponType::BASIC;
    int projectile_count = 1;              // Nombre de projectiles par tir
    float spread_angle = 0.0f;             // Angle d'écart total en degrés
    float projectile_speed = 400.0f;       // Vitesse des projectiles
    float fire_rate = 0.5f;                // Cooldown entre chaque tir (secondes)
    float time_since_last_fire = 999.0f;   // Temps écoulé depuis le dernier tir
    Sprite projectile_sprite;              // Apparence du projectile à créer
};

struct FireRate {
    float cooldown = 0.1f;
    float time_since_last_fire = 999.0f;
};

// Logique de jeu

struct Health
{
    int max = 100;
    int current = 100;
};

struct Invulnerability {
    float time_remaining = 0.0f;
};

struct Damage
{
    int value = 10;
};

struct Score
{
    int value = 0;
};

#endif /* !COMPONENT_HPP_ */