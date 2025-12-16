#pragma once

#include "PacketTypes.hpp"
#include "NetworkConfig.hpp"
#include <cstdint>
#include <cstring>

// Cross-platform packed struct support
#ifdef _MSC_VER
    #define PACK_START __pragma(pack(push, 1))
    #define PACK_END __pragma(pack(pop))
    #define PACKED
#else
    #define PACK_START
    #define PACK_END
    #define PACKED __attribute__((packed))
#endif

namespace rtype::protocol {

/**
 * @brief CLIENT_CONNECT payload (0x01)
 * Total size: 33 bytes
 */
PACK_START
struct PACKED ClientConnectPayload {
    uint8_t client_version;
    char player_name[32];

    ClientConnectPayload() : client_version(0x01) {
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
 * @brief SERVER_ACCEPT payload (0x81)
 * Total size: 8 bytes
 */
PACK_START
struct PACKED ServerAcceptPayload {
    uint32_t assigned_player_id;
    uint8_t server_tick_rate;
    uint8_t max_players;
    uint16_t map_id;

    ServerAcceptPayload()
        : assigned_player_id(0), server_tick_rate(60), max_players(4), map_id(0) {}
};
PACK_END

static_assert(sizeof(ServerAcceptPayload) == 8, "ServerAcceptPayload must be 8 bytes");

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
 * @brief CLIENT_JOIN_LOBBY payload (0x05)
 * Total size: 6 bytes
 */
PACK_START
struct PACKED ClientJoinLobbyPayload {
    uint32_t player_id;
    GameMode game_mode;
    Difficulty difficulty;

    ClientJoinLobbyPayload()
        : player_id(0), game_mode(GameMode::SQUAD), difficulty(Difficulty::NORMAL) {}
};
PACK_END

static_assert(sizeof(ClientJoinLobbyPayload) == 6, "ClientJoinLobbyPayload must be 6 bytes");

/**
 * @brief CLIENT_UDP_HANDSHAKE payload (0x08)
 * Sent via UDP to associate TCP and UDP connections
 * Total size: 8 bytes
 */
PACK_START
struct PACKED ClientUdpHandshakePayload {
    uint32_t player_id;
    uint32_t session_id;

    ClientUdpHandshakePayload() : player_id(0), session_id(0) {}
};
PACK_END

static_assert(sizeof(ClientUdpHandshakePayload) == 8, "ClientUdpHandshakePayload must be 8 bytes");

/**
 * @brief CLIENT_LEAVE_LOBBY payload (0x06)
 * Total size: 8 bytes
 */
PACK_START
struct PACKED ClientLeaveLobbyPayload {
    uint32_t player_id;
    uint32_t lobby_id;

    ClientLeaveLobbyPayload() : player_id(0), lobby_id(0) {}
};
PACK_END

static_assert(sizeof(ClientLeaveLobbyPayload) == 8, "ClientLeaveLobbyPayload must be 8 bytes");

/**
 * @brief Player entry in SERVER_LOBBY_STATE
 * Size: 38 bytes
 */
PACK_START
struct PACKED PlayerLobbyEntry {
    uint32_t player_id;
    char player_name[32];
    uint16_t player_level;

    PlayerLobbyEntry() : player_id(0), player_level(0) {
        std::memset(player_name, 0, sizeof(player_name));
    }

    void set_name(const std::string& name) {
        std::memset(player_name, 0, sizeof(player_name));
        std::strncpy(player_name, name.c_str(), sizeof(player_name) - 1);
    }
};
PACK_END

static_assert(sizeof(PlayerLobbyEntry) == 38, "PlayerLobbyEntry must be 38 bytes");

/**
 * @brief SERVER_LOBBY_STATE payload header (0x87)
 * Base size: 8 bytes + (38 × player_count) bytes
 */
PACK_START
struct PACKED ServerLobbyStatePayload {
    uint32_t lobby_id;
    GameMode game_mode;
    Difficulty difficulty;
    uint8_t current_player_count;
    uint8_t required_player_count;

    ServerLobbyStatePayload()
        : lobby_id(0)
        , game_mode(GameMode::SQUAD)
        , difficulty(Difficulty::NORMAL)
        , current_player_count(0)
        , required_player_count(4) {}
};
PACK_END

static_assert(sizeof(ServerLobbyStatePayload) == 8, "ServerLobbyStatePayload base must be 8 bytes");

/**
 * @brief SERVER_GAME_START_COUNTDOWN payload (0x88)
 * Total size: 9 bytes
 */
PACK_START
struct PACKED ServerGameStartCountdownPayload {
    uint32_t lobby_id;
    uint8_t countdown_value;
    GameMode game_mode;
    Difficulty difficulty;
    uint16_t map_id;

    ServerGameStartCountdownPayload()
        : lobby_id(0)
        , countdown_value(5)
        , game_mode(GameMode::SQUAD)
        , difficulty(Difficulty::NORMAL)
        , map_id(0) {}
};
PACK_END

static_assert(sizeof(ServerGameStartCountdownPayload) == 9, "ServerGameStartCountdownPayload must be 9 bytes");

/**
 * @brief SERVER_COUNTDOWN_CANCELLED payload (0x89)
 * Total size: 7 bytes
 */
PACK_START
struct PACKED ServerCountdownCancelledPayload {
    uint32_t lobby_id;
    CountdownCancelReason reason;
    uint8_t new_player_count;
    uint8_t required_count;

    ServerCountdownCancelledPayload()
        : lobby_id(0)
        , reason(CountdownCancelReason::PLAYER_LEFT)
        , new_player_count(0)
        , required_count(4) {}
};
PACK_END

static_assert(sizeof(ServerCountdownCancelledPayload) == 7, "ServerCountdownCancelledPayload must be 7 bytes");

/**
 * @brief Player spawn data in SERVER_GAME_START
 * Size: 12 bytes
 */
PACK_START
struct PACKED PlayerSpawnData {
    uint32_t player_id;
    float spawn_x;
    float spawn_y;

    PlayerSpawnData() : player_id(0), spawn_x(0.0f), spawn_y(0.0f) {}
};
PACK_END

static_assert(sizeof(PlayerSpawnData) == 12, "PlayerSpawnData must be 12 bytes");

/**
 * @brief SERVER_GAME_START payload header (0x8A)
 * Base size: 16 bytes + (12 × player_count) bytes
 * Contains UDP port for gameplay communication
 */
PACK_START
struct PACKED ServerGameStartPayload {
    uint32_t game_session_id;
    GameMode game_mode;
    Difficulty difficulty;
    uint32_t server_tick;
    uint32_t level_seed;
    uint16_t udp_port;  // UDP port for gameplay communication

    ServerGameStartPayload()
        : game_session_id(0)
        , game_mode(GameMode::SQUAD)
        , difficulty(Difficulty::NORMAL)
        , server_tick(0)
        , level_seed(0)
        , udp_port(config::DEFAULT_UDP_PORT) {}
};
PACK_END

static_assert(sizeof(ServerGameStartPayload) == 16, "ServerGameStartPayload base must be 16 bytes");

/**
 * @brief CLIENT_INPUT payload (0x10)
 * Total size: 10 bytes
 */
PACK_START
struct PACKED ClientInputPayload {
    uint32_t player_id;
    uint16_t input_flags;
    uint32_t client_tick;

    ClientInputPayload() : player_id(0), input_flags(0), client_tick(0) {}

    bool is_up_pressed() const { return (input_flags & INPUT_UP) != 0; }
    bool is_down_pressed() const { return (input_flags & INPUT_DOWN) != 0; }
    bool is_left_pressed() const { return (input_flags & INPUT_LEFT) != 0; }
    bool is_right_pressed() const { return (input_flags & INPUT_RIGHT) != 0; }
    bool is_shoot_pressed() const { return (input_flags & INPUT_SHOOT) != 0; }
    bool is_charge_pressed() const { return (input_flags & INPUT_CHARGE) != 0; }
    bool is_special_pressed() const { return (input_flags & INPUT_SPECIAL) != 0; }
};
PACK_END

static_assert(sizeof(ClientInputPayload) == 10, "ClientInputPayload must be 10 bytes");

/**
 * @brief Entity state in SERVER_SNAPSHOT
 * Size: 21 bytes
 */
PACK_START
struct PACKED EntityState {
    uint32_t entity_id;
    EntityType entity_type;
    float position_x;
    float position_y;
    int16_t velocity_x;
    int16_t velocity_y;
    uint16_t health;
    uint16_t flags;

    EntityState()
        : entity_id(0)
        , entity_type(EntityType::PLAYER)
        , position_x(0.0f)
        , position_y(0.0f)
        , velocity_x(0)
        , velocity_y(0)
        , health(100)
        , flags(0) {}

    bool is_invulnerable() const { return (flags & ENTITY_INVULNERABLE) != 0; }
    bool is_charging() const { return (flags & ENTITY_CHARGING) != 0; }
    bool is_damaged() const { return (flags & ENTITY_DAMAGED) != 0; }
};
PACK_END

static_assert(sizeof(EntityState) == 21, "EntityState must be 21 bytes");

/**
 * @brief SERVER_SNAPSHOT payload header (0xA0)
 * Base size: 6 bytes + (21 × entity_count) bytes
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
 * @brief Enemy subtype identifiers for SERVER_ENTITY_SPAWN
 */
enum class EnemySubtype : uint8_t {
    BASIC = 0x00,
    FAST = 0x01,
    TANK = 0x02,
    BOSS = 0x03,
};

/**
 * @brief SERVER_ENTITY_SPAWN payload (0xB0)
 * Total size: 16 bytes (enhanced with subtype and health)
 * Layout: entity_id(4) + entity_type(1) + spawn_x(4) + spawn_y(4) + subtype(1) + health(2) = 16 bytes
 */
PACK_START
struct PACKED ServerEntitySpawnPayload {
    uint32_t entity_id;
    EntityType entity_type;
    float spawn_x;
    float spawn_y;
    uint8_t subtype;
    uint16_t health;

    ServerEntitySpawnPayload()
        : entity_id(0)
        , entity_type(EntityType::ENEMY_BASIC)
        , spawn_x(0.0f)
        , spawn_y(0.0f)
        , subtype(static_cast<uint8_t>(EnemySubtype::BASIC))
        , health(100) {}
};
PACK_END

static_assert(sizeof(ServerEntitySpawnPayload) == 16, "ServerEntitySpawnPayload must be 16 bytes");

/**
 * @brief SERVER_ENTITY_DESTROY payload (0xB1)
 * Total size: 13 bytes
 */
PACK_START
struct PACKED ServerEntityDestroyPayload {
    uint32_t entity_id;
    DestroyReason reason;
    float position_x;
    float position_y;

    ServerEntityDestroyPayload()
        : entity_id(0), reason(DestroyReason::KILLED), position_x(0.0f), position_y(0.0f) {}
};
PACK_END

static_assert(sizeof(ServerEntityDestroyPayload) == 13, "ServerEntityDestroyPayload must be 13 bytes");

/**
 * @brief SERVER_PROJECTILE_SPAWN payload (0xB3)
 * Total size: 21 bytes
 */
PACK_START
struct PACKED ServerProjectileSpawnPayload {
    uint32_t projectile_id;
    uint32_t owner_id;
    ProjectileType projectile_type;
    float spawn_x;
    float spawn_y;
    int16_t velocity_x;
    int16_t velocity_y;

    ServerProjectileSpawnPayload()
        : projectile_id(0)
        , owner_id(0)
        , projectile_type(ProjectileType::BULLET)
        , spawn_x(0.0f)
        , spawn_y(0.0f)
        , velocity_x(0)
        , velocity_y(0) {}
};
PACK_END

static_assert(sizeof(ServerProjectileSpawnPayload) == 21, "ServerProjectileSpawnPayload must be 21 bytes");

/**
 * @brief SERVER_POWERUP_COLLECTED payload (0xC0)
 * Total size: 6 bytes
 */
PACK_START
struct PACKED ServerPowerupCollectedPayload {
    uint32_t player_id;
    PowerupType powerup_type;
    uint8_t new_weapon_level;

    ServerPowerupCollectedPayload()
        : player_id(0), powerup_type(PowerupType::WEAPON_UPGRADE), new_weapon_level(1) {}
};
PACK_END

static_assert(sizeof(ServerPowerupCollectedPayload) == 6, "ServerPowerupCollectedPayload must be 6 bytes");

/**
 * @brief SERVER_SCORE_UPDATE payload (0xC1)
 * Total size: 13 bytes
 */
PACK_START
struct PACKED ServerScoreUpdatePayload {
    uint32_t player_id;
    int32_t score_delta;
    uint32_t new_total_score;
    uint8_t combo_multiplier;

    ServerScoreUpdatePayload()
        : player_id(0), score_delta(0), new_total_score(0), combo_multiplier(1) {}
};
PACK_END

static_assert(sizeof(ServerScoreUpdatePayload) == 13, "ServerScoreUpdatePayload must be 13 bytes");

/**
 * @brief SERVER_WAVE_START payload (0xC2)
 * Total size: 44 bytes
 */
PACK_START
struct PACKED ServerWaveStartPayload {
    uint32_t wave_number;
    uint16_t total_waves;
    float scroll_distance;
    uint16_t expected_enemies;
    char wave_name[32];

    ServerWaveStartPayload()
        : wave_number(0), total_waves(0), scroll_distance(0.0f), expected_enemies(0) {
        std::memset(wave_name, 0, sizeof(wave_name));
    }

    void set_wave_name(const std::string& name) {
        std::memset(wave_name, 0, sizeof(wave_name));
        std::strncpy(wave_name, name.c_str(), sizeof(wave_name) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerWaveStartPayload) == 44, "ServerWaveStartPayload must be 44 bytes");

/**
 * @brief SERVER_WAVE_COMPLETE payload (0xC3)
 * Total size: 13 bytes
 */
PACK_START
struct PACKED ServerWaveCompletePayload {
    uint32_t wave_number;
    uint32_t completion_time;
    uint16_t enemies_killed;
    uint16_t bonus_points;
    uint8_t all_waves_complete;

    ServerWaveCompletePayload()
        : wave_number(0)
        , completion_time(0)
        , enemies_killed(0)
        , bonus_points(0)
        , all_waves_complete(0) {}
};
PACK_END

static_assert(sizeof(ServerWaveCompletePayload) == 13, "ServerWaveCompletePayload must be 13 bytes");

/**
 * @brief SERVER_PLAYER_RESPAWN payload (0xC5)
 * Total size: 15 bytes
 */
PACK_START
struct PACKED ServerPlayerRespawnPayload {
    uint32_t player_id;
    float respawn_x;
    float respawn_y;
    uint16_t invulnerability_duration;
    uint8_t lives_remaining;

    ServerPlayerRespawnPayload()
        : player_id(0)
        , respawn_x(0.0f)
        , respawn_y(0.0f)
        , invulnerability_duration(3000)
        , lives_remaining(3) {}
};
PACK_END

static_assert(sizeof(ServerPlayerRespawnPayload) == 15, "ServerPlayerRespawnPayload must be 15 bytes");

/**
 * @brief Score entry in SERVER_GAME_OVER
 * Size: 12 bytes
 */
PACK_START
struct PACKED FinalScoreEntry {
    uint32_t player_id;
    uint32_t final_score;
    uint16_t deaths;
    uint16_t kills;

    FinalScoreEntry() : player_id(0), final_score(0), deaths(0), kills(0) {}
};
PACK_END

static_assert(sizeof(FinalScoreEntry) == 12, "FinalScoreEntry must be 12 bytes");

/**
 * @brief SERVER_GAME_OVER payload header (0xC6)
 * Base size: 9 bytes + (12 × player_count) bytes
 */
PACK_START
struct PACKED ServerGameOverPayload {
    GameResult result;
    uint32_t total_time;
    uint32_t enemies_killed;

    ServerGameOverPayload()
        : result(GameResult::VICTORY), total_time(0), enemies_killed(0) {}
};
PACK_END

static_assert(sizeof(ServerGameOverPayload) == 9, "ServerGameOverPayload base must be 9 bytes");

}
