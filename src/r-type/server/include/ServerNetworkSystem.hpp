/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ServerNetworkSystem - ECS System for server-side network synchronization
*/

#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"
#include "ServerConfig.hpp"
#include "core/event/EventBus.hpp"
#include "interfaces/INetworkSystemListener.hpp"

#include <queue>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <mutex>

namespace rtype::server {

/**
 * @brief ECS System for server-side network synchronization
 *
 * Simple system that:
 * - Processes player inputs from network
 * - Sends state snapshots to clients (20 Hz)
 * - Broadcasts entity spawn/destroy events
 * - Handles enemy shooting logic
 */
class ServerNetworkSystem : public ISystem {
public:
    /**
     * @brief Construct a new ServerNetworkSystem
     * @param session_id The game session ID
     * @param snapshot_interval Time between snapshots (default: 50ms)
     */
    explicit ServerNetworkSystem(uint32_t session_id, float snapshot_interval = config::SNAPSHOT_INTERVAL);
    ~ServerNetworkSystem() override = default;

    /**
     * @brief Set the listener for network events
     */
    void set_listener(INetworkSystemListener* listener) { listener_ = listener; }

    /**
     * @brief Set the player entity mapping (owned by GameSession)
     */
    void set_player_entities(std::unordered_map<uint32_t, Entity>* player_entities) {
        player_entities_ = player_entities;
    }

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    /**
     * @brief Queue a player input for processing
     */
    void queue_input(uint32_t player_id, const protocol::ClientInputPayload& input);

    /**
     * @brief Queue an entity spawn for broadcasting
     */
    void queue_entity_spawn(Entity entity, protocol::EntityType type, float x, float y,
                            uint16_t health, uint8_t subtype = 0);

    /**
     * @brief Queue an entity destruction for broadcasting
     */
    void queue_entity_destroy(Entity entity);

    /**
     * @brief Queue a powerup collected event for broadcasting
     */
    void queue_powerup_collected(uint32_t player_id, protocol::PowerupType type);

    uint32_t get_tick_count() const { return tick_count_; }

    /**
     * @brief Drain all pending entity spawns atomically
     * @return Queue of pending spawns (queue in this object is cleared)
     */
    std::queue<protocol::ServerEntitySpawnPayload> drain_pending_spawns();

    /**
     * @brief Drain all pending entity destroys atomically
     * @return Queue of pending destroys (queue in this object is cleared)
     */
    std::queue<uint32_t> drain_pending_destroys();

    /**
     * @brief Drain all pending projectile spawns atomically
     * @return Queue of pending projectiles (queue in this object is cleared)
     */
    std::queue<protocol::ServerProjectileSpawnPayload> drain_pending_projectiles();

    /**
     * @brief Drain all pending explosions atomically
     * @return Queue of pending explosions (queue in this object is cleared)
     */
    std::queue<protocol::ServerExplosionPayload> drain_pending_explosions();

    /**
     * @brief Drain all pending score updates atomically
     * @return Queue of pending scores (queue in this object is cleared)
     */
    std::queue<protocol::ServerScoreUpdatePayload> drain_pending_scores();

private:
    void process_pending_inputs(Registry& registry);
    void send_state_snapshot(Registry& registry);
    void broadcast_pending_spawns();
    void broadcast_pending_destroys();
    void broadcast_pending_projectiles();
    void broadcast_pending_explosions();
    void broadcast_pending_scores();
    void broadcast_pending_powerups();
    void spawn_projectile(Registry& registry, Entity owner, float x, float y);
    void spawn_enemy_projectile(Registry& registry, Entity owner, float x, float y);
    void update_enemy_shooting(Registry& registry, float dt);
    std::vector<uint8_t> serialize_snapshot(Registry& registry);

    uint32_t session_id_;
    float snapshot_interval_;
    float snapshot_timer_ = 0.0f;
    uint32_t tick_count_ = 0;

    INetworkSystemListener* listener_ = nullptr;

    std::queue<std::pair<uint32_t, protocol::ClientInputPayload>> pending_inputs_;
    std::queue<protocol::ServerEntitySpawnPayload> pending_spawns_;
    std::queue<uint32_t> pending_destroys_;
    std::queue<protocol::ServerProjectileSpawnPayload> pending_projectiles_;
    std::queue<protocol::ServerExplosionPayload> pending_explosions_;
    std::queue<protocol::ServerScoreUpdatePayload> pending_scores_;
    std::queue<protocol::ServerPowerupCollectedPayload> pending_powerups_;

    std::mutex spawns_mutex_;
    std::mutex destroys_mutex_;
    std::mutex projectiles_mutex_;
    std::mutex explosions_mutex_;
    std::mutex scores_mutex_;
    std::mutex powerups_mutex_;

    std::unordered_map<uint32_t, float> shoot_cooldowns_;
    std::unordered_map<uint32_t, float> switch_cooldowns_;
    std::unordered_map<Entity, float> enemy_shoot_cooldowns_;

    // Lag compensation: track last processed input sequence per player
    std::unordered_map<uint32_t, uint32_t> last_processed_input_seq_;

    static constexpr float SHOOT_COOLDOWN = 0.2f;
    static constexpr float SWITCH_COOLDOWN = 0.5f;
    static constexpr float ENEMY_SHOOT_COOLDOWN = 1.5f;
    static constexpr float ENEMY_SHOOT_RANGE = 800.0f;

    core::EventBus::SubscriptionId shotFiredSubId_;
    core::EventBus::SubscriptionId enemyKilledSubId_;
    core::EventBus::SubscriptionId explosionSubId_;
    core::EventBus::SubscriptionId bonusCollectedSubId_;

    std::unordered_map<uint32_t, Entity>* player_entities_ = nullptr;
};

}
