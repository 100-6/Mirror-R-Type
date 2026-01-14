#pragma once

#include "PacketTypes.hpp"
#include "BagarioConfig.hpp"
#include <cstdint>
#include <cstring>

#ifdef _MSC_VER
    #define PACK_START __pragma(pack(push, 1))
    #define PACK_END __pragma(pack(pop))
    #define PACKED
#else
    #define PACK_START
    #define PACK_END
    #define PACKED __attribute__((packed))
#endif

namespace bagario::protocol {

/**
 * @brief CLIENT_CONNECT payload (0x01)
 * Total size: 33 bytes
 */
PACK_START
struct PACKED ClientConnectPayload {
    uint8_t client_version;
    char player_name[32];

    ClientConnectPayload() : client_version(config::PROTOCOL_VERSION) {
        std::memset(player_name, 0, sizeof(player_name));
    }

    void set_player_name(const std::string& name) {
        std::memset(player_name, 0, sizeof(player_name));
        std::strncpy(player_name, name.c_str(), sizeof(player_name) - 1);
    }
};
PACK_END

static_assert(sizeof(ClientConnectPayload) == 33, "ClientConnectPayload must be 33 bytes");

/**
 * @brief CLIENT_DISCONNECT payload (0x02)
 * Total size: 5 bytes
 */
PACK_START
struct PACKED ClientDisconnectPayload {
    uint32_t player_id;
    DisconnectReason reason;

    ClientDisconnectPayload() : player_id(0), reason(DisconnectReason::USER_QUIT) {}
};
PACK_END

static_assert(sizeof(ClientDisconnectPayload) == 5, "ClientDisconnectPayload must be 5 bytes");

/**
 * @brief CLIENT_PING payload (0x04)
 * Total size: 8 bytes
 */
PACK_START
struct PACKED ClientPingPayload {
    uint32_t player_id;
    uint32_t client_timestamp;

    ClientPingPayload() : player_id(0), client_timestamp(0) {}
};
PACK_END

static_assert(sizeof(ClientPingPayload) == 8, "ClientPingPayload must be 8 bytes");

/**
 * @brief CLIENT_INPUT payload (0x10)
 * Player sends mouse position as movement target
 * Total size: 16 bytes
 */
PACK_START
struct PACKED ClientInputPayload {
    uint32_t player_id;
    float target_x;
    float target_y;
    uint32_t sequence;

    ClientInputPayload() : player_id(0), target_x(0), target_y(0), sequence(0) {}
};
PACK_END

static_assert(sizeof(ClientInputPayload) == 16, "ClientInputPayload must be 16 bytes");

/**
 * @brief CLIENT_SPLIT payload (0x11)
 * Player requests to split their cells
 * Total size: 4 bytes
 */
PACK_START
struct PACKED ClientSplitPayload {
    uint32_t player_id;

    ClientSplitPayload() : player_id(0) {}
};
PACK_END

static_assert(sizeof(ClientSplitPayload) == 4, "ClientSplitPayload must be 4 bytes");

/**
 * @brief CLIENT_EJECT_MASS payload (0x12)
 * Player requests to eject mass
 * Total size: 12 bytes
 */
PACK_START
struct PACKED ClientEjectMassPayload {
    uint32_t player_id;
    float direction_x;
    float direction_y;

    ClientEjectMassPayload() : player_id(0), direction_x(0), direction_y(0) {}
};
PACK_END

static_assert(sizeof(ClientEjectMassPayload) == 12, "ClientEjectMassPayload must be 12 bytes");

/**
 * @brief SERVER_ACCEPT payload (0x81)
 * Total size: 18 bytes
 */
PACK_START
struct PACKED ServerAcceptPayload {
    uint32_t assigned_player_id;  // 4 bytes
    float map_width;              // 4 bytes
    float map_height;             // 4 bytes
    float starting_mass;          // 4 bytes
    uint8_t server_tick_rate;     // 1 byte
    uint8_t max_players;          // 1 byte

    ServerAcceptPayload()
        : assigned_player_id(0)
        , map_width(config::MAP_WIDTH)
        , map_height(config::MAP_HEIGHT)
        , starting_mass(config::STARTING_MASS)
        , server_tick_rate(config::TICK_RATE)
        , max_players(config::MAX_PLAYERS) {}
};
PACK_END

static_assert(sizeof(ServerAcceptPayload) == 18, "ServerAcceptPayload must be 18 bytes");

/**
 * @brief SERVER_REJECT payload (0x82)
 * Total size: 65 bytes
 */
PACK_START
struct PACKED ServerRejectPayload {
    RejectReason reason_code;
    char reason_message[64];

    ServerRejectPayload() : reason_code(RejectReason::SERVER_FULL) {
        std::memset(reason_message, 0, sizeof(reason_message));
    }

    void set_message(const std::string& message) {
        std::memset(reason_message, 0, sizeof(reason_message));
        std::strncpy(reason_message, message.c_str(), sizeof(reason_message) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerRejectPayload) == 65, "ServerRejectPayload must be 65 bytes");

/**
 * @brief SERVER_PONG payload (0x85)
 * Total size: 8 bytes
 */
PACK_START
struct PACKED ServerPongPayload {
    uint32_t client_timestamp;
    uint32_t server_timestamp;

    ServerPongPayload() : client_timestamp(0), server_timestamp(0) {}
};
PACK_END

static_assert(sizeof(ServerPongPayload) == 8, "ServerPongPayload must be 8 bytes");

/**
 * @brief Entity state in SERVER_SNAPSHOT
 * Size: 25 bytes
 */
PACK_START
struct PACKED EntityState {
    uint32_t entity_id;
    EntityType entity_type;
    float position_x;
    float position_y;
    float mass;
    uint32_t color;
    uint32_t owner_id;

    EntityState()
        : entity_id(0)
        , entity_type(EntityType::FOOD)
        , position_x(0.0f)
        , position_y(0.0f)
        , mass(1.0f)
        , color(0xFFFFFFFF)
        , owner_id(0) {}
};
PACK_END

static_assert(sizeof(EntityState) == 25, "EntityState must be 25 bytes");

/**
 * @brief SERVER_SNAPSHOT payload header (0xA0)
 * Base size: 6 bytes + (25 * entity_count) bytes
 */
PACK_START
struct PACKED ServerSnapshotPayload {
    uint32_t server_tick;
    uint16_t entity_count;

    ServerSnapshotPayload() : server_tick(0), entity_count(0) {}
};
PACK_END

static_assert(sizeof(ServerSnapshotPayload) == 6, "ServerSnapshotPayload base must be 6 bytes");

/**
 * @brief SERVER_ENTITY_SPAWN payload (0xB0)
 * Total size: 29 bytes
 */
PACK_START
struct PACKED ServerEntitySpawnPayload {
    uint32_t entity_id;
    EntityType entity_type;
    float spawn_x;
    float spawn_y;
    float mass;
    uint32_t color;
    uint32_t owner_id;
    char owner_name[4];

    ServerEntitySpawnPayload()
        : entity_id(0)
        , entity_type(EntityType::FOOD)
        , spawn_x(0.0f)
        , spawn_y(0.0f)
        , mass(1.0f)
        , color(0xFFFFFFFF)
        , owner_id(0) {
        std::memset(owner_name, 0, sizeof(owner_name));
    }
};
PACK_END

static_assert(sizeof(ServerEntitySpawnPayload) == 29, "ServerEntitySpawnPayload must be 29 bytes");

/**
 * @brief SERVER_ENTITY_DESTROY payload (0xB1)
 * Total size: 17 bytes
 */
PACK_START
struct PACKED ServerEntityDestroyPayload {
    uint32_t entity_id;
    DestroyReason reason;
    float position_x;
    float position_y;
    uint32_t killer_id;

    ServerEntityDestroyPayload()
        : entity_id(0)
        , reason(DestroyReason::EATEN)
        , position_x(0.0f)
        , position_y(0.0f)
        , killer_id(0) {}
};
PACK_END

static_assert(sizeof(ServerEntityDestroyPayload) == 17, "ServerEntityDestroyPayload must be 17 bytes");

/**
 * @brief SERVER_PLAYER_EATEN payload (0xC0)
 * Sent when a player loses all their cells
 * Total size: 12 bytes
 */
PACK_START
struct PACKED ServerPlayerEatenPayload {
    uint32_t player_id;
    uint32_t killer_id;
    float final_mass;

    ServerPlayerEatenPayload()
        : player_id(0)
        , killer_id(0)
        , final_mass(0.0f) {}
};
PACK_END

static_assert(sizeof(ServerPlayerEatenPayload) == 12, "ServerPlayerEatenPayload must be 12 bytes");

/**
 * @brief Leaderboard entry
 * Size: 40 bytes
 */
PACK_START
struct PACKED LeaderboardEntry {
    uint32_t player_id;
    char player_name[32];
    float total_mass;

    LeaderboardEntry() : player_id(0), total_mass(0.0f) {
        std::memset(player_name, 0, sizeof(player_name));
    }

    void set_name(const std::string& name) {
        std::memset(player_name, 0, sizeof(player_name));
        std::strncpy(player_name, name.c_str(), sizeof(player_name) - 1);
    }
};
PACK_END

static_assert(sizeof(LeaderboardEntry) == 40, "LeaderboardEntry must be 40 bytes");

/**
 * @brief SERVER_LEADERBOARD payload (0xC1)
 * Base size: 1 byte + (40 * entry_count) bytes
 * Max 10 entries = 401 bytes max
 */
PACK_START
struct PACKED ServerLeaderboardPayload {
    uint8_t entry_count;

    ServerLeaderboardPayload() : entry_count(0) {}
};
PACK_END

static_assert(sizeof(ServerLeaderboardPayload) == 1, "ServerLeaderboardPayload base must be 1 byte");

}
