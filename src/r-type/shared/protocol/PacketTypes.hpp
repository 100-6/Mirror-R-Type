#pragma once

#include <cstdint>
#include <string>

namespace rtype::protocol {

/**
 * @brief Packet type identifiers
 *
 * Range allocation:
 * - 0x01-0x04: Connection Management (Client → Server)
 * - 0x05-0x09: Lobby & Matchmaking (Client → Server)
 * - 0x10-0x1F: Player Input (Client → Server)
 * - 0x20-0x3F: Reserved for future client-to-server use
 * - 0x81-0x8A: Connection & Lobby (Server → Client)
 * - 0xA0-0xAF: World State (Server → Client)
 * - 0xB0-0xBF: Entity Events (Server → Client)
 * - 0xC0-0xCF: Game Mechanics (Server → Client)
 * - 0xF0-0xFF: System & Chat (Server → Client)
 */
enum class PacketType : uint8_t {
    // ========== Client → Server ==========
    // Connection Management (0x01-0x04)
    CLIENT_CONNECT = 0x01,
    CLIENT_DISCONNECT = 0x02,
    CLIENT_PING = 0x04,

    // Lobby & Matchmaking (0x05-0x09)
    CLIENT_JOIN_LOBBY = 0x05,
    CLIENT_LEAVE_LOBBY = 0x06,

    // Player Input (0x10-0x1F)
    CLIENT_INPUT = 0x10,

    // ========== Server → Client ==========
    // Connection & Lobby (0x81-0x8A)
    SERVER_ACCEPT = 0x81,
    SERVER_REJECT = 0x82,
    SERVER_PLAYER_JOINED = 0x83,
    SERVER_PLAYER_LEFT = 0x84,
    SERVER_PONG = 0x85,
    SERVER_LOBBY_STATE = 0x87,
    SERVER_GAME_START_COUNTDOWN = 0x88,
    SERVER_COUNTDOWN_CANCELLED = 0x89,
    SERVER_GAME_START = 0x8A,

    // World State (0xA0-0xAF)
    SERVER_SNAPSHOT = 0xA0,
    SERVER_DELTA_SNAPSHOT = 0xA1,

    // Entity Events (0xB0-0xBF)
    SERVER_ENTITY_SPAWN = 0xB0,
    SERVER_ENTITY_DESTROY = 0xB1,
    SERVER_ENTITY_DAMAGE = 0xB2,
    SERVER_PROJECTILE_SPAWN = 0xB3,

    // Game Mechanics (0xC0-0xCF)
    SERVER_POWERUP_COLLECTED = 0xC0,
    SERVER_SCORE_UPDATE = 0xC1,
    SERVER_WAVE_START = 0xC2,
    SERVER_WAVE_COMPLETE = 0xC3,
    SERVER_PLAYER_RESPAWN = 0xC5,
    SERVER_GAME_OVER = 0xC6,
};

/**
 * @brief Game mode selection
 */
enum class GameMode : uint8_t {
    DUO = 0x01,
    TRIO = 0x02,
    SQUAD = 0x03,
};

/**
 * @brief Difficulty level selection
 */
enum class Difficulty : uint8_t {
    EASY = 0x01,
    NORMAL = 0x02,
    HARD = 0x03,
};

/**
 * @brief Disconnect reason codes
 */
enum class DisconnectReason : uint8_t {
    USER_QUIT = 0x01,
    TIMEOUT = 0x02,
    ERROR = 0x03,
};

/**
 * @brief Connection rejection reason codes
 */
enum class RejectReason : uint8_t {
    SERVER_FULL = 0x01,
    VERSION_MISMATCH = 0x02,
    BANNED = 0x03,
    MAINTENANCE = 0x04,
};

/**
 * @brief Countdown cancellation reason codes
 */
enum class CountdownCancelReason : uint8_t {
    PLAYER_LEFT = 0x01,
    SERVER_ERROR = 0x02,
};

/**
 * @brief Entity type identifiers
 */
enum class EntityType : uint8_t {
    PLAYER = 0x01,
    ENEMY_BASIC = 0x02,
    ENEMY_ELITE = 0x03,
    ENEMY_BOSS = 0x04,
    PROJECTILE_PLAYER = 0x05,
    PROJECTILE_ENEMY = 0x06,
    POWERUP_WEAPON = 0x07,
    POWERUP_SHIELD = 0x08,
    POWERUP_SCORE = 0x09,
};

/**
 * @brief Entity destruction reason codes
 */
enum class DestroyReason : uint8_t {
    KILLED = 0x01,
    OUT_OF_BOUNDS = 0x02,
    COLLECTED = 0x03,
    TIMEOUT = 0x04,
};

/**
 * @brief Projectile type identifiers
 */
enum class ProjectileType : uint8_t {
    BULLET = 0x01,
    MISSILE = 0x02,
    LASER = 0x03,
    CHARGE_SHOT = 0x04,
};

/**
 * @brief Power-up type identifiers
 */
enum class PowerupType : uint8_t {
    WEAPON_UPGRADE = 0x01,
    SHIELD = 0x02,
    SPEED = 0x03,
    SCORE = 0x04,
};

/**
 * @brief Game result codes
 */
enum class GameResult : uint8_t {
    VICTORY = 0x01,
    DEFEAT = 0x02,
    TIMEOUT = 0x03,
};

/**
 * @brief Input flags bitfield
 *
 * Each bit represents a specific input:
 * - Bit 0: UP
 * - Bit 1: DOWN
 * - Bit 2: LEFT
 * - Bit 3: RIGHT
 * - Bit 4: SHOOT
 * - Bit 5: CHARGE
 * - Bit 6: SPECIAL
 * - Bits 7-15: Reserved (must be 0)
 */
enum InputFlags : uint16_t {
    INPUT_UP = 1 << 0,
    INPUT_DOWN = 1 << 1,
    INPUT_LEFT = 1 << 2,
    INPUT_RIGHT = 1 << 3,
    INPUT_SHOOT = 1 << 4,
    INPUT_CHARGE = 1 << 5,
    INPUT_SPECIAL = 1 << 6,
};

/**
 * @brief Entity state flags bitfield
 *
 * - Bit 0: INVULNERABLE
 * - Bit 1: CHARGING
 * - Bit 2: DAMAGED (visual feedback)
 * - Bits 3-15: Reserved
 */
enum EntityStateFlags : uint16_t {
    ENTITY_INVULNERABLE = 1 << 0,
    ENTITY_CHARGING = 1 << 1,
    ENTITY_DAMAGED = 1 << 2,
};

/**
 * @brief Get required player count for a game mode
 *
 * @param mode Game mode
 * @return Required number of players
 */
inline uint8_t get_required_player_count(GameMode mode) {
    switch (mode) {
    case GameMode::DUO:
        return 2;
    case GameMode::TRIO:
        return 3;
    case GameMode::SQUAD:
        return 4;
    default:
        return 0;
    }
}

/**
 * @brief Convert GameMode to human-readable string
 *
 * @param mode Game mode
 * @return String representation
 */
inline std::string game_mode_to_string(GameMode mode) {
    switch (mode) {
    case GameMode::DUO:
        return "DUO";
    case GameMode::TRIO:
        return "TRIO";
    case GameMode::SQUAD:
        return "SQUAD";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Convert Difficulty to human-readable string
 *
 * @param difficulty Difficulty level
 * @return String representation
 */
inline std::string difficulty_to_string(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::EASY:
        return "EASY";
    case Difficulty::NORMAL:
        return "NORMAL";
    case Difficulty::HARD:
        return "HARD";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Convert PacketType to human-readable string
 *
 * @param type Packet type
 * @return String representation
 */
inline std::string packet_type_to_string(PacketType type) {
    switch (type) {
    case PacketType::CLIENT_CONNECT:
        return "CLIENT_CONNECT";
    case PacketType::CLIENT_DISCONNECT:
        return "CLIENT_DISCONNECT";
    case PacketType::CLIENT_PING:
        return "CLIENT_PING";
    case PacketType::CLIENT_JOIN_LOBBY:
        return "CLIENT_JOIN_LOBBY";
    case PacketType::CLIENT_LEAVE_LOBBY:
        return "CLIENT_LEAVE_LOBBY";
    case PacketType::CLIENT_INPUT:
        return "CLIENT_INPUT";
    case PacketType::SERVER_ACCEPT:
        return "SERVER_ACCEPT";
    case PacketType::SERVER_REJECT:
        return "SERVER_REJECT";
    case PacketType::SERVER_PLAYER_JOINED:
        return "SERVER_PLAYER_JOINED";
    case PacketType::SERVER_PLAYER_LEFT:
        return "SERVER_PLAYER_LEFT";
    case PacketType::SERVER_PONG:
        return "SERVER_PONG";
    case PacketType::SERVER_LOBBY_STATE:
        return "SERVER_LOBBY_STATE";
    case PacketType::SERVER_GAME_START_COUNTDOWN:
        return "SERVER_GAME_START_COUNTDOWN";
    case PacketType::SERVER_COUNTDOWN_CANCELLED:
        return "SERVER_COUNTDOWN_CANCELLED";
    case PacketType::SERVER_GAME_START:
        return "SERVER_GAME_START";
    case PacketType::SERVER_SNAPSHOT:
        return "SERVER_SNAPSHOT";
    case PacketType::SERVER_DELTA_SNAPSHOT:
        return "SERVER_DELTA_SNAPSHOT";
    case PacketType::SERVER_ENTITY_SPAWN:
        return "SERVER_ENTITY_SPAWN";
    case PacketType::SERVER_ENTITY_DESTROY:
        return "SERVER_ENTITY_DESTROY";
    case PacketType::SERVER_ENTITY_DAMAGE:
        return "SERVER_ENTITY_DAMAGE";
    case PacketType::SERVER_PROJECTILE_SPAWN:
        return "SERVER_PROJECTILE_SPAWN";
    case PacketType::SERVER_POWERUP_COLLECTED:
        return "SERVER_POWERUP_COLLECTED";
    case PacketType::SERVER_SCORE_UPDATE:
        return "SERVER_SCORE_UPDATE";
    case PacketType::SERVER_WAVE_START:
        return "SERVER_WAVE_START";
    case PacketType::SERVER_WAVE_COMPLETE:
        return "SERVER_WAVE_COMPLETE";
    case PacketType::SERVER_PLAYER_RESPAWN:
        return "SERVER_PLAYER_RESPAWN";
    case PacketType::SERVER_GAME_OVER:
        return "SERVER_GAME_OVER";
    default:
        return "UNKNOWN";
    }
}

}
