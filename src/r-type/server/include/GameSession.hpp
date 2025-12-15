#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>
#include <functional>

#include "ecs/Registry.hpp"
#include "Entity.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"
#include "WaveManager.hpp"
#include "ServerConfig.hpp"

namespace rtype::server {

/**
 * @brief Represents a player in a game session
 */
struct GamePlayer {
    uint32_t player_id;
    Entity entity;
    std::string player_name;
    uint32_t score;
    uint8_t lives;
    bool is_alive;

    GamePlayer()
        : player_id(0)
        , entity(0)
        , player_name("")
        , score(0)
        , lives(config::PLAYER_LIVES)
        , is_alive(true) {}

    GamePlayer(uint32_t id, const std::string& name)
        : player_id(id)
        , entity(0)
        , player_name(name)
        , score(0)
        , lives(config::PLAYER_LIVES)
        , is_alive(true) {}
};

/**
 * @brief Manages a single game session with ECS integration
 */
class GameSession {
public:
    using StateSnapshotCallback = std::function<void(uint32_t session_id, const std::vector<uint8_t>& snapshot)>;
    using EntitySpawnCallback = std::function<void(uint32_t session_id, const std::vector<uint8_t>& spawn_data)>;
    using EntityDestroyCallback = std::function<void(uint32_t session_id, uint32_t entity_id)>;
    using GameOverCallback = std::function<void(uint32_t session_id, const std::vector<uint32_t>& player_ids)>;

    GameSession(uint32_t session_id, protocol::GameMode game_mode, protocol::Difficulty difficulty, uint16_t map_id);
    ~GameSession() = default;

    /**
     * @brief Add a player to the game session
     */
    void add_player(uint32_t player_id, const std::string& player_name);

    /**
     * @brief Remove a player from the game session
     */
    void remove_player(uint32_t player_id);

    /**
     * @brief Handle player input
     */
    void handle_input(uint32_t player_id, const protocol::ClientInputPayload& input);

    /**
     * @brief Update game state (called every server tick)
     */
    void update(float delta_time);

    /**
     * @brief Get session ID
     */
    uint32_t get_session_id() const { return session_id_; }

    /**
     * @brief Get all player IDs in this session
     */
    std::vector<uint32_t> get_player_ids() const;

    /**
     * @brief Check if session is still active
     */
    bool is_active() const { return is_active_; }

    /**
     * @brief Set callback for state snapshots
     */
    void set_state_snapshot_callback(StateSnapshotCallback callback) {
        state_snapshot_callback_ = callback;
    }

    /**
     * @brief Set callback for entity spawns
     */
    void set_entity_spawn_callback(EntitySpawnCallback callback) {
        entity_spawn_callback_ = callback;
    }

    /**
     * @brief Set callback for entity destroys
     */
    void set_entity_destroy_callback(EntityDestroyCallback callback) {
        entity_destroy_callback_ = callback;
    }

    /**
     * @brief Set callback for game over
     */
    void set_game_over_callback(GameOverCallback callback) {
        game_over_callback_ = callback;
    }

    /**
     * @brief Get the ECS registry for this session
     */
    Registry& get_registry() { return registry_; }

private:
    uint32_t session_id_;
    protocol::GameMode game_mode_;
    protocol::Difficulty difficulty_;
    uint16_t map_id_;
    bool is_active_;

    Registry registry_;
    std::unordered_map<uint32_t, GamePlayer> players_;
    WaveManager wave_manager_;

    uint32_t tick_count_;
    float accumulated_time_;
    float current_scroll_;
    std::chrono::steady_clock::time_point session_start_time_;

    StateSnapshotCallback state_snapshot_callback_;
    EntitySpawnCallback entity_spawn_callback_;
    EntityDestroyCallback entity_destroy_callback_;
    GameOverCallback game_over_callback_;

    float snapshot_timer_;

    void spawn_player_entity(GamePlayer& player);
    void spawn_enemy(const std::string& enemy_type, float x, float y);
    void send_state_snapshot();
    void check_game_over();
    void update_ecs_systems(float delta_time);
};

}
