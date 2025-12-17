#pragma once

#include <cstdint>
#include "protocol/NetworkConfig.hpp"
#include <vector>

namespace rtype::server {

/**
 * @brief Serialize any POD struct to a byte vector
 */
template<typename T>
std::vector<uint8_t> serialize(const T& data) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&data);
    return std::vector<uint8_t>(bytes, bytes + sizeof(T));
}

/**
 * @brief Server configuration constants
 */
namespace config {

// === Network Configuration ===
// Port constants are inherited from protocol::config::DEFAULT_TCP_PORT and DEFAULT_UDP_PORT
// for consistency between client and server
constexpr uint16_t DEFAULT_TCP_PORT = protocol::config::DEFAULT_TCP_PORT;
constexpr uint16_t DEFAULT_UDP_PORT = protocol::config::DEFAULT_UDP_PORT;
constexpr uint32_t MAX_CLIENTS = 16;
constexpr uint32_t MAX_PACKET_SIZE = 1400;

// === Timing Configuration ===
    constexpr uint32_t SERVER_TICK_RATE = 64;                                    // 64 ticks per second
    constexpr float TICK_INTERVAL = 1.0f / SERVER_TICK_RATE;                     // ~15.625ms per tick
    constexpr uint32_t TICK_INTERVAL_MS = 1000 / SERVER_TICK_RATE;               // 15ms
constexpr uint32_t SNAPSHOT_RATE = 20;                                       // 20 snapshots per second
constexpr float SNAPSHOT_INTERVAL = 1.0f / SNAPSHOT_RATE;                    // 0.05s = 50ms

// === Lobby Configuration ===
constexpr uint8_t MAX_PLAYERS_PER_LOBBY = 4;
constexpr uint8_t MIN_PLAYERS_DUO = 2;
constexpr uint8_t MIN_PLAYERS_TRIO = 3;
constexpr uint8_t MIN_PLAYERS_SQUAD = 4;
constexpr uint32_t LOBBY_COUNTDOWN_DURATION_SECONDS = 5;

// === Gameplay Configuration ===
constexpr float GAME_SCROLL_SPEED = 50.0f;                                   // pixels per second
constexpr float PLAYER_MOVEMENT_SPEED = 200.0f;                              // pixels per second
constexpr uint16_t PLAYER_MAX_HEALTH = 100;
constexpr uint8_t PLAYER_LIVES = 3;
constexpr float PLAYER_WIDTH = 128.0f;                                       // Player hitbox width
constexpr float PLAYER_HEIGHT = 128.0f;                                      // Player hitbox height

// === Projectile Configuration ===
constexpr float PROJECTILE_SPEED = 500.0f;                                   // pixels per second
constexpr float PROJECTILE_WIDTH = 28.0f;
constexpr float PROJECTILE_HEIGHT = 12.0f;
constexpr float PROJECTILE_LIFETIME = 10.0f;                                 // seconds (longer range)
constexpr int PROJECTILE_DAMAGE = 25;                                        // player projectile damage
constexpr int ENEMY_PROJECTILE_DAMAGE = 20;                                  // enemy projectile damage

// === Enemy Configuration ===
constexpr float ENEMY_BASIC_SPEED = 100.0f;                                  // pixels per second
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

// === Screen/World Configuration ===
constexpr float WORLD_WIDTH = 1920.0f;
constexpr float WORLD_HEIGHT = 1080.0f;
constexpr float SCREEN_WIDTH = 1280.0f;
constexpr float SCREEN_HEIGHT = 720.0f;

constexpr float PLAYER_SPAWN_X = 100.0f;
constexpr float PLAYER_SPAWN_Y_BASE = 300.0f;
constexpr float PLAYER_SPAWN_Y_OFFSET = 80.0f;                               // Vertical spacing between players

constexpr float ENTITY_OFFSCREEN_LEFT = -100.0f;                             // When to despawn enemies
constexpr float ENTITY_OFFSCREEN_RIGHT = 2000.0f;

// === Wave Configuration ===
constexpr float DEFAULT_SPAWN_INTERVAL = 2.0f;                               // seconds between spawns
constexpr float DEFAULT_ENEMY_SPAWN_X = 1920.0f;                             // Right edge of screen
constexpr float DEFAULT_ENEMY_SPAWN_Y = 300.0f;

// === Wall Configuration ===
constexpr float WALL_WIDTH = 100.0f;
constexpr float WALL_HEIGHT = 80.0f;

// === Powerup/Bonus Configuration ===
constexpr float BONUS_SIZE = 12.0f;

} // namespace config

} // namespace rtype::server
