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

// Level System includes
#include "LevelManager.hpp"
#include "components/LevelComponents.hpp"
#include "systems/LevelSystem.hpp"
#include "systems/BossSystem.hpp"
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
    ~GameSession() = default;

    /**
     * @brief Set the listener for game session events
     */
    void set_listener(IGameSessionListener* listener) { listener_ = listener; }

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
    float get_current_scroll() const { return current_scroll_; }  // NEW: For checkpoint system

    /**
     * @brief Resync a client with all existing entities
     */
    void resync_client(uint32_t player_id, uint32_t tcp_client_id);

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

    void spawn_player_entity(GamePlayer& player);
    void check_game_over();
    void check_offscreen_enemies();
    
    // Map-based wall spawning from tiles
    void load_map_segments(uint16_t map_id);
    void spawn_walls_in_view();

    uint32_t session_id_;
    protocol::GameMode game_mode_;
    protocol::Difficulty difficulty_;
    uint16_t map_id_;
    std::atomic<bool> is_active_;

    Registry registry_;
    std::unordered_map<uint32_t, GamePlayer> players_;
    std::unordered_map<uint32_t, Entity> player_entities_;
    WaveManager wave_manager_;
    LevelManager level_manager_;  // NEW: Level system manager

    uint32_t tick_count_;
    float current_scroll_;
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
    std::vector<rtype::SegmentData> map_segments_;
    size_t next_segment_to_spawn_ = 0;
    int tile_size_ = 16;
};

}
