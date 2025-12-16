/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ClientNetworkSystem - Client-side network synchronization system
*/

#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"
#include "NetworkClient.hpp"

#include <unordered_map>
#include <cstdint>
#include <memory>

namespace rtype::client {

/**
 * @brief Client-side network synchronization system
 *
 * Handles:
 * - Entity replication from server
 * - Position updates from snapshots
 * - Spawn/destroy events
 * - Input sending
 */
class ClientNetworkSystem : public ISystem {
public:
    /**
     * @brief Construct ClientNetworkSystem
     * @param network_client Reference to NetworkClient
     * @param local_player_id Local player's ID from server
     * @param input_plugin Reference to input plugin for reading keys
     */
    explicit ClientNetworkSystem(NetworkClient& network_client, uint32_t local_player_id,
                                engine::IInputPlugin& input_plugin);
    ~ClientNetworkSystem() override = default;

    // ISystem interface
    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    /**
     * @brief Handle snapshot from server
     * @param server_tick Server tick number
     * @param entities List of entity states
     */
    void handle_snapshot(uint32_t server_tick,
                         const std::vector<protocol::EntityState>& entities);

    /**
     * @brief Handle entity spawn from server
     * @param spawn Spawn data
     */
    void handle_entity_spawn(const protocol::ServerEntitySpawnPayload& spawn);

    /**
     * @brief Handle entity destroy from server
     * @param destroy Destroy data
     */
    void handle_entity_destroy(const protocol::ServerEntityDestroyPayload& destroy);

    /**
     * @brief Set the local player entity
     * @param entity Local player entity ID
     */
    void set_local_player_entity(Entity entity) { local_player_entity_ = entity; }

    /**
     * @brief Get the local player entity
     * @return Local player entity ID
     */
    Entity get_local_player_entity() const { return local_player_entity_; }

private:
    /**
     * @brief Send player input to server
     * @param registry ECS registry
     */
    void send_player_input(Registry& registry);

    /**
     * @brief Get or create entity from server ID
     * @param registry ECS registry
     * @param server_entity_id Server entity ID
     * @param type Entity type
     * @return Client entity ID
     */
    Entity get_or_create_entity(Registry& registry, uint32_t server_entity_id,
                                protocol::EntityType type);

    /**
     * @brief Destroy entity by server ID
     * @param registry ECS registry
     * @param server_entity_id Server entity ID
     */
    void destroy_entity_by_server_id(Registry& registry, uint32_t server_entity_id);

    /**
     * @brief Apply snapshot state to entity
     * @param registry ECS registry
     * @param entity Client entity ID
     * @param state Entity state from server
     */
    void apply_entity_state(Registry& registry, Entity entity,
                           const protocol::EntityState& state);

    /**
     * @brief Get sprite for entity type
     * @param type Entity type
     * @return Sprite component
     */
    Sprite get_sprite_for_entity_type(protocol::EntityType type);

    NetworkClient& network_client_;
    engine::IInputPlugin& input_plugin_;
    uint32_t local_player_id_;
    Entity local_player_entity_ = 0;

    // Server entity ID -> Client entity mapping
    std::unordered_map<uint32_t, Entity> server_to_client_entities_;

    // Client tick counter
    uint32_t client_tick_ = 0;

    // Input send rate limiting (send every 50ms)
    float input_send_timer_ = 0.0f;
    static constexpr float INPUT_SEND_INTERVAL = 0.05f; // 20 inputs/sec

    // Registry reference for callbacks
    Registry* registry_ = nullptr;
};

} // namespace rtype::client
