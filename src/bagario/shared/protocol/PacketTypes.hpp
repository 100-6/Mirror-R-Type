#pragma once

#include <cstdint>
#include <string>

namespace bagario::protocol {

/**
 * @brief Packet type identifiers for Bagario
 *
 * Range allocation:
 * - 0x01-0x04: Connection Management (Client -> Server)
 * - 0x10-0x1F: Player Input (Client -> Server)
 * - 0x81-0x8F: Connection Response (Server -> Client)
 * - 0xA0-0xAF: World State (Server -> Client)
 * - 0xB0-0xBF: Entity Events (Server -> Client)
 * - 0xC0-0xCF: Game Events (Server -> Client)
 */
enum class PacketType : uint8_t {
    CLIENT_CONNECT = 0x01,
    CLIENT_DISCONNECT = 0x02,
    CLIENT_PING = 0x04,

    CLIENT_INPUT = 0x10,
    CLIENT_SPLIT = 0x11,
    CLIENT_EJECT_MASS = 0x12,
    CLIENT_SET_SKIN = 0x13,

    SERVER_ACCEPT = 0x81,
    SERVER_REJECT = 0x82,
    SERVER_PONG = 0x85,

    SERVER_SNAPSHOT = 0xA0,

    SERVER_ENTITY_SPAWN = 0xB0,
    SERVER_ENTITY_DESTROY = 0xB1,
    SERVER_CELL_MERGE = 0xB2,

    SERVER_PLAYER_EATEN = 0xC0,
    SERVER_LEADERBOARD = 0xC1,
    SERVER_PLAYER_SKIN = 0xC2,
};

/**
 * @brief Entity type identifiers
 */
enum class EntityType : uint8_t {
    PLAYER_CELL = 0x01,
    FOOD = 0x02,
    VIRUS = 0x03,
    EJECTED_MASS = 0x04,
};

/**
 * @brief Input flags bitfield
 */
enum InputFlags : uint8_t {
    INPUT_SPLIT = 1 << 0,
    INPUT_EJECT_MASS = 1 << 1,
};

/**
 * @brief Disconnect reason codes
 */
enum class DisconnectReason : uint8_t {
    USER_QUIT = 0x01,
    TIMEOUT = 0x02,
    KICKED = 0x03,
    SERVER_SHUTDOWN = 0x04,
};

/**
 * @brief Connection rejection reason codes
 */
enum class RejectReason : uint8_t {
    SERVER_FULL = 0x01,
    VERSION_MISMATCH = 0x02,
    INVALID_NAME = 0x03,
};

/**
 * @brief Entity destruction reason codes
 */
enum class DestroyReason : uint8_t {
    EATEN = 0x01,
    MERGED = 0x02,
    DECAYED = 0x03,
    OUT_OF_BOUNDS = 0x04,
};

/**
 * @brief Convert PacketType to human-readable string
 */
inline std::string packet_type_to_string(PacketType type) {
    switch (type) {
    case PacketType::CLIENT_CONNECT:
        return "CLIENT_CONNECT";
    case PacketType::CLIENT_DISCONNECT:
        return "CLIENT_DISCONNECT";
    case PacketType::CLIENT_PING:
        return "CLIENT_PING";
    case PacketType::CLIENT_INPUT:
        return "CLIENT_INPUT";
    case PacketType::CLIENT_SPLIT:
        return "CLIENT_SPLIT";
    case PacketType::CLIENT_EJECT_MASS:
        return "CLIENT_EJECT_MASS";
    case PacketType::CLIENT_SET_SKIN:
        return "CLIENT_SET_SKIN";
    case PacketType::SERVER_ACCEPT:
        return "SERVER_ACCEPT";
    case PacketType::SERVER_REJECT:
        return "SERVER_REJECT";
    case PacketType::SERVER_PONG:
        return "SERVER_PONG";
    case PacketType::SERVER_SNAPSHOT:
        return "SERVER_SNAPSHOT";
    case PacketType::SERVER_ENTITY_SPAWN:
        return "SERVER_ENTITY_SPAWN";
    case PacketType::SERVER_ENTITY_DESTROY:
        return "SERVER_ENTITY_DESTROY";
    case PacketType::SERVER_CELL_MERGE:
        return "SERVER_CELL_MERGE";
    case PacketType::SERVER_PLAYER_EATEN:
        return "SERVER_PLAYER_EATEN";
    case PacketType::SERVER_LEADERBOARD:
        return "SERVER_LEADERBOARD";
    case PacketType::SERVER_PLAYER_SKIN:
        return "SERVER_PLAYER_SKIN";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Convert EntityType to human-readable string
 */
inline std::string entity_type_to_string(EntityType type) {
    switch (type) {
    case EntityType::PLAYER_CELL:
        return "PLAYER_CELL";
    case EntityType::FOOD:
        return "FOOD";
    case EntityType::VIRUS:
        return "VIRUS";
    case EntityType::EJECTED_MASS:
        return "EJECTED_MASS";
    default:
        return "UNKNOWN";
    }
}

}
