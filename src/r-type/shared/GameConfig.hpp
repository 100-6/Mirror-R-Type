#pragma once

#include <cstdint>
#include "protocol/NetworkConfig.hpp"
#include "protocol/PacketTypes.hpp"

namespace rtype::shared::config {

// === Network Configuration ===
constexpr uint16_t DEFAULT_TCP_PORT = protocol::config::DEFAULT_TCP_PORT;
constexpr uint16_t DEFAULT_UDP_PORT = protocol::config::DEFAULT_UDP_PORT;
constexpr uint32_t MAX_CLIENTS = 16;
constexpr uint32_t MAX_PACKET_SIZE = 1400;

// === Timing Configuration ===
constexpr uint32_t SERVER_TICK_RATE = 64;                               // 64 ticks per second
constexpr float TICK_INTERVAL = 1.0f / SERVER_TICK_RATE;                // ~15.625ms per tick
constexpr uint32_t TICK_INTERVAL_MS = 1000 / SERVER_TICK_RATE;          // 15ms
constexpr uint32_t SNAPSHOT_RATE = 60;
constexpr float SNAPSHOT_INTERVAL = 1.0f / SNAPSHOT_RATE;               // 0.05s = 50ms

// === Lobby Configuration ===
constexpr uint8_t MAX_PLAYERS_PER_LOBBY = 4;
constexpr uint8_t MIN_PLAYERS_DUO = 2;
constexpr uint8_t MIN_PLAYERS_TRIO = 3;
constexpr uint8_t MIN_PLAYERS_SQUAD = 4;
constexpr uint32_t LOBBY_COUNTDOWN_DURATION_SECONDS = 5;

// === Gameplay Configuration ===
constexpr float GAME_SCROLL_SPEED = 50.0f;                              // pixels per second
constexpr float PLAYER_MOVEMENT_SPEED = 200.0f;                         // pixels per second
constexpr uint16_t PLAYER_MAX_HEALTH = 1000;
constexpr uint8_t PLAYER_LIVES = 3;

// === Difficulty Multipliers ===
// Enemy Health Multipliers (how much more HP enemies have)
constexpr float DIFFICULTY_EASY_HEALTH_MULT = 0.7f;
constexpr float DIFFICULTY_NORMAL_HEALTH_MULT = 1.0f;
constexpr float DIFFICULTY_HARD_HEALTH_MULT = 1.5f;

// Enemy Speed Multipliers (how fast enemies move)
constexpr float DIFFICULTY_EASY_SPEED_MULT = 0.8f;
constexpr float DIFFICULTY_NORMAL_SPEED_MULT = 1.0f;
constexpr float DIFFICULTY_HARD_SPEED_MULT = 1.3f;

// Enemy Damage Multipliers (how much damage enemy projectiles deal)
constexpr float DIFFICULTY_EASY_DAMAGE_MULT = 0.7f;
constexpr float DIFFICULTY_NORMAL_DAMAGE_MULT = 1.0f;
constexpr float DIFFICULTY_HARD_DAMAGE_MULT = 1.5f;

// Player Lives by Difficulty
constexpr uint8_t DIFFICULTY_EASY_LIVES = 5;
constexpr uint8_t DIFFICULTY_NORMAL_LIVES = 3;
constexpr uint8_t DIFFICULTY_HARD_LIVES = 1;

// === Screen / World ===
constexpr float SCREEN_WIDTH = 1920.0f;
constexpr float SCREEN_HEIGHT = 1080.0f;
constexpr float WORLD_WIDTH = 1920.0f;
constexpr float WORLD_HEIGHT = 1080.0f;

// === Player / Projectile dimensions ===
// Player ship hitbox sizes by ship type
// Small hitbox (SCOUT)
constexpr float PLAYER_HITBOX_SMALL_WIDTH = 80.0f;
constexpr float PLAYER_HITBOX_SMALL_HEIGHT = 80.0f;

// Medium hitbox (FIGHTER, BOMBER)
constexpr float PLAYER_HITBOX_MEDIUM_WIDTH = 104.0f;
constexpr float PLAYER_HITBOX_MEDIUM_HEIGHT = 104.0f;

// Large hitbox (CRUISER, CARRIER)
constexpr float PLAYER_HITBOX_LARGE_WIDTH = 128.0f;
constexpr float PLAYER_HITBOX_LARGE_HEIGHT = 128.0f;

// Legacy constants for backward compatibility
constexpr float PLAYER_WIDTH = PLAYER_HITBOX_LARGE_WIDTH;
constexpr float PLAYER_HEIGHT = PLAYER_HITBOX_LARGE_HEIGHT;
constexpr float PROJECTILE_WIDTH = 52.0f;
constexpr float PROJECTILE_HEIGHT = 24.0f;

// === Projectile behaviour ===
constexpr float PROJECTILE_SPEED = 500.0f;                              // pixels per second
constexpr float PROJECTILE_LIFETIME = 10.0f;                            // seconds (longer range)
constexpr int PROJECTILE_DAMAGE = 25;                                   // player projectile damage
constexpr int ENEMY_PROJECTILE_DAMAGE = 20;                             // enemy projectile damage

// === Enemy Configuration ===
constexpr float ENEMY_BASIC_SPEED = 100.0f;                             // pixels per second
constexpr uint16_t ENEMY_BASIC_HEALTH = 50;
constexpr float ENEMY_BASIC_WIDTH = 120.0f;
constexpr float ENEMY_BASIC_HEIGHT = 120.0f;

constexpr float ENEMY_FAST_SPEED = 200.0f;
constexpr uint16_t ENEMY_FAST_HEALTH = 30;
constexpr float ENEMY_FAST_WIDTH = 100.0f;
constexpr float ENEMY_FAST_HEIGHT = 100.0f;

constexpr float ENEMY_TANK_SPEED = 50.0f;
constexpr uint16_t ENEMY_TANK_HEALTH = 150;
constexpr float ENEMY_TANK_WIDTH = 160.0f;
constexpr float ENEMY_TANK_HEIGHT = 160.0f;

constexpr float ENEMY_BOSS_SPEED = 30.0f;
constexpr uint16_t ENEMY_BOSS_HEALTH = 500;
constexpr float ENEMY_BOSS_WIDTH = 280.0f;
constexpr float ENEMY_BOSS_HEIGHT = 280.0f;

// === Spawn / world positions ===
constexpr float PLAYER_SPAWN_X = 100.0f;
constexpr float PLAYER_SPAWN_Y_BASE = 300.0f;
constexpr float PLAYER_SPAWN_Y_OFFSET = 80.0f;                          // Vertical spacing between players

constexpr float ENTITY_OFFSCREEN_LEFT = -100.0f;                        // When to despawn enemies
constexpr float ENTITY_OFFSCREEN_RIGHT = 2000.0f;

// === Wave Configuration ===
constexpr float DEFAULT_SPAWN_INTERVAL = 2.0f;                          // seconds between spawns
constexpr float DEFAULT_ENEMY_SPAWN_X = 1920.0f;                        // Right edge of screen
constexpr float DEFAULT_ENEMY_SPAWN_Y = 300.0f;

// === Wall Configuration ===
constexpr float WALL_WIDTH = 100.0f;
constexpr float WALL_HEIGHT = 80.0f;

// === Powerup/Bonus Configuration ===
constexpr float BONUS_SIZE = 12.0f;

// === Explosion FX Configuration ===
constexpr float EXPLOSION_FRAME_SIZE = 32.0f;
constexpr float EXPLOSION_FRAME_TIME = 0.04f;
constexpr float EXPLOSION_DRAW_SCALE = 3.0f;

// === Difficulty Helper Functions ===

/**
 * @brief Get the health multiplier for a given difficulty
 */
inline float get_health_multiplier(protocol::Difficulty difficulty) {
    switch (difficulty) {
        case protocol::Difficulty::EASY:   return DIFFICULTY_EASY_HEALTH_MULT;
        case protocol::Difficulty::NORMAL: return DIFFICULTY_NORMAL_HEALTH_MULT;
        case protocol::Difficulty::HARD:   return DIFFICULTY_HARD_HEALTH_MULT;
        default: return DIFFICULTY_NORMAL_HEALTH_MULT;
    }
}

/**
 * @brief Get the speed multiplier for a given difficulty
 */
inline float get_speed_multiplier(protocol::Difficulty difficulty) {
    switch (difficulty) {
        case protocol::Difficulty::EASY:   return DIFFICULTY_EASY_SPEED_MULT;
        case protocol::Difficulty::NORMAL: return DIFFICULTY_NORMAL_SPEED_MULT;
        case protocol::Difficulty::HARD:   return DIFFICULTY_HARD_SPEED_MULT;
        default: return DIFFICULTY_NORMAL_SPEED_MULT;
    }
}

/**
 * @brief Get the damage multiplier for a given difficulty
 */
inline float get_damage_multiplier(protocol::Difficulty difficulty) {
    switch (difficulty) {
        case protocol::Difficulty::EASY:   return DIFFICULTY_EASY_DAMAGE_MULT;
        case protocol::Difficulty::NORMAL: return DIFFICULTY_NORMAL_DAMAGE_MULT;
        case protocol::Difficulty::HARD:   return DIFFICULTY_HARD_DAMAGE_MULT;
        default: return DIFFICULTY_NORMAL_DAMAGE_MULT;
    }
}

/**
 * @brief Get the number of player lives for a given difficulty
 */
inline uint8_t get_lives_for_difficulty(protocol::Difficulty difficulty) {
    switch (difficulty) {
        case protocol::Difficulty::EASY:   return DIFFICULTY_EASY_LIVES;
        case protocol::Difficulty::NORMAL: return DIFFICULTY_NORMAL_LIVES;
        case protocol::Difficulty::HARD:   return DIFFICULTY_HARD_LIVES;
        default: return DIFFICULTY_NORMAL_LIVES;
    }
}

} // namespace rtype::shared::config
