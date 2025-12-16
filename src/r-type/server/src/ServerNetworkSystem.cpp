/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ServerNetworkSystem implementation
*/

#include "ServerNetworkSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <cstring>

namespace rtype::server {

ServerNetworkSystem::ServerNetworkSystem(uint32_t session_id, float snapshot_interval)
    : session_id_(session_id)
    , snapshot_interval_(snapshot_interval)
    , snapshot_timer_(0.0f)
    , tick_count_(0)
{
    std::cout << "[ServerNetworkSystem " << session_id_ << "] Created (snapshot interval: "
              << snapshot_interval_ << "s)\n";
}

void ServerNetworkSystem::init(Registry& registry)
{
    std::cout << "[ServerNetworkSystem " << session_id_ << "] Initialized\n";
}

void ServerNetworkSystem::update(Registry& registry, float dt)
{
    // 1. Process pending inputs from clients
    process_pending_inputs(registry);

    // 2. Send snapshot if interval reached
    snapshot_timer_ += dt;
    if (snapshot_timer_ >= snapshot_interval_) {
        send_state_snapshot(registry);
        snapshot_timer_ = 0.0f;
        tick_count_++;
    }

    // 3. Broadcast pending spawn/destroy events
    broadcast_pending_spawns();
    broadcast_pending_destroys();
}

void ServerNetworkSystem::shutdown()
{
    std::cout << "[ServerNetworkSystem " << session_id_ << "] Shutdown\n";
}

void ServerNetworkSystem::queue_input(uint32_t player_id, const protocol::ClientInputPayload& input)
{
    pending_inputs_.push({player_id, input});
}

void ServerNetworkSystem::queue_entity_spawn(Entity entity, protocol::EntityType type,
                                              float x, float y, uint16_t health, uint8_t subtype)
{
    protocol::ServerEntitySpawnPayload spawn;
    spawn.entity_id = htonl(entity);
    spawn.entity_type = type;
    spawn.spawn_x = x;
    spawn.spawn_y = y;
    spawn.subtype = subtype;
    spawn.health = htons(health);
    pending_spawns_.push(spawn);
}

void ServerNetworkSystem::queue_entity_destroy(Entity entity)
{
    pending_destroys_.push(entity);
}

void ServerNetworkSystem::process_pending_inputs(Registry& registry)
{
    if (!player_entities_)
        return;

    auto& velocities = registry.get_components<Velocity>();

    while (!pending_inputs_.empty()) {
        auto [player_id, input] = pending_inputs_.front();
        pending_inputs_.pop();

        // Find the entity for this player
        auto it = player_entities_->find(player_id);
        if (it == player_entities_->end())
            continue;

        Entity player_entity = it->second;

        if (!velocities.has_entity(player_entity))
            continue;

        Velocity& vel = velocities[player_entity];

        // Reset velocity and apply based on input flags
        vel.x = 0.0f;
        vel.y = 0.0f;

        if (input.is_up_pressed())
            vel.y = -config::PLAYER_MOVEMENT_SPEED;
        if (input.is_down_pressed())
            vel.y = config::PLAYER_MOVEMENT_SPEED;
        if (input.is_left_pressed())
            vel.x = -config::PLAYER_MOVEMENT_SPEED;
        if (input.is_right_pressed())
            vel.x = config::PLAYER_MOVEMENT_SPEED;

        // TODO: Handle shooting
        if (input.is_shoot_pressed()) {
            // Spawn projectile entity
        }
    }
}

void ServerNetworkSystem::send_state_snapshot(Registry& registry)
{
    if (!snapshot_callback_)
        return;

    auto payload = serialize_snapshot(registry);
    snapshot_callback_(session_id_, payload);
}

std::vector<uint8_t> ServerNetworkSystem::serialize_snapshot(Registry& registry)
{
    protocol::ServerSnapshotPayload snapshot;
    snapshot.server_tick = htonl(tick_count_);

    std::vector<uint8_t> payload;
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&snapshot);
    payload.insert(payload.end(), header_bytes, header_bytes + sizeof(snapshot));

    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& healths = registry.get_components<Health>();

    uint16_t entity_count = 0;
    std::vector<protocol::EntityState> entity_states;

    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        Position& pos = positions.get_data_at(i);

        protocol::EntityState state;
        state.entity_id = htonl(entity);
        state.entity_type = protocol::EntityType::PLAYER; // TODO: Distinguish entity types
        state.position_x = pos.x;
        state.position_y = pos.y;

        if (velocities.has_entity(entity)) {
            Velocity& vel = velocities[entity];
            state.velocity_x = static_cast<int16_t>(vel.x * 10.0f);
            state.velocity_y = static_cast<int16_t>(vel.y * 10.0f);
        } else {
            state.velocity_x = 0;
            state.velocity_y = 0;
        }

        if (healths.has_entity(entity)) {
            Health& health = healths[entity];
            state.health = htons(static_cast<uint16_t>(health.current));
        } else {
            state.health = 0;
        }

        state.flags = 0;
        entity_states.push_back(state);
        entity_count++;
    }

    // Update entity count in header
    snapshot.entity_count = htons(entity_count);
    std::memcpy(payload.data(), &snapshot, sizeof(snapshot));

    // Append all entity states
    for (const auto& state : entity_states) {
        const uint8_t* state_bytes = reinterpret_cast<const uint8_t*>(&state);
        payload.insert(payload.end(), state_bytes, state_bytes + sizeof(state));
    }

    return payload;
}

void ServerNetworkSystem::broadcast_pending_spawns()
{
    if (!entity_spawn_callback_)
        return;

    while (!pending_spawns_.empty()) {
        const auto& spawn = pending_spawns_.front();
        entity_spawn_callback_(session_id_, serialize(spawn));
        pending_spawns_.pop();
    }
}

void ServerNetworkSystem::broadcast_pending_destroys()
{
    if (!entity_destroy_callback_)
        return;

    while (!pending_destroys_.empty()) {
        uint32_t entity_id = pending_destroys_.front();
        entity_destroy_callback_(session_id_, entity_id);
        pending_destroys_.pop();
    }
}

} // namespace rtype::server
