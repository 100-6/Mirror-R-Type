/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameSession - Manages a single game session with ECS integration
*/

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>
#include <string>
#include <atomic>

#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/systems/MovementSystem.hpp"
#include "ecs/systems/PhysiqueSystem.hpp"
#include "ecs/systems/DestroySystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/HealthSystem.hpp"
#include "Entity.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"
#include "WaveManager.hpp"
#include "ServerConfig.hpp"
#include "ServerNetworkSystem.hpp"
#include "interfaces/IGameSessionListener.hpp"
#include "interfaces/IWaveListener.hpp"
#include "interfaces/INetworkSystemListener.hpp"
#include "systems/MapConfigLoader.hpp"
#include "components/MapTypes.hpp"

// Forward declaration for procedural generation
namespace rtype {
    class ProceduralMapGenerator;
}

// Level System includes
#include "LevelManager.hpp"
#include "components/LevelComponents.hpp"
#include "systems/LevelSystem.hpp"
#include "systems/CheckpointSystem.hpp"

namespace rtype::server {

/**
 * @brief Represents a player in a game session
 */
struct GamePlayer {
    uint32_t player_id;
    Entity entity;
    std::string player_name;
    uint8_t skin_id;    // Player skin (0-14: 3 colors x 5 ship types)
    uint32_t score;
    uint8_t lives;
    bool is_alive;

    GamePlayer()
        : player_id(0), entity(0), player_name(""), skin_id(0), score(0)
        , lives(config::PLAYER_LIVES), is_alive(true) {}

    GamePlayer(uint32_t id, const std::string& name, uint8_t skin = 0)
        : player_id(id), entity(0), player_name(name), skin_id(skin), score(0)
        , lives(config::PLAYER_LIVES), is_alive(true) {}
};

/**
 * @brief Manages a single game session with ECS integration
 *
 * Simple class that:
 * - Manages ECS registry and systems
 * - Handles player lifecycle
 * - Processes waves and spawns
 * - Notifies listener of game events
 */
class GameSession : public IWaveListener, public INetworkSystemListener {
public:
    GameSession(uint32_t session_id, protocol::GameMode game_mode,
                protocol::Difficulty difficulty, uint16_t map_id);
    ~GameSession();  // Must be defined in .cpp where ProceduralMapGenerator is complete

    /**
     * @brief Set the listener for game session events
     */
    void set_listener(IGameSessionListener* listener) { listener_ = listener; }
    
    /**
     * @brief Respawn a player at specific coordinates
     * @param player_id ID of player to respawn
     * @param x World X coordinate
     * @param y World Y coordinate
     * @param invuln_duration Invulnerability duration in seconds
     * @param lives Remaining lives to sync
     * @return New player entity
     */
    Entity respawn_player_at(uint32_t player_id, float x, float y, float invuln_duration, uint8_t lives);

    void add_player(uint32_t player_id, const std::string& player_name, uint8_t skin_id = 0);
    void remove_player(uint32_t player_id);
    void handle_input(uint32_t player_id, const protocol::ClientInputPayload& input);

    void update(float delta_time);

    uint32_t get_session_id() const { return session_id_; }
    std::vector<uint32_t> get_player_ids() const;
    bool is_active() const { return is_active_.load(std::memory_order_acquire); }
    bool is_active_threadsafe() const { return is_active_.load(std::memory_order_acquire); }
    Registry& get_registry() { return registry_; }
    ServerNetworkSystem* get_network_system() { return network_system_; }
    float get_scroll_speed() const { return scroll_speed_; }
    double get_current_scroll() const { return current_scroll_; }  // NEW: For checkpoint system
    uint32_t get_map_seed() const { return map_seed_; }

    /**
     * @brief Resync a client with all existing entities
     */
    void resync_client(uint32_t player_id, uint32_t tcp_client_id);

    // Admin commands
    void pause();
    void resume();
    void clear_enemies();

    /**
     * @brief Send the leaderboard to all players (called before game over)
     */
    void send_leaderboard();

    /**
     * @brief Get player scores for global leaderboard
     * @return Vector of pairs (player_name, score)
     */
    std::vector<std::pair<std::string, uint32_t>> get_player_scores() const;

private:
    void on_wave_started(const Wave& wave) override;
    void on_wave_completed(const Wave& wave) override;
    void on_spawn_enemy(const std::string& enemy_type, float x, float y, const BonusDropConfig& bonus_drop) override;
    void on_spawn_wall(float x, float y) override;
    void on_spawn_powerup(const std::string& bonus_type, float x, float y) override;

    void on_snapshot_ready(uint32_t session_id, const std::vector<uint8_t>& snapshot) override;
    void on_entity_spawned(uint32_t session_id, const std::vector<uint8_t>& spawn_data) override;
    void on_entity_destroyed(uint32_t session_id, uint32_t entity_id) override;
    void on_projectile_spawned(uint32_t session_id, const std::vector<uint8_t>& projectile_data) override;
    void on_explosion_triggered(uint32_t session_id, const std::vector<uint8_t>& explosion_data) override;
    void on_score_updated(uint32_t session_id, const std::vector<uint8_t>& score_data) override;
    void on_powerup_collected(uint32_t session_id, const std::vector<uint8_t>& powerup_data) override;
    void on_player_respawn(uint32_t session_id, const std::vector<uint8_t>& respawn_data) override;
    void on_player_level_up(uint32_t session_id, const std::vector<uint8_t>& level_up_data) override;
    void on_level_transition(uint32_t session_id, const std::vector<uint8_t>& transition_data) override;
    void on_level_ready(uint32_t session_id, const std::vector<uint8_t>& level_ready_data) override;

    // === Entity Spawning Helpers ===
    /**
     * @brief Create player entity with all components
     */
    void spawn_player_entity(GamePlayer& player);
    void check_game_over();
    void check_offscreen_enemies();

    // Wave initialization
    void initialize_wave_state();

    // Map-based wall spawning from tiles
    void load_map_segments(uint16_t map_id);
    void spawn_walls_in_view();
    void despawn_walls_behind_camera();

    // Note: Wall collision is handled by CollisionSystem with setScroll()
    // See CollisionSystem::update() which uses m_currentScroll

    uint32_t session_id_;
    protocol::GameMode game_mode_;
    protocol::Difficulty difficulty_;
    uint16_t map_id_;
    std::atomic<bool> is_active_;
    bool is_paused_ = false;

    Registry registry_;
    std::unordered_map<uint32_t, GamePlayer> players_;
    std::unordered_map<uint32_t, Entity> player_entities_;
    WaveManager wave_manager_;
    LevelManager level_manager_;  // NEW: Level system manager

    uint32_t tick_count_;
    double current_scroll_;  // Use double for precision over long play sessions
    float scroll_speed_;
    std::chrono::steady_clock::time_point session_start_time_;

    ServerNetworkSystem* network_system_ = nullptr;

    IGameSessionListener* listener_ = nullptr;

    protocol::ServerWaveStartPayload last_wave_start_payload_;
    protocol::ServerWaveCompletePayload last_wave_complete_payload_;
    bool has_wave_started_ = false;
    bool has_wave_complete_ = false;

    // Map segment data for tile-based walls
    rtype::MapConfig map_config_;
    std::vector<rtype::SegmentData> map_segments_;  // For static maps
    std::unordered_map<int, rtype::SegmentData> generated_segments_;  // For procedural maps
    size_t next_segment_to_spawn_ = 0;
    int tile_size_ = 16;

    // Track level data loading
    uint8_t loaded_level_id_ = 0;
    game::LevelState last_level_state_ = game::LevelState::LEVEL_START;

    // Level transition delay (wait 1 second after boss death before fade)
    bool level_transition_pending_ = false;
    float level_transition_delay_timer_ = 0.0f;
    uint8_t pending_next_level_id_ = 0;
    static constexpr float LEVEL_TRANSITION_DELAY = 1.0f;  // 1 second delay

    // Procedural generation
    bool procedural_enabled_ = false;
    std::unique_ptr<class rtype::ProceduralMapGenerator> generator_;
    rtype::ProceduralConfig procedural_config_;
    uint32_t map_seed_ = 0;

    // Helper for procedural generation
    rtype::SegmentData* get_or_generate_segment(int segment_id);

    std::unordered_map<std::string, std::string> enemy_scripts_; // Map enemy type to script path
};

}
