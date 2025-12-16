/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ClientNetworkSystem - Implementation
*/

#include "systems/ClientNetworkSystem.hpp"
#include "ecs/systems/InputSystem.hpp"
#include <iostream>

namespace rtype::client {

ClientNetworkSystem::ClientNetworkSystem(NetworkClient& network_client, uint32_t local_player_id,
    engine::IInputPlugin& input_plugin)
    : network_client_(network_client), input_plugin_(input_plugin), local_player_id_(local_player_id) {
}

void ClientNetworkSystem::init(Registry& registry) {
    std::cout << "[ClientNetworkSystem] Initializing (player_id=" << local_player_id_ << ")\n";

    // Register callbacks with NetworkClient
    network_client_.set_on_snapshot([this](uint32_t tick,
        const std::vector<protocol::EntityState>& entities) {
        handle_snapshot(tick, entities);
    });

    network_client_.set_on_entity_spawn([this](
        const protocol::ServerEntitySpawnPayload& spawn) {
        handle_entity_spawn(spawn);
    });

    network_client_.set_on_entity_destroy([this](
        const protocol::ServerEntityDestroyPayload& destroy) {
        handle_entity_destroy(destroy);
    });
}

void ClientNetworkSystem::update(Registry& registry, float dt) {
    registry_ = &registry;

    // Update network client (receive packets)
    network_client_.update();

    // Send player input
    input_send_timer_ += dt;
    if (input_send_timer_ >= INPUT_SEND_INTERVAL) {
        send_player_input(registry);
        input_send_timer_ = 0.0f;
        client_tick_++;
    }
}

void ClientNetworkSystem::shutdown() {
    std::cout << "[ClientNetworkSystem] Shutting down\n";
    server_to_client_entities_.clear();
}

void ClientNetworkSystem::handle_snapshot(uint32_t server_tick,
    const std::vector<protocol::EntityState>& entities) {

    if (!registry_) return;

    // Apply each entity state
    for (const auto& state : entities) {
        Entity client_entity = get_or_create_entity(
            *registry_, state.entity_id, state.entity_type);

        if (client_entity == 0) continue;

        // Don't overwrite local player position (client-side prediction)
        // TODO: Add server reconciliation if needed
        if (state.entity_id == local_player_id_) {
            // Skip position update for local player
            // (local input has priority for responsive feel)
            continue;
        }

        // Apply state to entity
        apply_entity_state(*registry_, client_entity, state);
    }
}

Entity ClientNetworkSystem::get_or_create_entity(Registry& registry,
    uint32_t server_entity_id, protocol::EntityType type) {

    // Check if entity already exists
    auto it = server_to_client_entities_.find(server_entity_id);
    if (it != server_to_client_entities_.end()) {
        return it->second;
    }

    // Create new entity
    Entity entity = registry.spawn_entity();
    server_to_client_entities_[server_entity_id] = entity;

    std::cout << "[ClientNetworkSystem] Creating entity (server_id=" << server_entity_id
              << ", client_id=" << entity << ", type=" << static_cast<int>(type) << ")\n";

    // Add components based on type
    registry.add_component<Position>(entity, Position{0.0f, 0.0f});
    registry.add_component<Velocity>(entity, Velocity{0.0f, 0.0f});
    registry.add_component<Health>(entity, Health{100, 100});

    // Add Sprite component for rendering
    Sprite sprite = get_sprite_for_entity_type(type);
    registry.add_component<Sprite>(entity, std::move(sprite));

    // Add type-specific components
    switch (type) {
        case protocol::EntityType::PLAYER:
            // Players don't need special tag on client
            break;

        case protocol::EntityType::ENEMY_BASIC:
        case protocol::EntityType::ENEMY_FAST:
        case protocol::EntityType::ENEMY_TANK:
        case protocol::EntityType::ENEMY_BOSS:
        case protocol::EntityType::ENEMY_ELITE:
            registry.add_component<Enemy>(entity, Enemy{});
            break;

        case protocol::EntityType::PROJECTILE_PLAYER:
        case protocol::EntityType::PROJECTILE_ENEMY:
            {
                Projectile proj;
                proj.faction = (type == protocol::EntityType::PROJECTILE_PLAYER)
                    ? ProjectileFaction::Player
                    : ProjectileFaction::Enemy;
                registry.add_component<Projectile>(entity, std::move(proj));
            }
            break;

        default:
            break;
    }

    return entity;
}

Sprite ClientNetworkSystem::get_sprite_for_entity_type(protocol::EntityType type) {
    // NOTE: This creates placeholder sprites with INVALID_HANDLE
    // The actual texture loading should be handled by a dedicated sprite loading system
    // that has access to the graphics plugin
    Sprite sprite;
    sprite.texture = engine::INVALID_HANDLE;
    sprite.tint = engine::Color::White;
    sprite.rotation = 0.0f;
    sprite.origin_x = 0.0f;
    sprite.origin_y = 0.0f;
    sprite.layer = 0;

    switch (type) {
        case protocol::EntityType::PLAYER:
            sprite.width = 33.0f * 2.0f;
            sprite.height = 17.0f * 2.0f;
            break;

        case protocol::EntityType::ENEMY_BASIC:
            sprite.width = 33.0f * 2.0f;
            sprite.height = 16.0f * 2.0f;
            break;

        case protocol::EntityType::ENEMY_FAST:
            sprite.width = 33.0f * 2.0f;
            sprite.height = 34.0f * 2.0f;
            break;

        case protocol::EntityType::ENEMY_TANK:
            sprite.width = 33.0f * 2.0f;
            sprite.height = 32.0f * 2.0f;
            break;

        case protocol::EntityType::ENEMY_BOSS:
            sprite.width = 128.0f * 1.5f;
            sprite.height = 128.0f * 1.5f;
            break;

        case protocol::EntityType::PROJECTILE_PLAYER:
            sprite.width = 16.0f * 1.5f;
            sprite.height = 13.0f * 1.5f;
            break;

        case protocol::EntityType::PROJECTILE_ENEMY:
            sprite.width = 8.0f * 1.5f;
            sprite.height = 8.0f * 1.5f;
            break;

        default:
            sprite.width = 32.0f;
            sprite.height = 32.0f;
            break;
    }

    return sprite;
}

void ClientNetworkSystem::apply_entity_state(Registry& registry, Entity entity,
    const protocol::EntityState& state) {

    // Update Position
    auto& positions = registry.get_components<Position>();
    if (positions.has_entity(entity)) {
        Position& pos = positions[entity];
        pos.x = state.position_x;
        pos.y = state.position_y;
    }

    // Update Velocity
    auto& velocities = registry.get_components<Velocity>();
    if (velocities.has_entity(entity)) {
        Velocity& vel = velocities[entity];
        vel.x = static_cast<float>(state.velocity_x);
        vel.y = static_cast<float>(state.velocity_y);
    }

    // Update Health
    auto& healths = registry.get_components<Health>();
    if (healths.has_entity(entity)) {
        Health& health = healths[entity];
        health.current = state.health;
    }

    // TODO: Apply flags (invulnerable, charging, damaged)
}

void ClientNetworkSystem::send_player_input(Registry& registry) {
    if (local_player_entity_ == 0) return;

    // Build input flags from plugin
    uint16_t input_flags = 0;
    if (input_plugin_.is_key_pressed(engine::Key::Up) || input_plugin_.is_key_pressed(engine::Key::W)) {
        input_flags |= protocol::INPUT_UP;
    }
    if (input_plugin_.is_key_pressed(engine::Key::Down) || input_plugin_.is_key_pressed(engine::Key::S)) {
        input_flags |= protocol::INPUT_DOWN;
    }
    if (input_plugin_.is_key_pressed(engine::Key::Left) || input_plugin_.is_key_pressed(engine::Key::A)) {
        input_flags |= protocol::INPUT_LEFT;
    }
    if (input_plugin_.is_key_pressed(engine::Key::Right) || input_plugin_.is_key_pressed(engine::Key::D)) {
        input_flags |= protocol::INPUT_RIGHT;
    }
    if (input_plugin_.is_key_pressed(engine::Key::Space)) {
        input_flags |= protocol::INPUT_SHOOT;
    }

    // Send to server (only if we have input or periodically to maintain connection)
    network_client_.send_input(input_flags, client_tick_);
}

void ClientNetworkSystem::handle_entity_spawn(
    const protocol::ServerEntitySpawnPayload& spawn) {

    if (!registry_) return;

    std::cout << "[ClientNetworkSystem] Entity spawn (server_id=" << spawn.entity_id
              << ", type=" << static_cast<int>(spawn.entity_type)
              << ", pos=[" << spawn.spawn_x << ", " << spawn.spawn_y << "])\n";

    // Create entity immediately
    Entity entity = get_or_create_entity(
        *registry_, spawn.entity_id, spawn.entity_type);

    if (entity == 0) return;

    // Set initial position
    auto& positions = registry_->get_components<Position>();
    if (positions.has_entity(entity)) {
        Position& pos = positions[entity];
        pos.x = spawn.spawn_x;
        pos.y = spawn.spawn_y;
    }

    // Set health
    auto& healths = registry_->get_components<Health>();
    if (healths.has_entity(entity)) {
        Health& health = healths[entity];
        health.current = spawn.health;
        health.max = spawn.health;
    }
}

void ClientNetworkSystem::handle_entity_destroy(
    const protocol::ServerEntityDestroyPayload& destroy) {

    if (!registry_) return;

    std::cout << "[ClientNetworkSystem] Entity destroy (server_id=" << destroy.entity_id << ")\n";

    destroy_entity_by_server_id(*registry_, destroy.entity_id);
}

void ClientNetworkSystem::destroy_entity_by_server_id(Registry& registry,
    uint32_t server_entity_id) {

    auto it = server_to_client_entities_.find(server_entity_id);
    if (it == server_to_client_entities_.end()) {
        return; // Entity doesn't exist on client
    }

    Entity entity = it->second;

    // Mark for destruction (DestroySystem will handle cleanup)
    registry.add_component<ToDestroy>(entity, ToDestroy{});

    // Remove from mapping
    server_to_client_entities_.erase(it);
}

} // namespace rtype::client
