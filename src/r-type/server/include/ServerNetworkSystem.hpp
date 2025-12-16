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

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace rtype::server {

/**
 * @brief ECS System for server-side network synchronization
 *
 * Handles:
 * - Processing player inputs from network
 * - Sending state snapshots to clients
 * - Broadcasting entity spawn/destroy events
 */
class ServerNetworkSystem : public ISystem {
public:
    using SnapshotCallback = std::function<void(uint32_t session_id, const std::vector<uint8_t>&)>;
    using EntitySpawnCallback = std::function<void(uint32_t session_id, const std::vector<uint8_t>&)>;
    using EntityDestroyCallback = std::function<void(uint32_t session_id, uint32_t entity_id)>;
    using ProjectileSpawnCallback = std::function<void(uint32_t session_id, const std::vector<uint8_t>&)>;

    /**
     * @brief Construct a new ServerNetworkSystem
     *
     * @param session_id The game session ID this system belongs to
     * @param snapshot_interval Time between snapshots in seconds (default: 50ms)
     */
    explicit ServerNetworkSystem(uint32_t session_id, float snapshot_interval = config::SNAPSHOT_INTERVAL);

    ~ServerNetworkSystem() override = default;

    // ISystem interface
    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    /**
     * @brief Queue a player input for processing in next update
     *
     * @param player_id The player ID
     * @param input The input payload from client
     */
    void queue_input(uint32_t player_id, const protocol::ClientInputPayload& input);

    /**
     * @brief Queue an entity spawn for broadcasting
     *
     * @param entity The entity ID
     * @param type The entity type
     * @param x Spawn X position
     * @param y Spawn Y position
     * @param health Entity health
     * @param subtype Entity subtype (for enemies)
     */
    void queue_entity_spawn(Entity entity, protocol::EntityType type, float x, float y,
                            uint16_t health, uint8_t subtype = 0);

    /**
     * @brief Queue an entity destruction for broadcasting
     *
     * @param entity The entity ID to destroy
     */
    void queue_entity_destroy(Entity entity);

    /**
     * @brief Set the player entity mapping
     *
     * @param player_entities Pointer to map of player_id -> Entity
     */
    void set_player_entities(std::unordered_map<uint32_t, Entity>* player_entities) {
        player_entities_ = player_entities;
    }

    // Callback setters
    void set_snapshot_callback(SnapshotCallback callback) { snapshot_callback_ = std::move(callback); }
    void set_entity_spawn_callback(EntitySpawnCallback callback) { entity_spawn_callback_ = std::move(callback); }
    void set_entity_destroy_callback(EntityDestroyCallback callback) { entity_destroy_callback_ = std::move(callback); }
    void set_projectile_spawn_callback(ProjectileSpawnCallback callback) { projectile_spawn_callback_ = std::move(callback); }

    /**
     * @brief Get current tick count
     */
    uint32_t get_tick_count() const { return tick_count_; }

private:
    /**
     * @brief Process all pending inputs and update entity velocities
     */
    void process_pending_inputs(Registry& registry);

    /**
     * @brief Serialize and send state snapshot to clients
     */
    void send_state_snapshot(Registry& registry);

    /**
     * @brief Broadcast all pending entity spawns
     */
    void broadcast_pending_spawns();

    /**
     * @brief Broadcast all pending entity destroys
     */
    void broadcast_pending_destroys();

    /**
     * @brief Broadcast all pending projectile spawns
     */
    void broadcast_pending_projectiles();

    /**
     * @brief Spawn a projectile entity
     */
    void spawn_projectile(Registry& registry, Entity owner, float x, float y);

    /**
     * @brief Serialize the current world state to bytes
     */
    std::vector<uint8_t> serialize_snapshot(Registry& registry);

    uint32_t session_id_;
    float snapshot_interval_;
    float snapshot_timer_ = 0.0f;
    uint32_t tick_count_ = 0;

    // Queues for network events
    std::queue<std::pair<uint32_t, protocol::ClientInputPayload>> pending_inputs_;
    std::queue<protocol::ServerEntitySpawnPayload> pending_spawns_;
    std::queue<uint32_t> pending_destroys_;
    std::queue<protocol::ServerProjectileSpawnPayload> pending_projectiles_;

    // Cooldown tracking for shooting (player_id -> time since last shot)
    std::unordered_map<uint32_t, float> shoot_cooldowns_;
    static constexpr float SHOOT_COOLDOWN = 0.2f;  // 200ms between shots
    
    // Switch cooldown tracking
    std::unordered_map<uint32_t, float> switch_cooldowns_;
    static constexpr float SWITCH_COOLDOWN = 0.5f;

    // Event subscription
    core::EventBus::SubscriptionId shotFiredSubId_;

    // Callbacks to Server for actual network send
    SnapshotCallback snapshot_callback_;
    EntitySpawnCallback entity_spawn_callback_;
    EntityDestroyCallback entity_destroy_callback_;
    ProjectileSpawnCallback projectile_spawn_callback_;

    // Player ID -> Entity mapping (owned by GameSession)
    std::unordered_map<uint32_t, Entity>* player_entities_ = nullptr;
};

} // namespace rtype::server
