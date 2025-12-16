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
    , snapshot_timer_(0.0f) {

    std::cout << "[GameSession " << session_id_ << "] Created (mode: " << static_cast<int>(game_mode)
              << ", difficulty: " << static_cast<int>(difficulty) << ", map: " << map_id << ")\n";
    registry_.register_component<Position>();
    registry_.register_component<Velocity>();
    registry_.register_component<Health>();
    registry_.register_component<Controllable>();
    registry_.register_component<Enemy>();
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

    std::cout << "[GameSession " << session_id_ << "] Player " << player_id << " (" << player_name
              << ") added (entity ID: " << player.entity << ")\n";
    if (entity_spawn_callback_) {
        protocol::ServerEntitySpawnPayload spawn;
        spawn.entity_id = htonl(player.entity);
        spawn.entity_type = protocol::EntityType::PLAYER;
        spawn.spawn_x = config::PLAYER_SPAWN_X;
        spawn.spawn_y = config::PLAYER_SPAWN_Y_BASE + ((players_.size() - 1) * config::PLAYER_SPAWN_Y_OFFSET);
        spawn.subtype = 0;
        spawn.health = htons(config::PLAYER_MAX_HEALTH);
        entity_spawn_callback_(session_id_, serialize(spawn));
    }
}

void GameSession::remove_player(uint32_t player_id) {
    auto it = players_.find(player_id);

    if (it == players_.end())
        return;
    Entity player_entity = it->second.entity;
    players_.erase(it);
    registry_.kill_entity(player_entity);
    std::cout << "[GameSession " << session_id_ << "] Player " << player_id << " removed\n";
    if (entity_destroy_callback_)
        entity_destroy_callback_(session_id_, player_entity);
    check_game_over();
}

void GameSession::handle_input(uint32_t player_id, const protocol::ClientInputPayload& input) {
    auto it = players_.find(player_id);

    if (it == players_.end())
        return;
    Entity player_entity = it->second.entity;
    auto& velocities = registry_.get_components<Velocity>();
    if (velocities.has_entity(player_entity)) {
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
    }
    if (input.is_shoot_pressed()) {
        // TODO: Spawn projectile entity
        // std::cout << "[GameSession " << session_id_ << "] Player " << player_id << " shooting\n";
    }
}

void GameSession::update(float delta_time) {
    if (!is_active_)
        return;
    tick_count_++;
    snapshot_timer_ += delta_time;
    current_scroll_ += config::GAME_SCROLL_SPEED * delta_time;

    wave_manager_.update(delta_time, current_scroll_);
    update_ecs_systems(delta_time);
    if (snapshot_timer_ >= config::SNAPSHOT_INTERVAL) {
        send_state_snapshot();
        snapshot_timer_ = 0.0f;
    }
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
    std::string sprite_name = "enemy_basic.png";

    if (enemy_type == "fast") {
        velocity_x = -config::ENEMY_FAST_SPEED;
        health = config::ENEMY_FAST_HEALTH;
        entity_type = protocol::EntityType::ENEMY_FAST;
        subtype = protocol::EnemySubtype::FAST;
        sprite_name = "enemy_fast.png";
    }
    else if (enemy_type == "tank") {
        velocity_x = -config::ENEMY_TANK_SPEED;
        health = config::ENEMY_TANK_HEALTH;
        entity_type = protocol::EntityType::ENEMY_TANK;
        subtype = protocol::EnemySubtype::TANK;
        sprite_name = "enemy_tank.png";
    }
    else if (enemy_type == "boss") {
        velocity_x = -config::ENEMY_BOSS_SPEED;
        health = config::ENEMY_BOSS_HEALTH;
        entity_type = protocol::EntityType::ENEMY_BOSS;
        subtype = protocol::EnemySubtype::BOSS;
        sprite_name = "enemy_boss.png";
    }
    // Use exact coordinates from wave config
    registry_.add_component(enemy, Position{x, y});
    registry_.add_component(enemy, Velocity{velocity_x, 0.0f});
    registry_.add_component(enemy, Health{static_cast<int>(health), static_cast<int>(health)});
    registry_.add_component(enemy, Enemy{});
    std::cout << "[GameSession " << session_id_ << "] Spawned " << enemy_type << " enemy " << enemy
              << " at (" << x << ", " << y << ")\n";
    if (entity_spawn_callback_) {
        protocol::ServerEntitySpawnPayload spawn;
        spawn.entity_id = htonl(enemy);
        spawn.entity_type = entity_type;
        spawn.spawn_x = x;  // Use exact X from wave config
        spawn.spawn_y = y;  // Use exact Y from wave config
        spawn.subtype = static_cast<uint8_t>(subtype);
        spawn.health = htons(health);
        entity_spawn_callback_(session_id_, serialize(spawn));
    }
}

void GameSession::send_state_snapshot() {
    if (!state_snapshot_callback_)
        return;
    protocol::ServerSnapshotPayload snapshot;
    snapshot.server_tick = htonl(tick_count_);
    std::vector<uint8_t> payload;
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&snapshot);

    payload.insert(payload.end(), header_bytes, header_bytes + sizeof(snapshot));
    auto& positions = registry_.get_components<Position>();
    auto& velocities = registry_.get_components<Velocity>();
    auto& healths = registry_.get_components<Health>();

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
        } else
            state.health = 0;
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
    state_snapshot_callback_(session_id_, payload);
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
    for (Entity entity : entities_to_kill) {
        registry_.kill_entity(entity);
        if (entity_destroy_callback_)
            entity_destroy_callback_(session_id_, entity);
    }
}

}
