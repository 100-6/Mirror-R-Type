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
        : player_id(0), entity(0), player_name(""), score(0)
        , lives(config::PLAYER_LIVES), is_alive(true) {}

    GamePlayer(uint32_t id, const std::string& name)
        : player_id(id), entity(0), player_name(name), score(0)
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

    // === Configuration ===

    /**
     * @brief Set the listener for game session events
     */
    void set_listener(IGameSessionListener* listener) { listener_ = listener; }

    // === Player Management ===

    void add_player(uint32_t player_id, const std::string& player_name);
    void remove_player(uint32_t player_id);
    void handle_input(uint32_t player_id, const protocol::ClientInputPayload& input);

    // === Update ===

    void update(float delta_time);

    // === Queries ===

    uint32_t get_session_id() const { return session_id_; }
    std::vector<uint32_t> get_player_ids() const;
    bool is_active() const { return is_active_; }
    Registry& get_registry() { return registry_; }

    /**
     * @brief Resync a client with all existing entities
     */
    void resync_client(uint32_t player_id, uint32_t tcp_client_id);

private:
    // IWaveListener implementation
    void on_wave_started(const Wave& wave) override;
    void on_wave_completed(const Wave& wave) override;
    void on_spawn_enemy(const std::string& enemy_type, float x, float y) override;
    void on_spawn_wall(float x, float y) override;
    void on_spawn_powerup(const std::string& bonus_type, float x, float y) override;

    // INetworkSystemListener implementation
    void on_snapshot_ready(uint32_t session_id, const std::vector<uint8_t>& snapshot) override;
    void on_entity_spawned(uint32_t session_id, const std::vector<uint8_t>& spawn_data) override;
    void on_entity_destroyed(uint32_t session_id, uint32_t entity_id) override;
    void on_projectile_spawned(uint32_t session_id, const std::vector<uint8_t>& projectile_data) override;
    void on_explosion_triggered(uint32_t session_id, const std::vector<uint8_t>& explosion_data) override;
    void on_score_updated(uint32_t session_id, const std::vector<uint8_t>& score_data) override;

    // Internal helpers
    void spawn_player_entity(GamePlayer& player);
    void check_game_over();
    void check_offscreen_enemies();

    // Session data
    uint32_t session_id_;
    protocol::GameMode game_mode_;
    protocol::Difficulty difficulty_;
    uint16_t map_id_;
    bool is_active_;

    // ECS
    Registry registry_;
    std::unordered_map<uint32_t, GamePlayer> players_;
    std::unordered_map<uint32_t, Entity> player_entities_;
    WaveManager wave_manager_;

    // Timing
    uint32_t tick_count_;
    float current_scroll_;
    std::chrono::steady_clock::time_point session_start_time_;

    // Network system reference
    ServerNetworkSystem* network_system_ = nullptr;

    // Listener for game events
    IGameSessionListener* listener_ = nullptr;

    // Wave state for resync
    protocol::ServerWaveStartPayload last_wave_start_payload_;
    protocol::ServerWaveCompletePayload last_wave_complete_payload_;
    bool has_wave_started_ = false;
    bool has_wave_complete_ = false;
};

}
