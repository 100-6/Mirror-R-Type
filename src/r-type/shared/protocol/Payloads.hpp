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
        : assigned_player_id(0), server_tick_rate(64), max_players(4), map_id(0) {}
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
 * Size: 39 bytes
 */
PACK_START
struct PACKED PlayerLobbyEntry {
    uint32_t player_id;
    char player_name[32];
    uint16_t player_level;
    uint8_t skin_id;  // 0-14 (3 colors x 5 ship types)

    PlayerLobbyEntry() : player_id(0), player_level(0), skin_id(0) {
        std::memset(player_name, 0, sizeof(player_name));
    }

    void set_name(const std::string& name) {
        std::memset(player_name, 0, sizeof(player_name));
        std::strncpy(player_name, name.c_str(), sizeof(player_name) - 1);
    }
};
PACK_END

static_assert(sizeof(PlayerLobbyEntry) == 39, "PlayerLobbyEntry must be 39 bytes");

/**
 * @brief SERVER_LOBBY_STATE payload header (0x87)
 * Base size: 8 bytes + (39 × player_count) bytes
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
 * Base size: 18 bytes + (12 × player_count) bytes
 * Contains UDP port for gameplay communication and map_id
 */
PACK_START
struct PACKED ServerGameStartPayload {
    uint32_t game_session_id;
    GameMode game_mode;
    Difficulty difficulty;
    uint32_t server_tick;
    uint32_t level_seed;
    uint16_t udp_port;  // UDP port for gameplay communication
    uint16_t map_id;    // Map identifier (1=Nebula, 2=Asteroid, 3=Bydo)
    float scroll_speed;

    ServerGameStartPayload()
        : game_session_id(0)
        , game_mode(GameMode::SQUAD)
        , difficulty(Difficulty::NORMAL)
        , server_tick(0)
        , level_seed(0)
        , udp_port(config::DEFAULT_UDP_PORT)
        , map_id(1)
        , scroll_speed(60.0f) {}
};
PACK_END

static_assert(sizeof(ServerGameStartPayload) == 22, "ServerGameStartPayload base must be 22 bytes");

/**
 * @brief CLIENT_INPUT payload (0x10)
 * Total size: 14 bytes (added sequence_number for lag compensation)
 */
PACK_START
struct PACKED ClientInputPayload {
    uint32_t player_id;
    uint16_t input_flags;
    uint32_t client_tick;
    uint32_t sequence_number;  // For client prediction and reconciliation

    ClientInputPayload() : player_id(0), input_flags(0), client_tick(0), sequence_number(0) {}

    bool is_up_pressed() const { return (input_flags & INPUT_UP) != 0; }
    bool is_down_pressed() const { return (input_flags & INPUT_DOWN) != 0; }
    bool is_left_pressed() const { return (input_flags & INPUT_LEFT) != 0; }
    bool is_right_pressed() const { return (input_flags & INPUT_RIGHT) != 0; }
    bool is_shoot_pressed() const { return (input_flags & INPUT_SHOOT) != 0; }
    bool is_charge_pressed() const { return (input_flags & INPUT_CHARGE) != 0; }
    bool is_special_pressed() const { return (input_flags & INPUT_SPECIAL) != 0; }
    bool is_switch_weapon_pressed() const { return (input_flags & INPUT_SWITCH_WEAPON) != 0; }
};
PACK_END

static_assert(sizeof(ClientInputPayload) == 14, "ClientInputPayload must be 14 bytes");

/**
 * @brief Entity state in SERVER_SNAPSHOT
 * Size: 25 bytes (added last_ack_sequence for lag compensation)
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
    uint32_t last_ack_sequence;  // Last processed input sequence (0 for non-player entities)

    EntityState()
        : entity_id(0)
        , entity_type(EntityType::PLAYER)
        , position_x(0.0f)
        , position_y(0.0f)
        , velocity_x(0)
        , velocity_y(0)
        , health(100)
        , flags(0)
        , last_ack_sequence(0) {}

    bool is_invulnerable() const { return (flags & ENTITY_INVULNERABLE) != 0; }
    bool is_charging() const { return (flags & ENTITY_CHARGING) != 0; }
    bool is_damaged() const { return (flags & ENTITY_DAMAGED) != 0; }
};
PACK_END

static_assert(sizeof(EntityState) == 25, "EntityState must be 25 bytes");

/**
 * @brief SERVER_SNAPSHOT payload header (0xA0)
 * Base size: 10 bytes + (25 × entity_count) bytes
 */
PACK_START
struct PACKED ServerSnapshotPayload {
    uint32_t server_tick;
    uint16_t entity_count;
    float scroll_x;  // Current map scroll position for client synchronization

    ServerSnapshotPayload() : server_tick(0), entity_count(0), scroll_x(0.0f) {}
};
PACK_END

static_assert(sizeof(ServerSnapshotPayload) == 10, "ServerSnapshotPayload base must be 10 bytes");

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
 * @brief SERVER_EXPLOSION_EVENT payload (0xB4)
 * Total size: 16 bytes
 */
PACK_START
struct PACKED ServerExplosionPayload {
    uint32_t source_entity_id;
    float position_x;
    float position_y;
    float effect_scale;

    ServerExplosionPayload()
        : source_entity_id(0)
        , position_x(0.0f)
        , position_y(0.0f)
        , effect_scale(1.0f) {}
};
PACK_END

static_assert(sizeof(ServerExplosionPayload) == 16, "ServerExplosionPayload must be 16 bytes");

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
    uint32_t player_id;          // Network player ID (for identification)
    uint32_t entity_id;          // Server entity ID (for client lookup)
    int32_t score_delta;
    uint32_t new_total_score;
    uint8_t combo_multiplier;

    ServerScoreUpdatePayload()
        : player_id(0), entity_id(0), score_delta(0), new_total_score(0), combo_multiplier(1) {}
};
PACK_END

static_assert(sizeof(ServerScoreUpdatePayload) == 17, "ServerScoreUpdatePayload must be 17 bytes");

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
 * @brief SERVER_PLAYER_LEVEL_UP payload (0xC4)
 * Broadcast when a player levels up (ship and weapon change)
 * Total size: 16 bytes
 */
PACK_START
struct PACKED ServerPlayerLevelUpPayload {
    uint32_t player_id;           // Network player ID
    uint32_t entity_id;           // Server entity ID
    uint8_t new_level;            // New level (1-5)
    uint8_t new_ship_type;        // Ship type (0-4: SCOUT to CARRIER)
    uint8_t new_weapon_type;      // Weapon type (0-4: BASIC to CHARGE)
    uint8_t new_skin_id;          // Computed skin_id (color * 5 + ship_type)
    uint32_t current_score;       // Player's current score

    ServerPlayerLevelUpPayload()
        : player_id(0)
        , entity_id(0)
        , new_level(1)
        , new_ship_type(0)
        , new_weapon_type(0)
        , new_skin_id(0)
        , current_score(0) {}
};
PACK_END

static_assert(sizeof(ServerPlayerLevelUpPayload) == 16, "ServerPlayerLevelUpPayload must be 16 bytes");

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

/**
 * @brief Leaderboard entry for SERVER_LEADERBOARD
 * Contains player info and stats for end-game display
 * Size: 48 bytes
 */
PACK_START
struct PACKED LeaderboardEntry {
    uint32_t player_id;
    char player_name[32];
    uint32_t score;
    uint16_t kills;
    uint16_t deaths;
    uint8_t rank;
    uint8_t padding[3];  // Align to 48 bytes

    LeaderboardEntry() : player_id(0), score(0), kills(0), deaths(0), rank(0) {
        std::memset(player_name, 0, sizeof(player_name));
        std::memset(padding, 0, sizeof(padding));
    }

    void set_name(const std::string& name) {
        std::memset(player_name, 0, sizeof(player_name));
        std::strncpy(player_name, name.c_str(), sizeof(player_name) - 1);
    }
};
PACK_END

static_assert(sizeof(LeaderboardEntry) == 48, "LeaderboardEntry must be 48 bytes");

/**
 * @brief SERVER_LEADERBOARD payload header (0xC7)
 * Sent at end-game with all player scores
 * Base size: 2 bytes + (48 × entry_count) bytes
 */
PACK_START
struct PACKED ServerLeaderboardPayload {
    uint8_t entry_count;
    uint8_t is_final;     // 1 if game is over, 0 if in-game update

    ServerLeaderboardPayload() : entry_count(0), is_final(1) {}
};
PACK_END

static_assert(sizeof(ServerLeaderboardPayload) == 2, "ServerLeaderboardPayload must be 2 bytes");

/**
 * @brief Global leaderboard entry for all-time top scores
 * Size: 40 bytes
 */
PACK_START
struct PACKED GlobalLeaderboardEntry {
    char player_name[32];
    uint32_t score;
    uint32_t timestamp;

    GlobalLeaderboardEntry() : score(0), timestamp(0) {
        std::memset(player_name, 0, sizeof(player_name));
    }

    void set_name(const std::string& name) {
        std::memset(player_name, 0, sizeof(player_name));
        std::strncpy(player_name, name.c_str(), sizeof(player_name) - 1);
    }
};
PACK_END

static_assert(sizeof(GlobalLeaderboardEntry) == 40, "GlobalLeaderboardEntry must be 40 bytes");

/**
 * @brief SERVER_GLOBAL_LEADERBOARD payload header (0xC8)
 * Sent in response to CLIENT_REQUEST_GLOBAL_LEADERBOARD
 * Base size: 1 byte + (40 × entry_count) bytes
 */
PACK_START
struct PACKED ServerGlobalLeaderboardPayload {
    uint8_t entry_count;

    ServerGlobalLeaderboardPayload() : entry_count(0) {}
};
PACK_END

static_assert(sizeof(ServerGlobalLeaderboardPayload) == 1, "ServerGlobalLeaderboardPayload must be 1 byte");

/**
 * @brief CLIENT_CREATE_ROOM payload (0x20)
 * Total size: 105 bytes
 */
PACK_START
struct PACKED ClientCreateRoomPayload {
    uint32_t player_id;
    char room_name[32];
    char password_hash[64];
    GameMode game_mode;
    Difficulty difficulty;
    uint16_t map_id;
    uint8_t max_players;  // Maximum players (2-4)

    ClientCreateRoomPayload()
        : player_id(0)
        , game_mode(GameMode::SQUAD)
        , difficulty(Difficulty::NORMAL)
        , map_id(0)
        , max_players(4) {
        std::memset(room_name, 0, sizeof(room_name));
        std::memset(password_hash, 0, sizeof(password_hash));
    }

    void set_room_name(const std::string& name) {
        std::memset(room_name, 0, sizeof(room_name));
        std::strncpy(room_name, name.c_str(), sizeof(room_name) - 1);
    }

    void set_password_hash(const std::string& hash) {
        std::memset(password_hash, 0, sizeof(password_hash));
        std::strncpy(password_hash, hash.c_str(), sizeof(password_hash) - 1);
    }
};
PACK_END

static_assert(sizeof(ClientCreateRoomPayload) == 105, "ClientCreateRoomPayload must be 105 bytes");

/**
 * @brief CLIENT_JOIN_ROOM payload (0x21)
 * Total size: 72 bytes
 */
PACK_START
struct PACKED ClientJoinRoomPayload {
    uint32_t player_id;
    uint32_t room_id;
    char password_hash[64];

    ClientJoinRoomPayload() : player_id(0), room_id(0) {
        std::memset(password_hash, 0, sizeof(password_hash));
    }

    void set_password_hash(const std::string& hash) {
        std::memset(password_hash, 0, sizeof(password_hash));
        std::strncpy(password_hash, hash.c_str(), sizeof(password_hash) - 1);
    }
};
PACK_END

static_assert(sizeof(ClientJoinRoomPayload) == 72, "ClientJoinRoomPayload must be 72 bytes");

/**
 * @brief CLIENT_LEAVE_ROOM payload (0x22)
 * Total size: 8 bytes
 */
PACK_START
struct PACKED ClientLeaveRoomPayload {
    uint32_t player_id;
    uint32_t room_id;

    ClientLeaveRoomPayload() : player_id(0), room_id(0) {}
};
PACK_END

static_assert(sizeof(ClientLeaveRoomPayload) == 8, "ClientLeaveRoomPayload must be 8 bytes");

/**
 * @brief CLIENT_START_GAME payload (0x24)
 * Total size: 8 bytes
 */
PACK_START
struct PACKED ClientStartGamePayload {
    uint32_t player_id;
    uint32_t room_id;

    ClientStartGamePayload() : player_id(0), room_id(0) {}
};
PACK_END

static_assert(sizeof(ClientStartGamePayload) == 8, "ClientStartGamePayload must be 8 bytes");

/**
 * @brief Room information entry for SERVER_ROOM_LIST
 * Size: 44 bytes
 */
PACK_START
struct PACKED RoomInfo {
    uint32_t room_id;
    char room_name[32];
    GameMode game_mode;
    Difficulty difficulty;
    uint8_t current_players;
    uint8_t max_players;
    uint16_t map_id;
    RoomStatus status;
    uint8_t is_private;  // 0 = public, 1 = private

    RoomInfo()
        : room_id(0)
        , game_mode(GameMode::SQUAD)
        , difficulty(Difficulty::NORMAL)
        , current_players(0)
        , max_players(4)
        , map_id(0)
        , status(RoomStatus::WAITING)
        , is_private(0) {
        std::memset(room_name, 0, sizeof(room_name));
    }

    void set_name(const std::string& name) {
        std::memset(room_name, 0, sizeof(room_name));
        std::strncpy(room_name, name.c_str(), sizeof(room_name) - 1);
    }
};
PACK_END

static_assert(sizeof(RoomInfo) == 44, "RoomInfo must be 44 bytes");

/**
 * @brief SERVER_ROOM_LIST payload header (0x91)
 * Base size: 2 bytes + (44 × room_count) bytes
 */
PACK_START
struct PACKED ServerRoomListPayload {
    uint16_t room_count;

    ServerRoomListPayload() : room_count(0) {}
};
PACK_END

static_assert(sizeof(ServerRoomListPayload) == 2, "ServerRoomListPayload base must be 2 bytes");

/**
 * @brief SERVER_ROOM_CREATED payload (0x90)
 * Total size: 36 bytes
 */
PACK_START
struct PACKED ServerRoomCreatedPayload {
    uint32_t room_id;
    char room_name[32];

    ServerRoomCreatedPayload() : room_id(0) {
        std::memset(room_name, 0, sizeof(room_name));
    }

    void set_room_name(const std::string& name) {
        std::memset(room_name, 0, sizeof(room_name));
        std::strncpy(room_name, name.c_str(), sizeof(room_name) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerRoomCreatedPayload) == 36, "ServerRoomCreatedPayload must be 36 bytes");

/**
 * @brief SERVER_ROOM_JOINED payload (0x92)
 * Total size: 4 bytes
 */
PACK_START
struct PACKED ServerRoomJoinedPayload {
    uint32_t room_id;

    ServerRoomJoinedPayload() : room_id(0) {}
};
PACK_END

static_assert(sizeof(ServerRoomJoinedPayload) == 4, "ServerRoomJoinedPayload must be 4 bytes");

/**
 * @brief SERVER_ROOM_LEFT payload (0x93)
 * Total size: 8 bytes
 */
PACK_START
struct PACKED ServerRoomLeftPayload {
    uint32_t room_id;
    uint32_t player_id;

    ServerRoomLeftPayload() : room_id(0), player_id(0) {}
};
PACK_END

static_assert(sizeof(ServerRoomLeftPayload) == 8, "ServerRoomLeftPayload must be 8 bytes");

/**
 * @brief SERVER_ROOM_ERROR payload (0x95)
 * Total size: 65 bytes
 */
PACK_START
struct PACKED ServerRoomErrorPayload {
    RoomError error_code;
    char error_message[64];

    ServerRoomErrorPayload() : error_code(RoomError::ROOM_NOT_FOUND) {
        std::memset(error_message, 0, sizeof(error_message));
    }

    void set_message(const std::string& message) {
        std::memset(error_message, 0, sizeof(error_message));
        std::strncpy(error_message, message.c_str(), sizeof(error_message) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerRoomErrorPayload) == 65, "ServerRoomErrorPayload must be 65 bytes");

/**
 * @brief CLIENT_SET_PLAYER_NAME payload (0x25)
 * Used to change player name while in lobby
 * Total size: 36 bytes
 */
PACK_START
struct PACKED ClientSetPlayerNamePayload {
    uint32_t player_id;
    char new_name[32];

    ClientSetPlayerNamePayload() : player_id(0) {
        std::memset(new_name, 0, sizeof(new_name));
    }

    void set_name(const std::string& name) {
        std::memset(new_name, 0, sizeof(new_name));
        std::strncpy(new_name, name.c_str(), sizeof(new_name) - 1);
    }
};
PACK_END

static_assert(sizeof(ClientSetPlayerNamePayload) == 36, "ClientSetPlayerNamePayload must be 36 bytes");

/**
 * @brief SERVER_PLAYER_NAME_UPDATED payload (0x96)
 * Broadcast to room members when a player changes their name
 * Total size: 40 bytes
 */
PACK_START
struct PACKED ServerPlayerNameUpdatedPayload {
    uint32_t player_id;
    char new_name[32];
    uint32_t room_id;

    ServerPlayerNameUpdatedPayload() : player_id(0), room_id(0) {
        std::memset(new_name, 0, sizeof(new_name));
    }

    void set_name(const std::string& name) {
        std::memset(new_name, 0, sizeof(new_name));
        std::strncpy(new_name, name.c_str(), sizeof(new_name) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerPlayerNameUpdatedPayload) == 40, "ServerPlayerNameUpdatedPayload must be 40 bytes");

/**
 * @brief CLIENT_SET_PLAYER_SKIN payload (0x26)
 * Used to change player skin while in lobby
 * Total size: 5 bytes
 */
PACK_START
struct PACKED ClientSetPlayerSkinPayload {
    uint32_t player_id;
    uint8_t skin_id;  // 0-14 (3 colors x 5 ship types)

    ClientSetPlayerSkinPayload() : player_id(0), skin_id(0) {}
};
PACK_END

static_assert(sizeof(ClientSetPlayerSkinPayload) == 5, "ClientSetPlayerSkinPayload must be 5 bytes");

/**
 * @brief SERVER_PLAYER_SKIN_UPDATED payload (0x97)
 * Broadcast to room members when a player changes their skin
 * Total size: 9 bytes
 */
PACK_START
struct PACKED ServerPlayerSkinUpdatedPayload {
    uint32_t player_id;
    uint8_t skin_id;
    uint32_t room_id;

    ServerPlayerSkinUpdatedPayload() : player_id(0), skin_id(0), room_id(0) {}
};
PACK_END

static_assert(sizeof(ServerPlayerSkinUpdatedPayload) == 9, "ServerPlayerSkinUpdatedPayload must be 9 bytes");

/**
 * @brief CLIENT_ADMIN_AUTH payload (0x30)
 * Client sends password hash to authenticate as admin
 * Total size: 96 bytes
 */
PACK_START
struct PACKED ClientAdminAuthPayload {
    uint32_t client_id;              // 4 bytes - Network byte order
    char password_hash[64];          // 64 bytes - SHA256 hex string
    char username[28];               // 28 bytes - Admin username (display)

    ClientAdminAuthPayload() : client_id(0) {
        std::memset(password_hash, 0, sizeof(password_hash));
        std::memset(username, 0, sizeof(username));
    }

    void set_password_hash(const std::string& hash) {
        std::memset(password_hash, 0, sizeof(password_hash));
        std::strncpy(password_hash, hash.c_str(), sizeof(password_hash) - 1);
    }

    void set_username(const std::string& name) {
        std::memset(username, 0, sizeof(username));
        std::strncpy(username, name.c_str(), sizeof(username) - 1);
    }
};
PACK_END

static_assert(sizeof(ClientAdminAuthPayload) == 96, "ClientAdminAuthPayload must be 96 bytes");

/**
 * @brief CLIENT_ADMIN_COMMAND payload (0x31)
 * Client sends admin command to execute
 * Total size: 140 bytes
 */
PACK_START
struct PACKED ClientAdminCommandPayload {
    uint32_t admin_id;               // 4 bytes - Admin player ID
    uint8_t command_length;          // 1 byte - Length of command string
    char command[135];               // 135 bytes - Command string

    ClientAdminCommandPayload() : admin_id(0), command_length(0) {
        std::memset(command, 0, sizeof(command));
    }

    void set_command(const std::string& cmd) {
        command_length = std::min(static_cast<uint8_t>(cmd.size()),
                                 static_cast<uint8_t>(sizeof(command) - 1));
        std::memset(command, 0, sizeof(command));
        std::strncpy(command, cmd.c_str(), command_length);
    }

    std::string get_command() const {
        return std::string(command,
                          std::min(static_cast<size_t>(command_length),
                                  sizeof(command)));
    }
};
PACK_END

static_assert(sizeof(ClientAdminCommandPayload) == 140, "ClientAdminCommandPayload must be 140 bytes");

/**
 * @brief SERVER_ADMIN_AUTH_RESULT payload (0xD0)
 * Server responds with authentication result
 * Total size: 69 bytes
 */
PACK_START
struct PACKED ServerAdminAuthResultPayload {
    uint8_t success;                 // 1 byte - 0=failed, 1=success
    uint32_t admin_level;            // 4 bytes - 0=none, 1=admin (future: roles)
    char failure_reason[64];         // 64 bytes - Error message if failed

    ServerAdminAuthResultPayload() : success(0), admin_level(0) {
        std::memset(failure_reason, 0, sizeof(failure_reason));
    }

    void set_failure_reason(const std::string& reason) {
        std::memset(failure_reason, 0, sizeof(failure_reason));
        std::strncpy(failure_reason, reason.c_str(), sizeof(failure_reason) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerAdminAuthResultPayload) == 69, "ServerAdminAuthResultPayload must be 69 bytes");

/**
 * @brief SERVER_ADMIN_COMMAND_RESULT payload (0xD1)
 * Server responds with command execution result
 * Total size: 257 bytes
 */
PACK_START
struct PACKED ServerAdminCommandResultPayload {
    uint8_t success;                 // 1 byte - 0=failed, 1=success
    char message[256];               // 256 bytes - Result message or error

    ServerAdminCommandResultPayload() : success(0) {
        std::memset(message, 0, sizeof(message));
    }

    void set_message(const std::string& msg) {
        std::memset(message, 0, sizeof(message));
        std::strncpy(message, msg.c_str(), sizeof(message) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerAdminCommandResultPayload) == 257, "ServerAdminCommandResultPayload must be 257 bytes");

/**
 * @brief SERVER_ADMIN_NOTIFICATION payload (0xD2)
 * Server sends admin notifications (player events, etc.)
 * Total size: 128 bytes
 */
PACK_START
struct PACKED ServerAdminNotificationPayload {
    uint8_t notification_type;       // 1 byte - Type of notification
    char message[127];               // 127 bytes - Notification text

    ServerAdminNotificationPayload() : notification_type(0) {
        std::memset(message, 0, sizeof(message));
    }

    void set_message(const std::string& msg) {
        std::memset(message, 0, sizeof(message));
        std::strncpy(message, msg.c_str(), sizeof(message) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerAdminNotificationPayload) == 128, "ServerAdminNotificationPayload must be 128 bytes");

/**
 * @brief SERVER_KICK_NOTIFICATION payload (0xD3)
 * Server sends kick notification before disconnecting player
 * Total size: 128 bytes
 */
PACK_START
struct PACKED ServerKickNotificationPayload {
    char reason[128];                // 128 bytes - Kick reason

    ServerKickNotificationPayload() {
        std::memset(reason, 0, sizeof(reason));
    }

    void set_reason(const std::string& msg) {
        std::memset(reason, 0, sizeof(reason));
        std::strncpy(reason, msg.c_str(), sizeof(reason) - 1);
    }
};
PACK_END

static_assert(sizeof(ServerKickNotificationPayload) == 128, "ServerKickNotificationPayload must be 128 bytes");

}
