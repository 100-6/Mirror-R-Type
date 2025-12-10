/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CombatConfig - Configuration simple des armes et ennemis
*/

#ifndef COMBAT_CONFIG_HPP_
#define COMBAT_CONFIG_HPP_

// ============= WEAPONS =============

// BASIC - Tir simple
#define WEAPON_BASIC_PROJECTILES    1
#define WEAPON_BASIC_SPREAD         0.0f
#define WEAPON_BASIC_SPEED          400.0f
#define WEAPON_BASIC_FIRERATE       0.5f
#define WEAPON_BASIC_BURST_DELAY    0.0f

// SPREAD - Tir en éventail
#define WEAPON_SPREAD_PROJECTILES   3
#define WEAPON_SPREAD_SPREAD        30.0f
#define WEAPON_SPREAD_SPEED         350.0f
#define WEAPON_SPREAD_FIRERATE      0.7f
#define WEAPON_SPREAD_BURST_DELAY   0.0f

// BURST - Rafale rapide
#define WEAPON_BURST_PROJECTILES    5
#define WEAPON_BURST_SPREAD         0.0f
#define WEAPON_BURST_SPEED          450.0f
#define WEAPON_BURST_FIRERATE       1.0f
#define WEAPON_BURST_BURST_DELAY    0.05f

// LASER - À implémenter
#define WEAPON_LASER_PROJECTILES    1
#define WEAPON_LASER_SPREAD         0.0f
#define WEAPON_LASER_SPEED          0.0f
#define WEAPON_LASER_FIRERATE       0.0f
#define WEAPON_LASER_BURST_DELAY    0.0f

// ============= ENEMIES AI =============

// BASIC - Ennemi basique
#define ENEMY_BASIC_DETECTION       800.0f
#define ENEMY_BASIC_SHOOT_COOLDOWN  2.0f
#define ENEMY_BASIC_SPEED           100.0f
#define ENEMY_BASIC_HEALTH          30

// FAST - Ennemi rapide
#define ENEMY_FAST_DETECTION        600.0f
#define ENEMY_FAST_SHOOT_COOLDOWN   1.5f
#define ENEMY_FAST_SPEED            200.0f
#define ENEMY_FAST_HEALTH           20

// TANK - Ennemi tanky
#define ENEMY_TANK_DETECTION        1000.0f
#define ENEMY_TANK_SHOOT_COOLDOWN   3.0f
#define ENEMY_TANK_SPEED            50.0f
#define ENEMY_TANK_HEALTH           100

// BOSS - Boss
#define ENEMY_BOSS_DETECTION        1200.0f
#define ENEMY_BOSS_SHOOT_COOLDOWN   0.8f
#define ENEMY_BOSS_SPEED            80.0f
#define ENEMY_BOSS_HEALTH           500

#endif /* !COMBAT_CONFIG_HPP_ */
