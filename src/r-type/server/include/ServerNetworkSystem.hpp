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

    // === Configuration ===

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

    // === ISystem Interface ===

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    // === Input/Output Queuing ===

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

    // === Queries ===

    uint32_t get_tick_count() const { return tick_count_; }

private:
    void process_pending_inputs(Registry& registry);
    void send_state_snapshot(Registry& registry);
    void broadcast_pending_spawns();
    void broadcast_pending_destroys();
    void broadcast_pending_projectiles();
    void broadcast_pending_explosions();
    void broadcast_pending_scores();
    void spawn_projectile(Registry& registry, Entity owner, float x, float y);
    void spawn_enemy_projectile(Registry& registry, Entity owner, float x, float y);
    void update_enemy_shooting(Registry& registry, float dt);
    std::vector<uint8_t> serialize_snapshot(Registry& registry);

    // Configuration
    uint32_t session_id_;
    float snapshot_interval_;
    float snapshot_timer_ = 0.0f;
    uint32_t tick_count_ = 0;

    // Listener for network events
    INetworkSystemListener* listener_ = nullptr;

    // Queues for network events
    std::queue<std::pair<uint32_t, protocol::ClientInputPayload>> pending_inputs_;
    std::queue<protocol::ServerEntitySpawnPayload> pending_spawns_;
    std::queue<uint32_t> pending_destroys_;
    std::queue<protocol::ServerProjectileSpawnPayload> pending_projectiles_;
    std::queue<protocol::ServerExplosionPayload> pending_explosions_;
    std::queue<protocol::ServerScoreUpdatePayload> pending_scores_;

    // Cooldown tracking
    std::unordered_map<uint32_t, float> shoot_cooldowns_;
    std::unordered_map<uint32_t, float> switch_cooldowns_;
    std::unordered_map<Entity, float> enemy_shoot_cooldowns_;

    static constexpr float SHOOT_COOLDOWN = 0.2f;
    static constexpr float SWITCH_COOLDOWN = 0.5f;
    static constexpr float ENEMY_SHOOT_COOLDOWN = 1.5f;
    static constexpr float ENEMY_SHOOT_RANGE = 800.0f;

    // Event subscription ID
    core::EventBus::SubscriptionId shotFiredSubId_;
    core::EventBus::SubscriptionId enemyKilledSubId_;
    core::EventBus::SubscriptionId explosionSubId_;

    // Player ID -> Entity mapping (owned by GameSession)
    std::unordered_map<uint32_t, Entity>* player_entities_ = nullptr;
};

}
