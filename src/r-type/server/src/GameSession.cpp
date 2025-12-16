#include "GameSession.hpp"
#include "ServerConfig.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <cstring>

#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"

namespace rtype::server {

GameSession::GameSession(uint32_t session_id, protocol::GameMode game_mode, protocol::Difficulty difficulty, uint16_t map_id)
    : session_id_(session_id)
    , game_mode_(game_mode)
    , difficulty_(difficulty)
    , map_id_(map_id)
    , is_active_(true)
    , tick_count_(0)
    , current_scroll_(0.0f)
    , session_start_time_(std::chrono::steady_clock::now())
    , has_wave_started_(false)
    , has_wave_complete_(false) {

    std::cout << "[GameSession " << session_id_ << "] Created (mode: " << static_cast<int>(game_mode)
              << ", difficulty: " << static_cast<int>(difficulty) << ", map: " << map_id << ")\n";

    // Register ECS components
    registry_.register_component<Position>();
    registry_.register_component<Velocity>();
    registry_.register_component<Health>();
    registry_.register_component<Controllable>();
    registry_.register_component<Enemy>();
    registry_.register_component<NoFriction>();
    registry_.register_component<ToDestroy>();
    registry_.register_component<Collider>();
    registry_.register_component<Projectile>();
    registry_.register_component<Damage>();
    registry_.register_component<Invulnerability>();
    registry_.register_component<Score>();
    registry_.register_component<Wall>();
    registry_.register_component<Bonus>();
    registry_.register_component<Shield>();

    // Register game engine systems
    registry_.register_system<MovementSystem>();
    registry_.register_system<PhysiqueSystem>();
    registry_.register_system<CollisionSystem>();
    registry_.register_system<HealthSystem>();

    // Register ServerNetworkSystem BEFORE DestroySystem so it can queue destroys before entities are killed
    registry_.register_system<ServerNetworkSystem>(session_id_, config::SNAPSHOT_INTERVAL);

    // DestroySystem must be LAST to kill entities after network has queued destroy notifications
    registry_.register_system<DestroySystem>();
    network_system_ = &registry_.get_system<ServerNetworkSystem>();
    network_system_->set_player_entities(&player_entities_);

    // Load wave configuration
    std::string wave_file = WaveManager::get_map_file(game_mode, difficulty);
    if (wave_manager_.load_from_file(wave_file)) {
        std::cout << "[GameSession " << session_id_ << "] Loaded " << wave_manager_.get_total_waves() << " waves\n";
    } else {
        std::cerr << "[GameSession " << session_id_ << "] Failed to load wave config\n";
    }
    wave_manager_.set_spawn_enemy_callback([this](const std::string& enemy_type, float x, float y) {
        spawn_enemy(enemy_type, x, y);
    });
    wave_manager_.set_spawn_wall_callback([this](float x, float y) {
        spawn_wall(x, y);
    });
    wave_manager_.set_spawn_powerup_callback([this](const std::string& bonus_type, float x, float y) {
        spawn_powerup(bonus_type, x, y);
    });
    wave_manager_.set_wave_start_callback([this](const Wave& wave) {
        std::cout << "[GameSession " << session_id_ << "] Wave " << wave.wave_number << " started\n";
        if (!wave_start_network_callback_)
            return;
        protocol::ServerWaveStartPayload payload;
        payload.wave_number = htonl(wave.wave_number);
        payload.total_waves = htons(static_cast<uint16_t>(wave_manager_.get_total_waves()));
        payload.scroll_distance = wave.trigger.scroll_distance;
        uint16_t enemy_count = 0;
        for (const auto& spawn : wave.spawns) {
            if (spawn.type == "enemy")
                enemy_count += static_cast<uint16_t>(spawn.count);
        }
        payload.expected_enemies = htons(enemy_count);
        payload.set_wave_name("Wave " + std::to_string(wave.wave_number));
        last_wave_start_payload_ = payload;
        has_wave_started_ = true;
        wave_start_network_callback_(session_id_, serialize(payload));
    });

    wave_manager_.set_wave_complete_callback([this](const Wave& wave) {
        std::cout << "[GameSession " << session_id_ << "] Wave " << wave.wave_number << " completed\n";
        if (!wave_complete_network_callback_)
            return;
        protocol::ServerWaveCompletePayload payload;
        payload.wave_number = htonl(wave.wave_number);
        payload.completion_time = htonl(0);
        payload.enemies_killed = htons(0);
        payload.bonus_points = htons(0);
        payload.all_waves_complete = wave_manager_.all_waves_complete() ? 1 : 0;
        last_wave_complete_payload_ = payload;
        has_wave_complete_ = payload.all_waves_complete != 0;
        wave_complete_network_callback_(session_id_, serialize(payload));
    });
}

void GameSession::add_player(uint32_t player_id, const std::string& player_name) {
    if (players_.find(player_id) != players_.end()) {
        std::cerr << "[GameSession " << session_id_ << "] Player " << player_id << " already in session\n";
        return;
    }
    GamePlayer player(player_id, player_name);
    spawn_player_entity(player);
    players_[player_id] = player;
    player_entities_[player_id] = player.entity; // Update mapping for NetworkSystem

    std::cout << "[GameSession " << session_id_ << "] Player " << player_id << " (" << player_name
              << ") added (entity ID: " << player.entity << ")\n";

    // Queue spawn for broadcast via NetworkSystem
    if (network_system_) {
        float spawn_y = config::PLAYER_SPAWN_Y_BASE + ((players_.size() - 1) * config::PLAYER_SPAWN_Y_OFFSET);
        network_system_->queue_entity_spawn(
            player.entity,
            protocol::EntityType::PLAYER,
            config::PLAYER_SPAWN_X,
            spawn_y,
            config::PLAYER_MAX_HEALTH,
            static_cast<uint8_t>(player_id)  // Use subtype to send player_id so client can identify itself
        );
    }
}

void GameSession::remove_player(uint32_t player_id) {
    auto it = players_.find(player_id);

    if (it == players_.end())
        return;
    Entity player_entity = it->second.entity;
    players_.erase(it);
    player_entities_.erase(player_id); // Remove from mapping
    registry_.kill_entity(player_entity);
    std::cout << "[GameSession " << session_id_ << "] Player " << player_id << " removed\n";

    // Queue destroy for broadcast via NetworkSystem
    if (network_system_)
        network_system_->queue_entity_destroy(player_entity);
    check_game_over();
}

void GameSession::handle_input(uint32_t player_id, const protocol::ClientInputPayload& input) {
    // Delegate input processing to NetworkSystem
    if (network_system_)
        network_system_->queue_input(player_id, input);
}

void GameSession::update(float delta_time) {
    if (!is_active_)
        return;
    tick_count_++;
    current_scroll_ += config::GAME_SCROLL_SPEED * delta_time;

    // Debug scroll progress every 5 seconds
    static int scroll_log_counter = 0;
    if (++scroll_log_counter % 300 == 0) {
        std::cout << "[GameSession " << session_id_ << "] current_scroll=" << current_scroll_ << "\n";
    }

    wave_manager_.update(delta_time, current_scroll_);

    // ServerNetworkSystem now handles queueing destroy notifications BEFORE DestroySystem kills entities
    registry_.run_systems(delta_time);

    check_offscreen_enemies();

    if (wave_manager_.all_waves_complete()) {
        std::cout << "[GameSession " << session_id_ << "] All waves complete - game victory!\n";
        is_active_ = false;
    }
    check_game_over();
}

std::vector<uint32_t> GameSession::get_player_ids() const {
    std::vector<uint32_t> ids;

    ids.reserve(players_.size());
    for (const auto& [player_id, player] : players_)
        ids.push_back(player_id);
    return ids;
}

void GameSession::spawn_player_entity(GamePlayer& player) {
    Entity entity = registry_.spawn_entity();
    player.entity = entity;
    float spawn_x = config::PLAYER_SPAWN_X;
    float spawn_y = config::PLAYER_SPAWN_Y_BASE + (players_.size() * config::PLAYER_SPAWN_Y_OFFSET);

    registry_.add_component(entity, Position{spawn_x, spawn_y});
    registry_.add_component(entity, Velocity{0.0f, 0.0f});
    registry_.add_component(entity, Health{static_cast<int>(config::PLAYER_MAX_HEALTH), static_cast<int>(config::PLAYER_MAX_HEALTH)});
    registry_.add_component(entity, Controllable{config::PLAYER_MOVEMENT_SPEED});
    registry_.add_component(entity, Collider{config::PLAYER_WIDTH, config::PLAYER_HEIGHT});
    registry_.add_component(entity, Invulnerability{3.0f});  // 3 seconds invulnerability at spawn
    registry_.add_component(entity, Score{0});
    std::cout << "[GameSession " << session_id_ << "] Spawned player entity " << entity
              << " at (" << spawn_x << ", " << spawn_y << ")\n";
}

void GameSession::spawn_enemy(const std::string& enemy_type, float x, float y) {
    Entity enemy = registry_.spawn_entity();
    float velocity_x = -config::ENEMY_BASIC_SPEED;
    uint16_t health = config::ENEMY_BASIC_HEALTH;
    float width = config::ENEMY_BASIC_WIDTH;
    float height = config::ENEMY_BASIC_HEIGHT;
    protocol::EntityType entity_type = protocol::EntityType::ENEMY_BASIC;
    protocol::EnemySubtype subtype = protocol::EnemySubtype::BASIC;

    if (enemy_type == "fast") {
        velocity_x = -config::ENEMY_FAST_SPEED;
        health = config::ENEMY_FAST_HEALTH;
        width = config::ENEMY_FAST_WIDTH;
        height = config::ENEMY_FAST_HEIGHT;
        entity_type = protocol::EntityType::ENEMY_FAST;
        subtype = protocol::EnemySubtype::FAST;
    }
    else if (enemy_type == "tank") {
        velocity_x = -config::ENEMY_TANK_SPEED;
        health = config::ENEMY_TANK_HEALTH;
        width = config::ENEMY_TANK_WIDTH;
        height = config::ENEMY_TANK_HEIGHT;
        entity_type = protocol::EntityType::ENEMY_TANK;
        subtype = protocol::EnemySubtype::TANK;
    }
    else if (enemy_type == "boss") {
        velocity_x = -config::ENEMY_BOSS_SPEED;
        health = config::ENEMY_BOSS_HEALTH;
        width = config::ENEMY_BOSS_WIDTH;
        height = config::ENEMY_BOSS_HEIGHT;
        entity_type = protocol::EntityType::ENEMY_BOSS;
        subtype = protocol::EnemySubtype::BOSS;
    }

    // Add components to entity
    registry_.add_component(enemy, Position{x, y});
    registry_.add_component(enemy, Velocity{velocity_x, 0.0f});
    registry_.add_component(enemy, Health{static_cast<int>(health), static_cast<int>(health)});
    registry_.add_component(enemy, Enemy{});
    registry_.add_component(enemy, NoFriction{});  // Enemies keep constant velocity
    registry_.add_component(enemy, Collider{width, height});

    std::cout << "[GameSession " << session_id_ << "] Spawned " << enemy_type << " enemy " << enemy
              << " at (" << x << ", " << y << ")\n";

    // Queue spawn for broadcast via NetworkSystem
    if (network_system_) {
        network_system_->queue_entity_spawn(
            enemy,
            entity_type,
            x,
            y,
            health,
            static_cast<uint8_t>(subtype)
        );
    }
}

void GameSession::spawn_wall(float x, float y) {
    Entity wall = registry_.spawn_entity();

    registry_.add_component(wall, Position{x, y});
    registry_.add_component(wall, Velocity{-config::GAME_SCROLL_SPEED, 0.0f});  // Moves with scroll
    registry_.add_component(wall, Collider{config::WALL_WIDTH, config::WALL_HEIGHT});
    registry_.add_component(wall, Wall{});
    registry_.add_component(wall, NoFriction{});
    registry_.add_component(wall, Health{999999, 999999});  // Walls are indestructible

    std::cout << "[GameSession " << session_id_ << "] Spawned wall " << wall << " at (" << x << ", " << y << ")\n";

    // Queue spawn for broadcast via NetworkSystem
    if (network_system_) {
        network_system_->queue_entity_spawn(
            wall,
            protocol::EntityType::WALL,
            x,
            y,
            999999,
            0
        );
    }
}

void GameSession::spawn_powerup(const std::string& bonus_type, float x, float y) {
    Entity powerup = registry_.spawn_entity();

    registry_.add_component(powerup, Position{x, y});
    registry_.add_component(powerup, Velocity{-config::GAME_SCROLL_SPEED, 0.0f});  // Moves with scroll
    registry_.add_component(powerup, Collider{config::BONUS_SIZE, config::BONUS_SIZE});
    registry_.add_component(powerup, Bonus{});
    registry_.add_component(powerup, NoFriction{});

    std::cout << "[GameSession " << session_id_ << "] Spawned " << bonus_type << " powerup " << powerup << " at (" << x << ", " << y << ")\n";

    // Map bonus type to entity type
    protocol::EntityType entity_type = protocol::EntityType::BONUS_HEALTH;
    if (bonus_type == "shield")
        entity_type = protocol::EntityType::BONUS_SHIELD;
    else if (bonus_type == "speed")
        entity_type = protocol::EntityType::BONUS_SPEED;

    // Queue spawn for broadcast via NetworkSystem
    if (network_system_) {
        network_system_->queue_entity_spawn(
            powerup,
            entity_type,
            x,
            y,
            0,
            0
        );
    }
}

void GameSession::check_game_over() {
    if (players_.empty()) {
        is_active_ = false;
        std::cout << "[GameSession " << session_id_ << "] Game over - no players remaining\n";
        if (game_over_callback_)
            game_over_callback_(session_id_, {});
    }
}

void GameSession::process_destroyed_entities() {
    if (!network_system_)
        return;

    auto& to_destroy = registry_.get_components<ToDestroy>();

    // Queue all entities marked for destruction to be broadcast to clients
    for (size_t i = 0; i < to_destroy.size(); ++i) {
        Entity entity = to_destroy.get_entity_at(i);
        network_system_->queue_entity_destroy(entity);
        std::cout << "[GameSession " << session_id_ << "] Queued entity " << entity << " for network destroy\n";
    }
}

void GameSession::check_offscreen_enemies() {
    auto& positions = registry_.get_components<Position>();

    // Find entities to despawn (enemies that went off screen)
    std::vector<Entity> entities_to_kill;
    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        Position& pos = positions.get_data_at(i);
        bool is_player = false;

        for (const auto& [pid, player] : players_) {
            if (player.entity == entity) {
                is_player = true;
                break;
            }
        }
        // Despawn enemies that went off screen to the left
        if (!is_player && pos.x < config::ENTITY_OFFSCREEN_LEFT)
            entities_to_kill.push_back(entity);
    }

    // Kill entities and queue destroy for broadcast
    for (Entity entity : entities_to_kill) {
        registry_.kill_entity(entity);
        if (network_system_)
            network_system_->queue_entity_destroy(entity);
    }
}

void GameSession::resync_client(uint32_t player_id, uint32_t tcp_client_id) {
    if (!network_system_) {
        std::cerr << "[GameSession " << session_id_ << "] Cannot resync: no network system\n";
        return;
    }

    std::cout << "[GameSession " << session_id_ << "] Resyncing client " << tcp_client_id
              << " (player " << player_id << ") with existing entities\n";

    // Send spawn events for all existing entities
    auto& positions = registry_.get_components<Position>();
    auto& healths = registry_.get_components<Health>();

    int entity_count = 0;

    // Resync players
    for (const auto& [pid, player] : players_) {
        if (positions.has_entity(player.entity)) {
            const Position& pos = positions[player.entity];
            uint16_t health = 100;
            if (healths.has_entity(player.entity)) {
                health = healths[player.entity].current;
            }

            network_system_->queue_entity_spawn(
                player.entity,
                protocol::EntityType::PLAYER,
                pos.x,
                pos.y,
                health,
                static_cast<uint8_t>(pid)  // Send player_id in subtype
            );
            entity_count++;
        }
    }

    // Resync enemies
    auto& enemies = registry_.get_components<Enemy>();
    for (size_t i = 0; i < enemies.size(); ++i) {
        Entity entity = enemies.get_entity_at(i);
        if (positions.has_entity(entity)) {
            const Position& pos = positions[entity];
            uint16_t health = 100;
            if (healths.has_entity(entity)) {
                health = healths[entity].current;
            }

            network_system_->queue_entity_spawn(
                entity,
                protocol::EntityType::ENEMY_BASIC,
                pos.x,
                pos.y,
                health,
                0
            );
            entity_count++;
        }
    }

    // Resync projectiles
    auto& projectiles = registry_.get_components<Projectile>();
    for (size_t i = 0; i < projectiles.size(); ++i) {
        Entity entity = projectiles.get_entity_at(i);
        if (positions.has_entity(entity)) {
            const Position& pos = positions[entity];
            const Projectile& proj = projectiles.get_data_at(i);

            network_system_->queue_entity_spawn(
                entity,
                proj.faction == ProjectileFaction::Player ? protocol::EntityType::PROJECTILE_PLAYER : protocol::EntityType::PROJECTILE_ENEMY,
                pos.x,
                pos.y,
                0,
                0
            );
            entity_count++;
        }
    }

    if (wave_start_network_callback_ && has_wave_started_)
        wave_start_network_callback_(session_id_, serialize(last_wave_start_payload_));
    if (wave_complete_network_callback_ && has_wave_complete_)
        wave_complete_network_callback_(session_id_, serialize(last_wave_complete_payload_));

    std::cout << "[GameSession " << session_id_ << "] Queued " << entity_count
              << " entity spawns for resync\n";
}

}
