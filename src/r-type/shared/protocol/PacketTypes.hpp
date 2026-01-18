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
 * - 0x20-0x29: Room Management (Client → Server)
 * - 0x30-0x3F: Admin Commands (Client → Server)
 * - 0x81-0x8A: Connection & Lobby (Server → Client)
 * - 0x90-0x9F: Room Management (Server → Client)
 * - 0xA0-0xAF: World State (Server → Client)
 * - 0xB0-0xBF: Entity Events (Server → Client)
 * - 0xC0-0xCF: Game Mechanics (Server → Client)
 * - 0xD0-0xDF: Admin Responses (Server → Client)
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
    CLIENT_UDP_HANDSHAKE = 0x08,  // UDP handshake to associate TCP/UDP connections

    // Player Input (0x10-0x1F)
    CLIENT_INPUT = 0x10,

    // Room Management (0x20-0x29)
    CLIENT_CREATE_ROOM = 0x20,
    CLIENT_JOIN_ROOM = 0x21,
    CLIENT_LEAVE_ROOM = 0x22,
    CLIENT_REQUEST_ROOM_LIST = 0x23,
    CLIENT_START_GAME = 0x24,
    CLIENT_SET_PLAYER_NAME = 0x25,  // Change player name in lobby
    CLIENT_SET_PLAYER_SKIN = 0x26,  // Change player skin in lobby
    CLIENT_REQUEST_GLOBAL_LEADERBOARD = 0x27,  // Request global all-time leaderboard

    // Admin Commands (0x30-0x3F)
    CLIENT_ADMIN_AUTH = 0x30,           // Admin authentication request
    CLIENT_ADMIN_COMMAND = 0x31,        // Admin command execution (kick, list, etc.)

    // Chat (0x40-0x4F)
    CLIENT_CHAT_MESSAGE = 0x40,         // Client sends a chat message

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

    // Room Management (0x90-0x9F)
    SERVER_ROOM_CREATED = 0x90,
    SERVER_ROOM_LIST = 0x91,
    SERVER_ROOM_JOINED = 0x92,
    SERVER_ROOM_LEFT = 0x93,
    SERVER_ROOM_STATE_UPDATE = 0x94,
    SERVER_ROOM_ERROR = 0x95,
    SERVER_PLAYER_NAME_UPDATED = 0x96,  // Player name changed in room
    SERVER_PLAYER_SKIN_UPDATED = 0x97,  // Player skin changed in room

    // World State (0xA0-0xAF)
    SERVER_SNAPSHOT = 0xA0,
    SERVER_DELTA_SNAPSHOT = 0xA1,

    // Entity Events (0xB0-0xBF)
    SERVER_ENTITY_SPAWN = 0xB0,
    SERVER_ENTITY_DESTROY = 0xB1,
    SERVER_ENTITY_DAMAGE = 0xB2,
    SERVER_PROJECTILE_SPAWN = 0xB3,
    SERVER_EXPLOSION_EVENT = 0xB4,

    // Game Mechanics (0xC0-0xCF)
    SERVER_POWERUP_COLLECTED = 0xC0,
    SERVER_SCORE_UPDATE = 0xC1,
    SERVER_WAVE_START = 0xC2,
    SERVER_WAVE_COMPLETE = 0xC3,
    SERVER_PLAYER_LEVEL_UP = 0xC4,  // Player leveled up (ship/weapon changed)
    SERVER_PLAYER_RESPAWN = 0xC5,
    SERVER_GAME_OVER = 0xC6,
    SERVER_LEADERBOARD = 0xC7,      // End-game leaderboard with all player scores
    SERVER_GLOBAL_LEADERBOARD = 0xC8,  // Global all-time top 10 leaderboard
    SERVER_LEVEL_TRANSITION = 0xC9,
    SERVER_LEVEL_READY = 0xCA,  // Level fully loaded, client can stop fading
    SERVER_SHIELD_BROKEN = 0xCB,  // Player's shield was destroyed

    // Admin Responses (0xD0-0xDF)
    SERVER_ADMIN_AUTH_RESULT = 0xD0,    // Admin authentication result
    SERVER_ADMIN_COMMAND_RESULT = 0xD1, // Admin command execution result
    SERVER_ADMIN_NOTIFICATION = 0xD2,   // Admin notifications (player events, etc.)
    SERVER_KICK_NOTIFICATION = 0xD3,    // Kick notification sent before disconnect

    // Chat (0xF0-0xFF)
    SERVER_CHAT_MESSAGE = 0xF0,         // Server broadcasts a chat message to all clients
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
 * @brief Room status codes
 */
enum class RoomStatus : uint8_t {
    WAITING = 0x01,
    IN_PROGRESS = 0x02,
    FINISHED = 0x03,
};

/**
 * @brief Room error codes
 */
enum class RoomError : uint8_t {
    ROOM_NOT_FOUND = 0x01,
    ROOM_FULL = 0x02,
    WRONG_PASSWORD = 0x03,
    ALREADY_STARTED = 0x04,
    NOT_HOST = 0x05,
    INVALID_CONFIGURATION = 0x06,
    ALREADY_IN_ROOM = 0x07,
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
    ENEMY_FAST = 0x0A,
    ENEMY_TANK = 0x0B,
    POWERUP_HEALTH = 0x0C,
    POWERUP_SPEED = 0x0D,
    WALL = 0x0E,
    BONUS_HEALTH = 0x0C,  // Alias for POWERUP_HEALTH
    BONUS_SHIELD = 0x08,  // Alias for POWERUP_SHIELD
    BONUS_SPEED = 0x0D,   // Alias for POWERUP_SPEED
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
    HEALTH = 0x05,
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
 * - Bit 7: SWITCH_WEAPON
 * - Bits 8-15: Reserved (must be 0)
 */
enum InputFlags : uint16_t {
    INPUT_UP = 1 << 0,
    INPUT_DOWN = 1 << 1,
    INPUT_LEFT = 1 << 2,
    INPUT_RIGHT = 1 << 3,
    INPUT_SHOOT = 1 << 4,
    INPUT_CHARGE = 1 << 5,
    INPUT_SPECIAL = 1 << 6,
    INPUT_SWITCH_WEAPON = 1 << 7,
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
    case PacketType::CLIENT_UDP_HANDSHAKE:
        return "CLIENT_UDP_HANDSHAKE";
    case PacketType::CLIENT_INPUT:
        return "CLIENT_INPUT";
    case PacketType::CLIENT_CREATE_ROOM:
        return "CLIENT_CREATE_ROOM";
    case PacketType::CLIENT_JOIN_ROOM:
        return "CLIENT_JOIN_ROOM";
    case PacketType::CLIENT_LEAVE_ROOM:
        return "CLIENT_LEAVE_ROOM";
    case PacketType::CLIENT_REQUEST_ROOM_LIST:
        return "CLIENT_REQUEST_ROOM_LIST";
    case PacketType::CLIENT_START_GAME:
        return "CLIENT_START_GAME";
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
    case PacketType::SERVER_ROOM_CREATED:
        return "SERVER_ROOM_CREATED";
    case PacketType::SERVER_ROOM_LIST:
        return "SERVER_ROOM_LIST";
    case PacketType::SERVER_ROOM_JOINED:
        return "SERVER_ROOM_JOINED";
    case PacketType::SERVER_ROOM_LEFT:
        return "SERVER_ROOM_LEFT";
    case PacketType::SERVER_ROOM_STATE_UPDATE:
        return "SERVER_ROOM_STATE_UPDATE";
    case PacketType::SERVER_ROOM_ERROR:
        return "SERVER_ROOM_ERROR";
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
    case PacketType::SERVER_PLAYER_LEVEL_UP:
        return "SERVER_PLAYER_LEVEL_UP";
    case PacketType::SERVER_PLAYER_RESPAWN:
        return "SERVER_PLAYER_RESPAWN";
    case PacketType::SERVER_GAME_OVER:
        return "SERVER_GAME_OVER";
    case PacketType::SERVER_LEADERBOARD:
        return "SERVER_LEADERBOARD";
    case PacketType::SERVER_GLOBAL_LEADERBOARD:
        return "SERVER_GLOBAL_LEADERBOARD";
    case PacketType::CLIENT_REQUEST_GLOBAL_LEADERBOARD:
        return "CLIENT_REQUEST_GLOBAL_LEADERBOARD";
    case PacketType::SERVER_LEVEL_TRANSITION:
        return "SERVER_LEVEL_TRANSITION";
    case PacketType::SERVER_LEVEL_READY:
        return "SERVER_LEVEL_READY";
    case PacketType::CLIENT_SET_PLAYER_NAME:
        return "CLIENT_SET_PLAYER_NAME";
    case PacketType::CLIENT_SET_PLAYER_SKIN:
        return "CLIENT_SET_PLAYER_SKIN";
    case PacketType::CLIENT_ADMIN_AUTH:
        return "CLIENT_ADMIN_AUTH";
    case PacketType::CLIENT_ADMIN_COMMAND:
        return "CLIENT_ADMIN_COMMAND";
    case PacketType::SERVER_PLAYER_NAME_UPDATED:
        return "SERVER_PLAYER_NAME_UPDATED";
    case PacketType::SERVER_PLAYER_SKIN_UPDATED:
        return "SERVER_PLAYER_SKIN_UPDATED";
    case PacketType::SERVER_ADMIN_AUTH_RESULT:
        return "SERVER_ADMIN_AUTH_RESULT";
    case PacketType::SERVER_ADMIN_COMMAND_RESULT:
        return "SERVER_ADMIN_COMMAND_RESULT";
    case PacketType::SERVER_ADMIN_NOTIFICATION:
        return "SERVER_ADMIN_NOTIFICATION";
    case PacketType::SERVER_KICK_NOTIFICATION:
        return "SERVER_KICK_NOTIFICATION";
    case PacketType::CLIENT_CHAT_MESSAGE:
        return "CLIENT_CHAT_MESSAGE";
    case PacketType::SERVER_CHAT_MESSAGE:
        return "SERVER_CHAT_MESSAGE";
    default:
        return "UNKNOWN";
    }
}

}
