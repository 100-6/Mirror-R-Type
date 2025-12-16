#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <functional>
#include "GameSession.hpp"
#include "protocol/PacketTypes.hpp"

namespace rtype::server {

/**
 * @brief Manages the lifecycle of game sessions
 *
 * This class handles:
 * - Creating and destroying game sessions
 * - Updating active sessions
 * - Managing session callbacks for network events
 */
class GameSessionManager {
public:
    // Callback types for session events
    using StateSnapshotCallback = std::function<void(uint32_t, const std::vector<uint8_t>&)>;
    using EntitySpawnCallback = std::function<void(uint32_t, const std::vector<uint8_t>&)>;
    using EntityDestroyCallback = std::function<void(uint32_t, uint32_t)>;
    using ProjectileSpawnCallback = std::function<void(uint32_t, const std::vector<uint8_t>&)>;
    using GameOverCallback = std::function<void(uint32_t, const std::vector<uint32_t>&)>;
    using WaveStartCallback = std::function<void(uint32_t, const std::vector<uint8_t>&)>;
    using WaveCompleteCallback = std::function<void(uint32_t, const std::vector<uint8_t>&)>;

    GameSessionManager();

    /**
     * @brief Create a new game session
     * @param session_id Unique session identifier
     * @param game_mode Game mode
     * @param difficulty Difficulty level
     * @param level_seed Random seed for level generation
     * @return Pointer to the created session
     */
    GameSession* create_session(uint32_t session_id, protocol::GameMode game_mode,
                               protocol::Difficulty difficulty, uint32_t level_seed);

    /**
     * @brief Get a game session by ID
     * @param session_id Session identifier
     * @return Pointer to the session, or nullptr if not found
     */
    GameSession* get_session(uint32_t session_id);

    /**
     * @brief Update all active game sessions
     * @param delta_time Time elapsed since last update (in seconds)
     */
    void update_all(float delta_time);

    /**
     * @brief Remove inactive sessions
     */
    void cleanup_inactive_sessions();

    /**
     * @brief Remove a specific session
     * @param session_id Session identifier
     */
    void remove_session(uint32_t session_id);

    /**
     * @brief Get all active session IDs
     */
    std::vector<uint32_t> get_active_session_ids() const;

    // Setters for session event callbacks
    void set_state_snapshot_callback(StateSnapshotCallback callback) {
        on_state_snapshot_ = callback;
    }

    void set_entity_spawn_callback(EntitySpawnCallback callback) {
        on_entity_spawn_ = callback;
    }

    void set_entity_destroy_callback(EntityDestroyCallback callback) {
        on_entity_destroy_ = callback;
    }

    void set_projectile_spawn_callback(ProjectileSpawnCallback callback) {
        on_projectile_spawn_ = callback;
    }

    void set_game_over_callback(GameOverCallback callback) {
        on_game_over_ = callback;
    }

    void set_wave_start_callback(WaveStartCallback callback) {
        on_wave_start_ = callback;
    }

    void set_wave_complete_callback(WaveCompleteCallback callback) {
        on_wave_complete_ = callback;
    }

private:
    std::unordered_map<uint32_t, std::unique_ptr<GameSession>> sessions_;

    // Session event callbacks
    StateSnapshotCallback on_state_snapshot_;
    EntitySpawnCallback on_entity_spawn_;
    EntityDestroyCallback on_entity_destroy_;
    ProjectileSpawnCallback on_projectile_spawn_;
    GameOverCallback on_game_over_;
    WaveStartCallback on_wave_start_;
    WaveCompleteCallback on_wave_complete_;

    /**
     * @brief Setup callbacks for a newly created session
     */
    void setup_session_callbacks(GameSession* session);
};

}
