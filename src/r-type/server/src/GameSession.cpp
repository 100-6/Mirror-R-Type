#include "GameSession.hpp"
#include "ServerConfig.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <cmath>

#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"

namespace rtype::server {

// Screen dimensions for collision bounds
static constexpr float SCREEN_WIDTH = 1920.0f;
static constexpr float SCREEN_HEIGHT = 1080.0f;

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

    // Register ALL ECS components (same as solo game)
    registry_.register_component<Position>();
    registry_.register_component<Velocity>();
    registry_.register_component<Health>();
    registry_.register_component<Controllable>();
    registry_.register_component<Enemy>();
    registry_.register_component<Wall>();
    registry_.register_component<NoFriction>();
    registry_.register_component<ToDestroy>();
    registry_.register_component<Collider>();
    registry_.register_component<Projectile>();
    registry_.register_component<Damage>();
    registry_.register_component<Score>();
    registry_.register_component<Owner>();

    // Register game engine systems (server-side logic)
    registry_.register_system<MovementSystem>();
    registry_.register_system<PhysiqueSystem>();
    registry_.register_system<DestroySystem>();

    // Register ServerNetworkSystem (must be after other systems for proper snapshot timing)
    registry_.register_system<ServerNetworkSystem>(session_id_, config::SNAPSHOT_INTERVAL);
    network_system_ = &registry_.get_system<ServerNetworkSystem>();
    network_system_->set_player_entities(&player_entities_);

    // Connect shoot callback - when a player shoots, queue it for processing
    network_system_->set_shoot_callback([this](uint32_t player_id) {
        queue_shoot(player_id);
    });

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
    // For players, we use subtype to pass the player_id so the client can identify its own entity
    if (network_system_) {
        float spawn_y = config::PLAYER_SPAWN_Y_BASE + ((players_.size() - 1) * config::PLAYER_SPAWN_Y_OFFSET);
        network_system_->queue_entity_spawn(
            player.entity,
            protocol::EntityType::PLAYER,
            config::PLAYER_SPAWN_X,
            spawn_y,
            config::PLAYER_MAX_HEALTH,
            static_cast<uint8_t>(player_id & 0xFF)  // Pass player_id in subtype for identification
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

    // Process shooting requests from players
    process_shooting();

    // Run ECS systems (movement, physics, etc.)
    registry_.run_systems(delta_time);

    // Server-side collision detection
    check_collisions();

    // Remove off-screen entities
    check_offscreen_enemies();

    if (wave_manager_.all_waves_complete()) {
        std::cout << "[GameSession " << session_id_ << "] All waves complete - game victory!\n";
        is_active_ = false;
        if (game_over_callback_) {
            // Collect all player IDs for victory notification
            std::vector<uint32_t> player_ids;
            for (const auto& [pid, player] : players_) {
                player_ids.push_back(pid);
            }
            game_over_callback_(session_id_, player_ids);
        }
        return;  // Don't check game over again
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

    // Player hitbox size (matches client visual)
    const float PLAYER_WIDTH = 66.0f;   // 330 * 0.20 scale
    const float PLAYER_HEIGHT = 56.0f;  // 280 * 0.20 scale

    registry_.add_component(entity, Position{spawn_x, spawn_y});
    registry_.add_component(entity, Velocity{0.0f, 0.0f});
    registry_.add_component(entity, Health{static_cast<int>(config::PLAYER_MAX_HEALTH), static_cast<int>(config::PLAYER_MAX_HEALTH)});
    registry_.add_component(entity, Controllable{config::PLAYER_MOVEMENT_SPEED});
    registry_.add_component(entity, Collider{PLAYER_WIDTH, PLAYER_HEIGHT});
    registry_.add_component(entity, Score{0});

    std::cout << "[GameSession " << session_id_ << "] Spawned player entity " << entity
              << " at (" << spawn_x << ", " << spawn_y << ")\n";
}

void GameSession::spawn_enemy(const std::string& enemy_type, float x, float y) {
    Entity enemy = registry_.spawn_entity();
    float velocity_x = -config::ENEMY_BASIC_SPEED;
    uint16_t health = config::ENEMY_BASIC_HEALTH;
    protocol::EntityType entity_type = protocol::EntityType::ENEMY_BASIC;
    protocol::EnemySubtype subtype = protocol::EnemySubtype::BASIC;
    // Enemy sizes scaled 6x (multiplied by 6 to match client visual scale of 2.1)
    float enemy_size = 384.0f;  // Default enemy size (64 * 6)

    if (enemy_type == "fast") {
        velocity_x = -config::ENEMY_FAST_SPEED;
        health = config::ENEMY_FAST_HEALTH;
        entity_type = protocol::EntityType::ENEMY_FAST;
        subtype = protocol::EnemySubtype::FAST;
        enemy_size = 288.0f;  // 48 * 6
    }
    else if (enemy_type == "tank") {
        velocity_x = -config::ENEMY_TANK_SPEED;
        health = config::ENEMY_TANK_HEALTH;
        entity_type = protocol::EntityType::ENEMY_TANK;
        subtype = protocol::EnemySubtype::TANK;
        enemy_size = 480.0f;  // 80 * 6
    }
    else if (enemy_type == "boss") {
        velocity_x = -config::ENEMY_BOSS_SPEED;
        health = config::ENEMY_BOSS_HEALTH;
        entity_type = protocol::EntityType::ENEMY_BOSS;
        subtype = protocol::EnemySubtype::BOSS;
        enemy_size = 768.0f;  // 128 * 6
    }

    // Add components to entity
    registry_.add_component(enemy, Position{x, y});
    registry_.add_component(enemy, Velocity{velocity_x, 0.0f});
    registry_.add_component(enemy, Health{static_cast<int>(health), static_cast<int>(health)});
    registry_.add_component(enemy, Enemy{});
    registry_.add_component(enemy, NoFriction{});  // Enemies keep constant velocity
    registry_.add_component(enemy, Collider{enemy_size, enemy_size});
    registry_.add_component(enemy, Damage{10});  // Enemies deal 10 damage on contact

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
    float velocity_x = -config::GAME_SCROLL_SPEED; // Walls move with the scroll
    const float WALL_SIZE = 64.0f;

    registry_.add_component(wall, Position{x, y});
    registry_.add_component(wall, Velocity{velocity_x, 0.0f});
    registry_.add_component(wall, Wall{});
    registry_.add_component(wall, NoFriction{});
    registry_.add_component(wall, Collider{WALL_SIZE, WALL_SIZE});

    std::cout << "[GameSession " << session_id_ << "] Spawned wall " << wall
              << " at (" << x << ", " << y << ")\n";

    // Queue spawn for broadcast via NetworkSystem
    if (network_system_) {
        network_system_->queue_entity_spawn(
            wall,
            protocol::EntityType::WALL,
            x,
            y,
            0, // Walls have no health (indestructible)
            0
        );
    }
}

void GameSession::spawn_projectile(uint32_t owner_player_id, float x, float y, float vx, float vy) {
    Entity projectile = registry_.spawn_entity();

    const float BULLET_WIDTH = 32.0f;
    const float BULLET_HEIGHT = 8.0f;
    const int BULLET_DAMAGE = 25;

    registry_.add_component(projectile, Position{x, y});
    registry_.add_component(projectile, Velocity{vx, vy});
    registry_.add_component(projectile, Collider{BULLET_WIDTH, BULLET_HEIGHT});
    registry_.add_component(projectile, Projectile{0.0f, 5.0f, 0.0f, ProjectileFaction::Player});
    registry_.add_component(projectile, Damage{BULLET_DAMAGE});
    registry_.add_component(projectile, NoFriction{});
    registry_.add_component(projectile, Owner{owner_player_id});  // Track who shot this

    // Queue spawn for broadcast
    if (network_system_) {
        network_system_->queue_entity_spawn(
            projectile,
            protocol::EntityType::PROJECTILE,
            x,
            y,
            0,  // No health for projectiles
            static_cast<uint8_t>(owner_player_id & 0xFF)
        );
    }
}

void GameSession::process_shooting() {
    // Process pending shoots
    for (uint32_t player_id : pending_shoots_) {
        auto it = players_.find(player_id);
        if (it == players_.end())
            continue;

        Entity player_entity = it->second.entity;
        auto& positions = registry_.get_components<Position>();

        if (!positions.has_entity(player_entity))
            continue;

        Position& pos = positions[player_entity];

        // Spawn projectile to the right of the player
        const float BULLET_SPEED = 800.0f;
        const float PLAYER_WIDTH = 66.0f;
        spawn_projectile(player_id, pos.x + PLAYER_WIDTH, pos.y + 20.0f, BULLET_SPEED, 0.0f);
    }
    pending_shoots_.clear();        Projectile& proj = projectiles.get_data_at(i);

        if (proj.faction != ProjectileFaction::Player)
            continue;  // Only player projectiles damage enemies

        if (!positions.has_entity(proj_entity) || !colliders.has_entity(proj_entity))
            continue;

        Position& proj_pos = positions[proj_entity];
        Collider& proj_col = colliders[proj_entity];

        // Check against all enemies
        for (size_t j = 0; j < enemies.size(); ++j) {
            Entity enemy_entity = enemies.get_entity_at(j);

            if (!positions.has_entity(enemy_entity) || !colliders.has_entity(enemy_entity))
                continue;

            Position& enemy_pos = positions[enemy_entity];
            Collider& enemy_col = colliders[enemy_entity];

            // AABB collision check
            bool collision =
                proj_pos.x < enemy_pos.x + enemy_col.width &&
                proj_pos.x + proj_col.width > enemy_pos.x &&
                proj_pos.y < enemy_pos.y + enemy_col.height &&
                proj_pos.y + proj_col.height > enemy_pos.y;

            if (collision) {
                // Damage enemy
                if (healths.has_entity(enemy_entity)) {
                    int damage = damages.has_entity(proj_entity) ? damages[proj_entity].value : 25;
                    healths[enemy_entity].current -= damage;

                    if (healths[enemy_entity].current <= 0) {
                        entities_to_destroy.push_back(enemy_entity);

                        // Award points to the player who shot this projectile
                        if (owners.has_entity(proj_entity)) {
                            uint32_t owner_player_id = owners[proj_entity].player_id;
                            auto player_it = players_.find(owner_player_id);
                            if (player_it != players_.end()) {
                                Entity player_entity = player_it->second.entity;
                                if (scores.has_entity(player_entity)) {
                                    scores[player_entity].value += 100;  // 100 points per kill
                                    std::cout << "[GameSession] Player " << owner_player_id
                                             << " killed enemy, score: " << scores[player_entity].value << "\n";
                                }
                            }
                        }
                    }
                }
                // Destroy projectile
                entities_to_destroy.push_back(proj_entity);
                break;  // Projectile can only hit one enemy
            }
        }
    }

    // Check projectile vs wall collisions
    for (size_t i = 0; i < projectiles.size(); ++i) {
        Entity proj_entity = projectiles.get_entity_at(i);

        if (!positions.has_entity(proj_entity) || !colliders.has_entity(proj_entity))
            continue;

        Position& proj_pos = positions[proj_entity];
        Collider& proj_col = colliders[proj_entity];

        // Check against all walls
        for (size_t j = 0; j < walls.size(); ++j) {
            Entity wall_entity = walls.get_entity_at(j);

            if (!positions.has_entity(wall_entity) || !colliders.has_entity(wall_entity))
                continue;

            Position& wall_pos = positions[wall_entity];
            Collider& wall_col = colliders[wall_entity];

            // AABB collision check
            bool collision =
                proj_pos.x < wall_pos.x + wall_col.width &&
                proj_pos.x + proj_col.width > wall_pos.x &&
                proj_pos.y < wall_pos.y + wall_col.height &&
                proj_pos.y + proj_col.height > wall_pos.y;

            if (collision) {
                // Destroy projectile when it hits a wall
                entities_to_destroy.push_back(proj_entity);
                std::cout << "[GameSession] Projectile " << proj_entity << " destroyed by wall\n";
                break;  // Projectile is destroyed, no need to check other walls
            }
        }
    }

    // Check player vs enemy/wall collisions
    for (const auto& [player_id, player] : players_) {
        Entity player_entity = player.entity;

        if (!positions.has_entity(player_entity) || !colliders.has_entity(player_entity))
            continue;

        Position& player_pos = positions[player_entity];
        Collider& player_col = colliders[player_entity];

        // Check against walls (push player back)
        for (size_t i = 0; i < walls.size(); ++i) {
            Entity wall_entity = walls.get_entity_at(i);

            if (!positions.has_entity(wall_entity) || !colliders.has_entity(wall_entity))
                continue;

            Position& wall_pos = positions[wall_entity];
            Collider& wall_col = colliders[wall_entity];

            // AABB collision check
            bool collision =
                player_pos.x < wall_pos.x + wall_col.width &&
                player_pos.x + player_col.width > wall_pos.x &&
                player_pos.y < wall_pos.y + wall_col.height &&
                player_pos.y + player_col.height > wall_pos.y;

            if (collision) {
                // Push player out of wall (simple resolution)
                float overlap_left = (player_pos.x + player_col.width) - wall_pos.x;
                float overlap_right = (wall_pos.x + wall_col.width) - player_pos.x;
                float overlap_top = (player_pos.y + player_col.height) - wall_pos.y;
                float overlap_bottom = (wall_pos.y + wall_col.height) - player_pos.y;

                float min_overlap = std::min({overlap_left, overlap_right, overlap_top, overlap_bottom});

                if (min_overlap == overlap_left) player_pos.x -= overlap_left;
                else if (min_overlap == overlap_right) player_pos.x += overlap_right;
                else if (min_overlap == overlap_top) player_pos.y -= overlap_top;
                else player_pos.y += overlap_bottom;
            }
        }

        // Check against enemies (damage player)
        for (size_t i = 0; i < enemies.size(); ++i) {
            Entity enemy_entity = enemies.get_entity_at(i);

            if (!positions.has_entity(enemy_entity) || !colliders.has_entity(enemy_entity))
                continue;

            Position& enemy_pos = positions[enemy_entity];
            Collider& enemy_col = colliders[enemy_entity];

            // AABB collision check
            bool collision =
                player_pos.x < enemy_pos.x + enemy_col.width &&
                player_pos.x + player_col.width > enemy_pos.x &&
                player_pos.y < enemy_pos.y + enemy_col.height &&
                player_pos.y + player_col.height > enemy_pos.y;

            if (collision && healths.has_entity(player_entity)) {
                int damage = damages.has_entity(enemy_entity) ? damages[enemy_entity].value : 10;
                healths[player_entity].current -= damage;

                // Destroy enemy on collision
                entities_to_destroy.push_back(enemy_entity);

                if (healths[player_entity].current <= 0) {
                    // Player dies - will be handled by check_game_over
                }
            }
        }
    }

    // Destroy entities and notify clients
    for (Entity entity : entities_to_destroy) {
        registry_.kill_entity(entity);
        if (network_system_)
            network_system_->queue_entity_destroy(entity);
    }
}

void GameSession::check_game_over() {
    // Check if ALL players are dead (not just if players_ is empty)
    auto& healths = registry_.get_components<Health>();

    int alive_players = 0;
    for (const auto& [player_id, player] : players_) {
        if (healths.has_entity(player.entity) && healths[player.entity].current > 0) {
            alive_players++;
        }
    }

    // Only trigger game over if ALL players are dead
    if (alive_players == 0 && !players_.empty()) {
        is_active_ = false;
        std::cout << "[GameSession " << session_id_ << "] Game over - all players dead\n";
        if (game_over_callback_) {
            // Collect all player IDs
            std::vector<uint32_t> player_ids;
            for (const auto& [pid, player] : players_) {
                player_ids.push_back(pid);
            }
            game_over_callback_(session_id_, player_ids);
        }
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

}
