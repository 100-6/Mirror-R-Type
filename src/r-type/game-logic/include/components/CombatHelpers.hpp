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

// Helper pour récupérer les stats d'une arme
inline void get_weapon_stats(WeaponType type, int& projectiles, float& spread, float& speed, float& firerate, float& burst_delay)
{
    switch (type) {
        case WeaponType::BASIC:
            projectiles = WEAPON_BASIC_PROJECTILES;
            spread = WEAPON_BASIC_SPREAD;
            speed = WEAPON_BASIC_SPEED;
            firerate = WEAPON_BASIC_FIRERATE;
            burst_delay = WEAPON_BASIC_BURST_DELAY;
            break;
        case WeaponType::SPREAD:
            projectiles = WEAPON_SPREAD_PROJECTILES;
            spread = WEAPON_SPREAD_SPREAD;
            speed = WEAPON_SPREAD_SPEED;
            firerate = WEAPON_SPREAD_FIRERATE;
            burst_delay = WEAPON_SPREAD_BURST_DELAY;
            break;
        case WeaponType::BURST:
            projectiles = WEAPON_BURST_PROJECTILES;
            spread = WEAPON_BURST_SPREAD;
            speed = WEAPON_BURST_SPEED;
            firerate = WEAPON_BURST_FIRERATE;
            burst_delay = WEAPON_BURST_BURST_DELAY;
            break;
        case WeaponType::LASER:
            projectiles = WEAPON_LASER_PROJECTILES;
            spread = WEAPON_LASER_SPREAD;
            speed = WEAPON_LASER_SPEED;
            firerate = WEAPON_LASER_FIRERATE;
            burst_delay = WEAPON_LASER_BURST_DELAY;
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
