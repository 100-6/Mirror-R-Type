#pragma once

#include <cstdint>
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
constexpr uint16_t DEFAULT_SERVER_PORT = 4242;
constexpr uint32_t MAX_CLIENTS = 16;
constexpr uint32_t MAX_PACKET_SIZE = 1400;

// === Timing Configuration ===
constexpr uint32_t SERVER_TICK_RATE = 32;                                    // 32 ticks per second
constexpr float TICK_INTERVAL = 1.0f / SERVER_TICK_RATE;                     // ~31.25ms per tick
constexpr uint32_t TICK_INTERVAL_MS = 1000 / SERVER_TICK_RATE;               // 31ms

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

// === Enemy Configuration ===
constexpr float ENEMY_BASIC_SPEED = 100.0f;                                  // pixels per second
constexpr uint16_t ENEMY_BASIC_HEALTH = 50;

constexpr float ENEMY_FAST_SPEED = 200.0f;
constexpr uint16_t ENEMY_FAST_HEALTH = 30;

constexpr float ENEMY_TANK_SPEED = 50.0f;
constexpr uint16_t ENEMY_TANK_HEALTH = 150;

constexpr float ENEMY_BOSS_SPEED = 30.0f;
constexpr uint16_t ENEMY_BOSS_HEALTH = 500;

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

} // namespace config

} // namespace rtype::server
