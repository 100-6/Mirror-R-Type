/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CombatHelpers - Helpers pour récupérer les stats de combat
*/

#ifndef COMBAT_HELPERS_HPP_
#define COMBAT_HELPERS_HPP_

#include "GameComponents.hpp"
#include "CombatConfig.hpp"

// Helper pour créer une arme configurée
inline Weapon create_weapon(WeaponType type, engine::TextureHandle texture)
{
    Weapon weapon;
    weapon.type = type;
    weapon.time_since_last_fire = 999.0f;
    weapon.burst_count = 0;
    
    // Config de base du sprite
    weapon.projectile_sprite.texture = texture;
    weapon.projectile_sprite.rotation = 0.0f;
    weapon.projectile_sprite.origin_x = 0.0f;
    weapon.projectile_sprite.origin_y = 0.0f;
    weapon.projectile_sprite.layer = 1;

    switch (type) {
        case WeaponType::BASIC:
            weapon.projectile_sprite.width = WEAPON_BASIC_WIDTH;
            weapon.projectile_sprite.height = WEAPON_BASIC_HEIGHT;
            weapon.projectile_sprite.tint = engine::Color{WEAPON_BASIC_COLOR_R, WEAPON_BASIC_COLOR_G, WEAPON_BASIC_COLOR_B, WEAPON_BASIC_COLOR_A};
            break;
        case WeaponType::SPREAD:
            weapon.projectile_sprite.width = WEAPON_SPREAD_WIDTH;
            weapon.projectile_sprite.height = WEAPON_SPREAD_HEIGHT;
            weapon.projectile_sprite.tint = engine::Color{WEAPON_SPREAD_COLOR_R, WEAPON_SPREAD_COLOR_G, WEAPON_SPREAD_COLOR_B, WEAPON_SPREAD_COLOR_A};
            break;
        case WeaponType::BURST:
            weapon.projectile_sprite.width = WEAPON_BURST_WIDTH;
            weapon.projectile_sprite.height = WEAPON_BURST_HEIGHT;
            weapon.projectile_sprite.tint = engine::Color{WEAPON_BURST_COLOR_R, WEAPON_BURST_COLOR_G, WEAPON_BURST_COLOR_B, WEAPON_BURST_COLOR_A};
            break;
        case WeaponType::LASER:
            weapon.projectile_sprite.width = WEAPON_LASER_WIDTH;
            weapon.projectile_sprite.height = WEAPON_LASER_HEIGHT;
            weapon.projectile_sprite.tint = engine::Color{WEAPON_LASER_COLOR_R, WEAPON_LASER_COLOR_G, WEAPON_LASER_COLOR_B, WEAPON_LASER_COLOR_A};
            break;
        case WeaponType::CHARGE:
            weapon.projectile_sprite.width = WEAPON_CHARGE_WIDTH_MIN;
            weapon.projectile_sprite.height = WEAPON_CHARGE_HEIGHT_MIN;
            weapon.projectile_sprite.tint = engine::Color{WEAPON_CHARGE_COLOR_R, WEAPON_CHARGE_COLOR_G, WEAPON_CHARGE_COLOR_B, WEAPON_CHARGE_COLOR_A};
            break;
    }
    
    return weapon;
}

// Helper pour récupérer les stats d'une arme
inline void get_weapon_stats(WeaponType type, int& projectiles, float& spread, int& damage, float& speed, float& firerate, float& burst_delay)
{
    switch (type) {
        case WeaponType::BASIC:
            projectiles = WEAPON_BASIC_PROJECTILES;
            spread = WEAPON_BASIC_SPREAD;
            damage = WEAPON_BASIC_DAMAGE;
            speed = WEAPON_BASIC_SPEED;
            firerate = WEAPON_BASIC_FIRERATE;
            burst_delay = WEAPON_BASIC_BURST_DELAY;
            break;
        case WeaponType::SPREAD:
            projectiles = WEAPON_SPREAD_PROJECTILES;
            spread = WEAPON_SPREAD_SPREAD;
            damage = WEAPON_SPREAD_DAMAGE;
            speed = WEAPON_SPREAD_SPEED;
            firerate = WEAPON_SPREAD_FIRERATE;
            burst_delay = WEAPON_SPREAD_BURST_DELAY;
            break;
        case WeaponType::BURST:
            projectiles = WEAPON_BURST_PROJECTILES;
            spread = WEAPON_BURST_SPREAD;
            damage = WEAPON_BURST_DAMAGE;
            speed = WEAPON_BURST_SPEED;
            firerate = WEAPON_BURST_FIRERATE;
            burst_delay = WEAPON_BURST_BURST_DELAY;
            break;
        case WeaponType::LASER:
            projectiles = WEAPON_LASER_PROJECTILES;
            spread = WEAPON_LASER_SPREAD;
            damage = WEAPON_LASER_DAMAGE;
            speed = WEAPON_LASER_SPEED;
            firerate = WEAPON_LASER_FIRERATE;
            burst_delay = WEAPON_LASER_BURST_DELAY;
            break;
        case WeaponType::CHARGE:
            projectiles = WEAPON_CHARGE_PROJECTILES;
            spread = WEAPON_CHARGE_SPREAD;
            damage = WEAPON_CHARGE_DAMAGE_MIN; // Damage depends on charge, returning MIN here
            speed = WEAPON_CHARGE_SPEED;
            firerate = WEAPON_CHARGE_FIRERATE;
            burst_delay = WEAPON_CHARGE_BURST_DELAY;
            break;
    }
}

// Helper pour récupérer les stats d'un ennemi
inline void get_enemy_stats(EnemyType type, float& detection, float& cooldown, float& speed, int& health)
{
    switch (type) {
        case EnemyType::Basic:
            detection = ENEMY_BASIC_DETECTION;
            cooldown = ENEMY_BASIC_SHOOT_COOLDOWN;
            speed = ENEMY_BASIC_SPEED;
            health = ENEMY_BASIC_HEALTH;
            break;
        case EnemyType::Fast:
            detection = ENEMY_FAST_DETECTION;
            cooldown = ENEMY_FAST_SHOOT_COOLDOWN;
            speed = ENEMY_FAST_SPEED;
            health = ENEMY_FAST_HEALTH;
            break;
        case EnemyType::Tank:
            detection = ENEMY_TANK_DETECTION;
            cooldown = ENEMY_TANK_SHOOT_COOLDOWN;
            speed = ENEMY_TANK_SPEED;
            health = ENEMY_TANK_HEALTH;
            break;
        case EnemyType::Boss:
            detection = ENEMY_BOSS_DETECTION;
            cooldown = ENEMY_BOSS_SHOOT_COOLDOWN;
            speed = ENEMY_BOSS_SPEED;
            health = ENEMY_BOSS_HEALTH;
            break;
    }
}

#endif /* !COMBAT_HELPERS_HPP_ */
