/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameComponents
*/

#ifndef GAME_COMPONENTS_HPP_
#define GAME_COMPONENTS_HPP_

#include "ecs/CoreComponents.hpp"

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

// Tags Spécifiques R-Type

struct Enemy {};

struct Projectile {
    float angle = 0.0f;
    float lifetime = 5.0f;
    float time_alive = 0.0f;
};

struct EnemyProjectile {};  // Projectile tiré par un ennemi
struct IsEnemyProjectile {};  // Alias pour AISystem (Legacy support or fix later)
struct Wall {};
struct Background {};

struct HitFlash {
    float time_remaining = 0.0f;
    engine::Color original_color = engine::Color::White;
};

// Logique de jeu (Stats)

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

#endif /* !GAME_COMPONENTS_HPP_ */
