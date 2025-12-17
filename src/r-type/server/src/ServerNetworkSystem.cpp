/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ServerNetworkSystem implementation
*/

#include "ServerNetworkSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include "ecs/events/GameEvents.hpp"
#include "ecs/events/InputEvents.hpp"
#include "components/CombatHelpers.hpp"
#include "systems/ShootingSystem.hpp"

#include <iostream>
#include <cstring>
#include <cmath>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

namespace rtype::server {

ServerNetworkSystem::ServerNetworkSystem(uint32_t session_id, float snapshot_interval)
    : session_id_(session_id)
    , snapshot_interval_(snapshot_interval)
{
    std::cout << "[ServerNetworkSystem " << session_id_ << "] Created (snapshot interval: "
              << snapshot_interval_ << "s)\n";
}

void ServerNetworkSystem::init(Registry& registry)
{
    std::cout << "[ServerNetworkSystem " << session_id_ << "] Initialized\n";

    // Subscribe to ShotFiredEvent from ShootingSystem
    shotFiredSubId_ = registry.get_event_bus().subscribe<ecs::ShotFiredEvent>(
        [this, &registry](const ecs::ShotFiredEvent& event) {
            auto& velocities = registry.get_components<Velocity>();
            auto& positions = registry.get_components<Position>();
            auto& projectiles = registry.get_components<Projectile>();

            if (!velocities.has_entity(event.projectile) ||
                !positions.has_entity(event.projectile) ||
                !projectiles.has_entity(event.projectile))
                return;

            const auto& vel = velocities[event.projectile];
            const auto& pos = positions[event.projectile];

            protocol::ServerProjectileSpawnPayload spawn;
            spawn.projectile_id = htonl(event.projectile);
            spawn.owner_id = htonl(event.shooter);
            spawn.projectile_type = protocol::ProjectileType::BULLET;
            spawn.spawn_x = pos.x;
            spawn.spawn_y = pos.y;
            spawn.velocity_x = static_cast<int16_t>(vel.x);
            spawn.velocity_y = static_cast<int16_t>(vel.y);

            pending_projectiles_.push(spawn);
            std::cout << "[SHOOT] Queued projectile " << event.projectile
                      << " from shooter " << event.shooter << "\n";
        });
}

void ServerNetworkSystem::update(Registry& registry, float dt)
{
    // Update cooldowns
    for (auto& [player_id, cooldown] : shoot_cooldowns_)
        cooldown += dt;
    for (auto& [player_id, cooldown] : switch_cooldowns_)
        cooldown += dt;
    for (auto& [enemy_id, cooldown] : enemy_shoot_cooldowns_)
        cooldown += dt;

    // 1. Process pending inputs
    process_pending_inputs(registry);

    // 2. Update enemy shooting
    update_enemy_shooting(registry, dt);

    // 3. Update projectile lifetimes
    auto& projectiles = registry.get_components<Projectile>();
    for (size_t i = 0; i < projectiles.size(); ++i) {
        Entity entity = projectiles.get_entity_at(i);
        Projectile& proj = projectiles.get_data_at(i);
        proj.time_alive += dt;
        if (proj.time_alive >= proj.lifetime)
            registry.add_component(entity, ToDestroy{});
    }

    // 4. Queue destroy notifications for entities marked ToDestroy
    auto& to_destroy = registry.get_components<ToDestroy>();
    for (size_t i = 0; i < to_destroy.size(); ++i) {
        Entity entity = to_destroy.get_entity_at(i);
        queue_entity_destroy(entity);
    }

    // 5. Send snapshot if interval reached
    snapshot_timer_ += dt;
    if (snapshot_timer_ >= snapshot_interval_) {
        send_state_snapshot(registry);
        snapshot_timer_ = 0.0f;
        tick_count_++;
    }

    // 6. Broadcast pending events
    broadcast_pending_spawns();
    broadcast_pending_destroys();
    broadcast_pending_projectiles();
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

        auto it = player_entities_->find(player_id);
        if (it == player_entities_->end())
            continue;

        Entity player_entity = it->second;
        if (!velocities.has_entity(player_entity))
            continue;

        Velocity& vel = velocities[player_entity];

        // Apply input to velocity
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

        // Handle shooting via events
        auto& weapons = registry.get_components<Weapon>();
        if (weapons.has_entity(player_entity)) {
            bool was_held = weapons[player_entity].trigger_held;
            bool is_pressed = input.is_shoot_pressed();

            if (is_pressed && !was_held) {
                std::cout << "[SHOOT] PlayerStartFireEvent for player " << player_id << "\n";
                registry.get_event_bus().publish(ecs::PlayerStartFireEvent{player_entity});
            } else if (!is_pressed && was_held) {
                std::cout << "[SHOOT] PlayerStopFireEvent for player " << player_id << "\n";
                registry.get_event_bus().publish(ecs::PlayerStopFireEvent{player_entity});
            }
        }

        // Handle weapon switch
        if (input.is_switch_weapon_pressed()) {
            if (switch_cooldowns_.find(player_id) == switch_cooldowns_.end())
                switch_cooldowns_[player_id] = SWITCH_COOLDOWN;

            if (switch_cooldowns_[player_id] >= SWITCH_COOLDOWN) {
                if (weapons.has_entity(player_entity)) {
                    Weapon& w = weapons[player_entity];
                    int nextType = (static_cast<int>(w.type) + 1) % 5;
                    w = create_weapon(static_cast<WeaponType>(nextType), engine::INVALID_HANDLE);
                    switch_cooldowns_[player_id] = 0.0f;
                }
            }
        }
    }
}

void ServerNetworkSystem::send_state_snapshot(Registry& registry)
{
    if (!listener_)
        return;

    auto payload = serialize_snapshot(registry);
    listener_->on_snapshot_ready(session_id_, payload);
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
    auto& to_destroy = registry.get_components<ToDestroy>();

    uint16_t entity_count = 0;
    std::vector<protocol::EntityState> entity_states;

    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);

        // Skip entities marked for destruction
        if (to_destroy.has_entity(entity))
            continue;

        Position& pos = positions.get_data_at(i);

        protocol::EntityState state;
        state.entity_id = htonl(entity);
        state.entity_type = protocol::EntityType::PLAYER;
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

    snapshot.entity_count = htons(entity_count);
    std::memcpy(payload.data(), &snapshot, sizeof(snapshot));

    for (const auto& state : entity_states) {
        const uint8_t* state_bytes = reinterpret_cast<const uint8_t*>(&state);
        payload.insert(payload.end(), state_bytes, state_bytes + sizeof(state));
    }

    return payload;
}

void ServerNetworkSystem::broadcast_pending_spawns()
{
    if (!listener_)
        return;

    while (!pending_spawns_.empty()) {
        const auto& spawn = pending_spawns_.front();
        listener_->on_entity_spawned(session_id_, serialize(spawn));
        pending_spawns_.pop();
    }
}

void ServerNetworkSystem::broadcast_pending_destroys()
{
    if (!listener_)
        return;

    while (!pending_destroys_.empty()) {
        uint32_t entity_id = pending_destroys_.front();
        listener_->on_entity_destroyed(session_id_, entity_id);
        pending_destroys_.pop();
    }
}

void ServerNetworkSystem::broadcast_pending_projectiles()
{
    if (!listener_)
        return;

    while (!pending_projectiles_.empty()) {
        const auto& proj = pending_projectiles_.front();
        listener_->on_projectile_spawned(session_id_, serialize(proj));
        pending_projectiles_.pop();
        std::cout << "[SHOOT] Sent projectile " << ntohl(proj.projectile_id) << " to clients\n";
    }
}

void ServerNetworkSystem::spawn_projectile(Registry& registry, Entity owner, float x, float y)
{
    Entity projectile = registry.spawn_entity();

    registry.add_component(projectile, Position{x, y});
    registry.add_component(projectile, Velocity{config::PROJECTILE_SPEED, 0.0f});
    registry.add_component(projectile, Collider{config::PROJECTILE_WIDTH, config::PROJECTILE_HEIGHT});
    registry.add_component(projectile, Damage{config::PROJECTILE_DAMAGE});
    registry.add_component(projectile, Projectile{0.0f, config::PROJECTILE_LIFETIME, 0.0f, ProjectileFaction::Player});
    registry.add_component(projectile, NoFriction{});

    protocol::ServerProjectileSpawnPayload spawn;
    spawn.projectile_id = htonl(projectile);
    spawn.owner_id = htonl(owner);
    spawn.projectile_type = protocol::ProjectileType::BULLET;
    spawn.spawn_x = x;
    spawn.spawn_y = y;
    spawn.velocity_x = static_cast<int16_t>(config::PROJECTILE_SPEED);
    spawn.velocity_y = 0;
    pending_projectiles_.push(spawn);

    std::cout << "[ServerNetworkSystem " << session_id_ << "] Spawned projectile " << projectile
              << " from entity " << owner << " at (" << x << ", " << y << ")\n";
}

void ServerNetworkSystem::spawn_enemy_projectile(Registry& registry, Entity owner, float x, float y)
{
    Entity projectile = registry.spawn_entity();

    registry.add_component(projectile, Position{x, y});
    registry.add_component(projectile, Velocity{-config::PROJECTILE_SPEED, 0.0f});
    registry.add_component(projectile, Collider{config::PROJECTILE_WIDTH, config::PROJECTILE_HEIGHT});
    registry.add_component(projectile, Damage{config::ENEMY_PROJECTILE_DAMAGE});
    registry.add_component(projectile, Projectile{0.0f, config::PROJECTILE_LIFETIME, 0.0f, ProjectileFaction::Enemy});
    registry.add_component(projectile, NoFriction{});

    protocol::ServerProjectileSpawnPayload spawn;
    spawn.projectile_id = htonl(projectile);
    spawn.owner_id = htonl(owner);
    spawn.projectile_type = protocol::ProjectileType::BULLET;
    spawn.spawn_x = x;
    spawn.spawn_y = y;
    spawn.velocity_x = static_cast<int16_t>(-config::PROJECTILE_SPEED);
    spawn.velocity_y = 0;
    pending_projectiles_.push(spawn);

    std::cout << "[ServerNetworkSystem " << session_id_ << "] Enemy " << owner
              << " spawned projectile " << projectile << " at (" << x << ", " << y << ")\n";
}

void ServerNetworkSystem::update_enemy_shooting(Registry& registry, float dt)
{
    if (!player_entities_)
        return;

    auto& enemies = registry.get_components<Enemy>();
    auto& enemy_positions = registry.get_components<Position>();
    auto& enemy_colliders = registry.get_components<Collider>();
    auto& player_positions = registry.get_components<Position>();

    for (size_t i = 0; i < enemies.size(); ++i) {
        Entity enemy = enemies.get_entity_at(i);

        if (!enemy_positions.has_entity(enemy) || !enemy_colliders.has_entity(enemy))
            continue;

        Position& enemy_pos = enemy_positions[enemy];
        Collider& enemy_col = enemy_colliders[enemy];

        if (enemy_shoot_cooldowns_.find(enemy) == enemy_shoot_cooldowns_.end())
            enemy_shoot_cooldowns_[enemy] = ENEMY_SHOOT_COOLDOWN;

        if (enemy_shoot_cooldowns_[enemy] < ENEMY_SHOOT_COOLDOWN)
            continue;

        bool player_in_range = false;
        for (const auto& [player_id, player_entity] : *player_entities_) {
            if (!player_positions.has_entity(player_entity))
                continue;

            Position& player_pos = player_positions[player_entity];

            float dx = player_pos.x - enemy_pos.x;
            float dy = player_pos.y - enemy_pos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance <= ENEMY_SHOOT_RANGE && dx < 0) {
                player_in_range = true;
                break;
            }
        }

        if (player_in_range) {
            float spawn_x = enemy_pos.x;
            float spawn_y = enemy_pos.y + enemy_col.height / 2.0f - config::PROJECTILE_HEIGHT / 2.0f;
            spawn_enemy_projectile(registry, enemy, spawn_x, spawn_y);
            enemy_shoot_cooldowns_[enemy] = 0.0f;
        }
    }
}

}
