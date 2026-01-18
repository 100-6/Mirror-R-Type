/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ServerNetworkSystem implementation
*/

#include "ServerNetworkSystem.hpp"
#include "NetworkUtils.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include "ecs/events/GameEvents.hpp"
#include "ecs/events/InputEvents.hpp"
#include "components/CombatHelpers.hpp"
#include "systems/ShootingSystem.hpp"
#include "GameConfig.hpp"

#include <iostream>
#include <cmath>


namespace rtype::server {
using netutils::ByteOrder;
using netutils::Memory;

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
            spawn.projectile_id = ByteOrder::host_to_net32(event.projectile);
            spawn.owner_id = ByteOrder::host_to_net32(event.shooter);
            spawn.projectile_type = protocol::ProjectileType::BULLET;
            spawn.spawn_x = pos.x;
            spawn.spawn_y = pos.y;
            spawn.velocity_x = static_cast<int16_t>(vel.x);
            spawn.velocity_y = static_cast<int16_t>(vel.y);
            {
                std::lock_guard lock(projectiles_mutex_);
                pending_projectiles_.push(spawn);
            }
        });
    enemyKilledSubId_ = registry.get_event_bus().subscribe<ecs::EnemyKilledEvent>(
        [this, &registry](const ecs::EnemyKilledEvent& event) {
            auto& scores = registry.get_components<Score>();

            // Find the network player_id for the killer entity
            uint32_t killer_player_id = 0;
            uint32_t killer_score = 0;

            if (event.killer != 0 && player_entities_) {
                // Find which player_id corresponds to the killer entity
                for (const auto& [player_id, player_entity] : *player_entities_) {
                    if (player_entity == event.killer) {
                        killer_player_id = player_id;
                        if (scores.has_entity(event.killer)) {
                            killer_score = scores[event.killer].value;
                        }
                        break;
                    }
                }
            }

            protocol::ServerScoreUpdatePayload score_update;
            score_update.player_id = ByteOrder::host_to_net32(killer_player_id);
            score_update.entity_id = ByteOrder::host_to_net32(static_cast<uint32_t>(event.killer));
            score_update.score_delta = ByteOrder::host_to_net32(event.scoreValue);
            score_update.new_total_score = ByteOrder::host_to_net32(killer_score);
            {
                std::lock_guard lock(scores_mutex_);
                pending_scores_.push(score_update);
            }
        });

    explosionSubId_ = registry.get_event_bus().subscribe<ecs::ExplosionEvent>(
        [this](const ecs::ExplosionEvent& event) {
            protocol::ServerExplosionPayload payload;
            payload.source_entity_id = ByteOrder::host_to_net32(static_cast<uint32_t>(event.source));
            payload.position_x = event.x;
            payload.position_y = event.y;
            payload.effect_scale = event.scale;
            {
                std::lock_guard lock(explosions_mutex_);
                pending_explosions_.push(payload);
            }
        });

    // Subscribe to bonus collected events for network sync
    bonusCollectedSubId_ = registry.get_event_bus().subscribe<ecs::BonusCollectedEvent>(
        [this](const ecs::BonusCollectedEvent& event) {
            // Map BonusType to PowerupType
            protocol::PowerupType powerupType = protocol::PowerupType::WEAPON_UPGRADE;
            switch (event.bonusType) {
                case 0: powerupType = protocol::PowerupType::HEALTH; break;
                case 1: powerupType = protocol::PowerupType::SHIELD; break;
                case 2: powerupType = protocol::PowerupType::SPEED; break;
                case 3: powerupType = protocol::PowerupType::WEAPON_UPGRADE; break;
                default: powerupType = protocol::PowerupType::WEAPON_UPGRADE; break;
            }
            queue_powerup_collected(static_cast<uint32_t>(event.player), powerupType);
        });
}

void ServerNetworkSystem::update(Registry& registry, float dt)
{
    for (auto& [player_id, cooldown] : shoot_cooldowns_)
        cooldown += dt;
    for (auto& [player_id, cooldown] : switch_cooldowns_)
        cooldown += dt;
    for (auto& [enemy_id, cooldown] : enemy_shoot_cooldowns_)
        cooldown += dt;
    process_pending_inputs(registry);
    update_enemy_shooting(registry, dt);
    auto& projectiles = registry.get_components<Projectile>();
    for (size_t i = 0; i < projectiles.size(); ++i) {
        Entity entity = projectiles.get_entity_at(i);
        Projectile& proj = projectiles.get_data_at(i);
        proj.time_alive += dt;
        if (proj.time_alive >= proj.lifetime)
            registry.add_component(entity, ToDestroy{});
    }
    auto& to_destroy = registry.get_components<ToDestroy>();
    for (size_t i = 0; i < to_destroy.size(); ++i) {
        Entity entity = to_destroy.get_entity_at(i);
        queue_entity_destroy(entity);
    }
    snapshot_timer_ += dt;
    if (snapshot_timer_ >= snapshot_interval_) {
        send_state_snapshot(registry);
        snapshot_timer_ = 0.0f;
        tick_count_++;
    }

    // Broadcast pending events to clients
    broadcast_pending_spawns();
    broadcast_pending_destroys();
    broadcast_pending_powerups();
    broadcast_pending_respawns();
    broadcast_pending_level_ups();
    broadcast_pending_level_transitions();
    broadcast_pending_level_ready();
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
    spawn.entity_id = ByteOrder::host_to_net32(entity);
    spawn.entity_type = type;
    spawn.spawn_x = x;
    spawn.spawn_y = y;
    spawn.subtype = subtype;
    spawn.health = ByteOrder::host_to_net16(health);

    std::lock_guard lock(spawns_mutex_);
    pending_spawns_.push(spawn);
}

void ServerNetworkSystem::queue_entity_destroy(Entity entity)
{
    std::lock_guard lock(destroys_mutex_);
    pending_destroys_.push(entity);
}

void ServerNetworkSystem::queue_powerup_collected(uint32_t player_id, protocol::PowerupType type)
{
    protocol::ServerPowerupCollectedPayload payload;
    payload.player_id = ByteOrder::host_to_net32(player_id);
    payload.powerup_type = type;
    payload.new_weapon_level = 1;

    std::lock_guard lock(powerups_mutex_);
    pending_powerups_.push(payload);
    std::cout << "[ServerNetworkSystem] Queued powerup collected: player=" << player_id
              << " type=" << static_cast<int>(type) << std::endl;
}

void ServerNetworkSystem::queue_player_respawn(uint32_t player_id, float x, float y,
                                               float invuln_duration, uint8_t lives)
{
    pending_respawns_.push_back({player_id, x, y, invuln_duration, lives});
    std::cout << "[ServerNetworkSystem] Queued player respawn: player=" << player_id
              << " pos=(" << x << "," << y << ") lives=" << static_cast<int>(lives) << std::endl;
}

void ServerNetworkSystem::queue_player_level_up(uint32_t player_id, Entity entity, uint8_t new_level,
                                                uint8_t new_ship_type, uint8_t new_weapon_type,
                                                uint8_t new_skin_id, uint32_t current_score)
{
    protocol::ServerPlayerLevelUpPayload payload;
    payload.player_id = ByteOrder::host_to_net32(player_id);
    payload.entity_id = ByteOrder::host_to_net32(static_cast<uint32_t>(entity));
    payload.new_level = new_level;
    payload.new_ship_type = new_ship_type;
    payload.new_weapon_type = new_weapon_type;
    payload.new_skin_id = new_skin_id;
    payload.current_score = ByteOrder::host_to_net32(current_score);

    std::lock_guard lock(level_ups_mutex_);
    pending_level_ups_.push(payload);
    std::cout << "[ServerNetworkSystem] Queued player level-up: player=" << player_id
              << " entity=" << entity << " level=" << static_cast<int>(new_level)
              << " skin_id=" << static_cast<int>(new_skin_id) << std::endl;
}

void ServerNetworkSystem::queue_level_transition(uint16_t next_level_id)
{
    protocol::ServerLevelTransitionPayload payload;
    payload.next_level_id = ByteOrder::host_to_net16(next_level_id);
    pending_level_transitions_.push(payload);
    std::cout << "[ServerNetworkSystem] Queued level transition to level " << next_level_id << "\n";
}

void ServerNetworkSystem::queue_level_ready(uint16_t level_id)
{
    protocol::ServerLevelReadyPayload payload;
    payload.level_id = ByteOrder::host_to_net16(level_id);
    pending_level_ready_.push(payload);
    std::cout << "[ServerNetworkSystem] Queued level ready for level " << level_id << "\n";
}

void ServerNetworkSystem::process_pending_inputs(Registry& registry)
{
    if (!player_entities_)
        return;
    auto& velocities = registry.get_components<Velocity>();

    while (!pending_inputs_.empty()) {
        auto [player_id, input] = pending_inputs_.front();
        pending_inputs_.pop();

        // Store last processed sequence number for lag compensation
        uint32_t sequence = ntohl(input.sequence_number);
        last_processed_input_seq_[player_id] = sequence;

        auto it = player_entities_->find(player_id);
        if (it == player_entities_->end())
            continue;
        Entity player_entity = it->second;
        if (!velocities.has_entity(player_entity))
            continue;
        Velocity& vel = velocities[player_entity];
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
        // Weapon switching removed - weapon is now determined by player level (LevelUpSystem)
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
    // Maximum entities that fit in a single snapshot packet
    // MAX_PAYLOAD_SIZE = 1387 bytes, header = 10 bytes, EntityState = 25 bytes
    constexpr size_t MAX_ENTITIES_PER_SNAPSHOT = 55;

    protocol::ServerSnapshotPayload snapshot;
    snapshot.server_tick = ByteOrder::host_to_net32(tick_count_);
    snapshot.scroll_x = ByteOrder::host_to_net_float(current_scroll_x_);
    std::vector<uint8_t> payload;
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&snapshot);
    payload.insert(payload.end(), header_bytes, header_bytes + sizeof(snapshot));

    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& healths = registry.get_components<Health>();
    auto& to_destroy = registry.get_components<ToDestroy>();
    auto& enemies = registry.get_components<Enemy>();
    auto& ais = registry.get_components<AI>();
    auto& projectiles = registry.get_components<Projectile>();
    auto& walls = registry.get_components<Wall>();
    auto& controllables = registry.get_components<Controllable>();

    // Collect entities by priority: players first, then projectiles, then enemies
    // Walls are excluded from snapshots - they are spawned once and scroll predictably
    std::vector<Entity> priority_entities;
    std::vector<Entity> projectile_entities;
    std::vector<Entity> enemy_entities;

    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        if (to_destroy.has_entity(entity))
            continue;

        if (controllables.has_entity(entity)) {
            priority_entities.push_back(entity);  // Players - highest priority
        } else if (projectiles.has_entity(entity)) {
            projectile_entities.push_back(entity);  // Projectiles - high priority
        } else if (enemies.has_entity(entity)) {
            enemy_entities.push_back(entity);  // Enemies - medium priority
        }
        // Walls are NOT included in snapshots - they scroll predictably
    }

    std::vector<protocol::EntityState> entity_states;
    entity_states.reserve(MAX_ENTITIES_PER_SNAPSHOT);

    auto add_entity_state = [&](Entity entity) {
        if (entity_states.size() >= MAX_ENTITIES_PER_SNAPSHOT)
            return;
        if (!positions.has_entity(entity))
            return;

        Position& pos = positions[entity];
        protocol::EntityState state;
        state.entity_id = ByteOrder::host_to_net32(entity);

        // Determine entity type
        if (controllables.has_entity(entity)) {
            state.entity_type = protocol::EntityType::PLAYER;
        } else if (enemies.has_entity(entity)) {
            if (ais.has_entity(entity)) {
                switch (ais[entity].type) {
                    case EnemyType::Fast:
                        state.entity_type = protocol::EntityType::ENEMY_FAST;
                        break;
                    case EnemyType::Tank:
                        state.entity_type = protocol::EntityType::ENEMY_TANK;
                        break;
                    case EnemyType::Boss:
                        state.entity_type = protocol::EntityType::ENEMY_BOSS;
                        break;
                    default:
                        state.entity_type = protocol::EntityType::ENEMY_BASIC;
                        break;
                }
            } else {
                state.entity_type = protocol::EntityType::ENEMY_BASIC;
            }
        } else if (projectiles.has_entity(entity)) {
            const Projectile& proj = projectiles[entity];
            state.entity_type = (proj.faction == ProjectileFaction::Player)
                ? protocol::EntityType::PROJECTILE_PLAYER
                : protocol::EntityType::PROJECTILE_ENEMY;
        } else {
            return;  // Unknown entity type
        }

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
            state.health = ByteOrder::host_to_net16(static_cast<uint16_t>(health.current));
        } else {
            state.health = 0;
        }
        state.flags = 0;
        state.last_ack_sequence = 0;

        // Include last acknowledged input for players
        if (player_entities_ && controllables.has_entity(entity)) {
            for (const auto& [player_id, player_entity] : *player_entities_) {
                if (player_entity == entity) {
                    auto it = last_processed_input_seq_.find(player_id);
                    if (it != last_processed_input_seq_.end()) {
                        state.last_ack_sequence = ByteOrder::host_to_net32(it->second);
                    }
                    break;
                }
            }
        }

        entity_states.push_back(state);
    };

    // Add entities in priority order
    for (Entity e : priority_entities)
        add_entity_state(e);
    for (Entity e : projectile_entities)
        add_entity_state(e);
    for (Entity e : enemy_entities)
        add_entity_state(e);

    uint16_t entity_count = static_cast<uint16_t>(entity_states.size());
    snapshot.entity_count = ByteOrder::host_to_net16(entity_count);
    Memory::copy(payload.data(), &snapshot, sizeof(snapshot));
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
    }
}

void ServerNetworkSystem::broadcast_pending_explosions()
{
    if (!listener_)
        return;
    while (!pending_explosions_.empty()) {
        const auto& payload = pending_explosions_.front();
        listener_->on_explosion_triggered(session_id_, serialize(payload));
        pending_explosions_.pop();
    }
}

void ServerNetworkSystem::broadcast_pending_scores()
{
    if (!listener_)
        return;
    while (!pending_scores_.empty()) {
        const auto& score = pending_scores_.front();
        std::vector<uint8_t> payload;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&score);
        payload.insert(payload.end(), bytes, bytes + sizeof(score));
        listener_->on_score_updated(session_id_, payload);
        pending_scores_.pop();
    }
}

void ServerNetworkSystem::broadcast_pending_powerups()
{
    if (!listener_)
        return;
    while (!pending_powerups_.empty()) {
        const auto& powerup = pending_powerups_.front();
        std::vector<uint8_t> payload;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&powerup);
        payload.insert(payload.end(), bytes, bytes + sizeof(powerup));

        // Send multiple times to ensure delivery (UDP can lose packets)
        for (int i = 0; i < 5; ++i) {
            listener_->on_powerup_collected(session_id_, payload);
        }

        pending_powerups_.pop();
    }
}

void ServerNetworkSystem::broadcast_pending_respawns()
{
    if (!listener_)
        return;
    for (const auto& respawn : pending_respawns_) {
        protocol::ServerPlayerRespawnPayload payload;
        payload.player_id = htonl(respawn.player_id);
        payload.respawn_x = respawn.x;
        payload.respawn_y = respawn.y;
        payload.invulnerability_duration = htons(static_cast<uint16_t>(respawn.invuln_duration * 1000));
        payload.lives_remaining = respawn.lives_remaining;

        std::vector<uint8_t> payload_bytes;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&payload);
        payload_bytes.insert(payload_bytes.end(), bytes, bytes + sizeof(payload));

        // Single send - respawn is also communicated via entity spawn packet
        listener_->on_player_respawn(session_id_, payload_bytes);
    }
    pending_respawns_.clear();
}

void ServerNetworkSystem::broadcast_pending_level_ups()
{
    if (!listener_)
        return;
    std::lock_guard lock(level_ups_mutex_);
    while (!pending_level_ups_.empty()) {
        const auto& level_up = pending_level_ups_.front();
        std::vector<uint8_t> payload;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&level_up);
        payload.insert(payload.end(), bytes, bytes + sizeof(level_up));

        // Send multiple times to ensure delivery (UDP can lose packets)
        for (int i = 0; i < 3; ++i) {
            listener_->on_player_level_up(session_id_, payload);
        }

        pending_level_ups_.pop();
    }
}

void ServerNetworkSystem::broadcast_pending_level_transitions()
{
    if (!listener_)
        return;
    while (!pending_level_transitions_.empty()) {
        const auto& payload = pending_level_transitions_.front();
        std::vector<uint8_t> data;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&payload);
        data.insert(data.end(), bytes, bytes + sizeof(payload));

        // Send multiple times to ensure delivery
        for (int i = 0; i < 5; ++i) {
            listener_->on_level_transition(session_id_, data);
        }
        
        pending_level_transitions_.pop();
        std::cout << "[ServerNetworkSystem] Broadcast level transition to clients\n";
    }
}

void ServerNetworkSystem::broadcast_pending_level_ready()
{
    if (!listener_)
        return;
    while (!pending_level_ready_.empty()) {
        const auto& payload = pending_level_ready_.front();
        std::vector<uint8_t> data;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&payload);
        data.insert(data.end(), bytes, bytes + sizeof(payload));

        // Send multiple times to ensure delivery
        for (int i = 0; i < 5; ++i) {
            listener_->on_level_ready(session_id_, data);
        }

        pending_level_ready_.pop();
        std::cout << "[ServerNetworkSystem] Broadcast level ready to clients\n";
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
    registry.add_component(projectile, ProjectileOwner{owner});  // Track who fired this projectile
    registry.add_component(projectile, NoFriction{});
    protocol::ServerProjectileSpawnPayload spawn;
    spawn.projectile_id = ByteOrder::host_to_net32(projectile);
    spawn.owner_id = ByteOrder::host_to_net32(owner);
    spawn.projectile_type = protocol::ProjectileType::BULLET;
    spawn.spawn_x = x;
    spawn.spawn_y = y;
    spawn.velocity_x = static_cast<int16_t>(config::PROJECTILE_SPEED);
    spawn.velocity_y = 0;
    pending_projectiles_.push(spawn);
}

void ServerNetworkSystem::spawn_enemy_projectile(Registry& registry, Entity owner, float x, float y)
{
    Entity projectile = registry.spawn_entity();

    // Apply difficulty multiplier to enemy projectile damage
    float damage_mult = rtype::shared::config::get_damage_multiplier(difficulty_);
    int scaled_damage = static_cast<int>(static_cast<float>(config::ENEMY_PROJECTILE_DAMAGE) * damage_mult);

    registry.add_component(projectile, Position{x, y});
    registry.add_component(projectile, Velocity{-config::PROJECTILE_SPEED, 0.0f});
    registry.add_component(projectile, Collider{config::PROJECTILE_WIDTH, config::PROJECTILE_HEIGHT});
    registry.add_component(projectile, Damage{scaled_damage});
    registry.add_component(projectile, Projectile{0.0f, config::PROJECTILE_LIFETIME, 0.0f, ProjectileFaction::Enemy});
    registry.add_component(projectile, ProjectileOwner{owner});  // Track which enemy fired this
    registry.add_component(projectile, NoFriction{});
    protocol::ServerProjectileSpawnPayload spawn;
    spawn.projectile_id = ByteOrder::host_to_net32(projectile);
    spawn.owner_id = ByteOrder::host_to_net32(owner);
    spawn.projectile_type = protocol::ProjectileType::BULLET;
    spawn.spawn_x = x;
    spawn.spawn_y = y;
    spawn.velocity_x = static_cast<int16_t>(-config::PROJECTILE_SPEED);
    spawn.velocity_y = 0;
    pending_projectiles_.push(spawn);
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

std::queue<protocol::ServerEntitySpawnPayload> ServerNetworkSystem::drain_pending_spawns()
{
    std::lock_guard lock(spawns_mutex_);
    std::queue<protocol::ServerEntitySpawnPayload> temp;
    temp.swap(pending_spawns_);
    return temp;
}

std::queue<uint32_t> ServerNetworkSystem::drain_pending_destroys()
{
    std::lock_guard lock(destroys_mutex_);
    std::queue<uint32_t> temp;
    temp.swap(pending_destroys_);
    return temp;
}

std::queue<protocol::ServerProjectileSpawnPayload> ServerNetworkSystem::drain_pending_projectiles()
{
    std::lock_guard lock(projectiles_mutex_);
    std::queue<protocol::ServerProjectileSpawnPayload> temp;
    temp.swap(pending_projectiles_);
    return temp;
}

std::queue<protocol::ServerExplosionPayload> ServerNetworkSystem::drain_pending_explosions()
{
    std::lock_guard lock(explosions_mutex_);
    std::queue<protocol::ServerExplosionPayload> temp;
    temp.swap(pending_explosions_);
    return temp;
}

std::queue<protocol::ServerScoreUpdatePayload> ServerNetworkSystem::drain_pending_scores()
{
    std::lock_guard lock(scores_mutex_);
    std::queue<protocol::ServerScoreUpdatePayload> temp;
    temp.swap(pending_scores_);
    return temp;
}

std::queue<protocol::ServerPlayerLevelUpPayload> ServerNetworkSystem::drain_pending_level_ups()
{
    std::lock_guard lock(level_ups_mutex_);
    std::queue<protocol::ServerPlayerLevelUpPayload> temp;
    temp.swap(pending_level_ups_);
    return temp;
}

}
