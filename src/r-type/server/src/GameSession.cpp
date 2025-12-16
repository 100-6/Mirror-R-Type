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
    , session_start_time_(std::chrono::steady_clock::now()) {

    std::cout << "[GameSession " << session_id_ << "] Created (mode: " << static_cast<int>(game_mode)
              << ", difficulty: " << static_cast<int>(difficulty) << ", map: " << map_id << ")\n";

    // Register ECS components
    registry_.register_component<Position>();
    registry_.register_component<Velocity>();
    registry_.register_component<Health>();
    registry_.register_component<Controllable>();
    registry_.register_component<Enemy>();

    // Register ServerNetworkSystem
    registry_.register_system<ServerNetworkSystem>(session_id_, config::SNAPSHOT_INTERVAL);
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
    wave_manager_.set_wave_start_callback([this](uint32_t wave_num, const std::string& wave_name) {
        std::cout << "[GameSession " << session_id_ << "] Wave " << wave_num << " started: " << wave_name << "\n";
        // TODO: Send SERVER_WAVE_START to clients
    });

    wave_manager_.set_wave_complete_callback([this](uint32_t wave_num) {
        std::cout << "[GameSession " << session_id_ << "] Wave " << wave_num << " completed\n";
        // TODO: Send SERVER_WAVE_COMPLETE to clients
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
            0
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

    wave_manager_.update(delta_time, current_scroll_);
    update_ecs_systems(delta_time);

    // Run all registered systems (including ServerNetworkSystem for snapshots)
    registry_.run_systems(delta_time);

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
    std::cout << "[GameSession " << session_id_ << "] Spawned player entity " << entity
              << " at (" << spawn_x << ", " << spawn_y << ")\n";
}

void GameSession::spawn_enemy(const std::string& enemy_type, float x, float y) {
    Entity enemy = registry_.spawn_entity();
    float velocity_x = -config::ENEMY_BASIC_SPEED;
    uint16_t health = config::ENEMY_BASIC_HEALTH;
    protocol::EntityType entity_type = protocol::EntityType::ENEMY_BASIC;
    protocol::EnemySubtype subtype = protocol::EnemySubtype::BASIC;

    if (enemy_type == "fast") {
        velocity_x = -config::ENEMY_FAST_SPEED;
        health = config::ENEMY_FAST_HEALTH;
        entity_type = protocol::EntityType::ENEMY_FAST;
        subtype = protocol::EnemySubtype::FAST;
    }
    else if (enemy_type == "tank") {
        velocity_x = -config::ENEMY_TANK_SPEED;
        health = config::ENEMY_TANK_HEALTH;
        entity_type = protocol::EntityType::ENEMY_TANK;
        subtype = protocol::EnemySubtype::TANK;
    }
    else if (enemy_type == "boss") {
        velocity_x = -config::ENEMY_BOSS_SPEED;
        health = config::ENEMY_BOSS_HEALTH;
        entity_type = protocol::EntityType::ENEMY_BOSS;
        subtype = protocol::EnemySubtype::BOSS;
    }

    // Add components to entity
    registry_.add_component(enemy, Position{x, y});
    registry_.add_component(enemy, Velocity{velocity_x, 0.0f});
    registry_.add_component(enemy, Health{static_cast<int>(health), static_cast<int>(health)});
    registry_.add_component(enemy, Enemy{});

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

void GameSession::check_game_over() {
    if (players_.empty()) {
        is_active_ = false;
        std::cout << "[GameSession " << session_id_ << "] Game over - no players remaining\n";
        if (game_over_callback_)
            game_over_callback_(session_id_, {});
    }
}

void GameSession::update_ecs_systems(float delta_time) {
    auto& positions = registry_.get_components<Position>();
    auto& velocities = registry_.get_components<Velocity>();

    // Update positions based on velocities
    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        Position& pos = positions.get_data_at(i);
        if (velocities.has_entity(entity)) {
            Velocity& vel = velocities[entity];
            pos.x += vel.x * delta_time;
            pos.y += vel.y * delta_time;
            // Clamp to screen bounds
            if (pos.x < 0.0f)
                pos.x = 0.0f;
            if (pos.x > config::SCREEN_WIDTH)
                pos.x = config::SCREEN_WIDTH;
            if (pos.y < 0.0f)
                pos.y = 0.0f;
            if (pos.y > config::SCREEN_HEIGHT)
                pos.y = config::SCREEN_HEIGHT;
        }
    }

    // Find entities to despawn
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

}
