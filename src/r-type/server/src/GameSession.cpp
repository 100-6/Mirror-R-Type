/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameSession implementation
*/

#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #undef ERROR
#else
    #include <arpa/inet.h>
#endif

#include "GameSession.hpp"
#include "ServerConfig.hpp"
#include "systems/ShootingSystem.hpp"
#include "systems/BonusSystem.hpp"
#include "systems/BonusWeaponSystem.hpp"
#include "systems/AttachmentSystem.hpp"
#include "systems/ScoreSystem.hpp"
#include "systems/LevelUpSystem.hpp"
#include "components/CombatHelpers.hpp"
#include "components/ShipComponents.hpp"
#include "components/PlayerLevelComponent.hpp"
#include "components/LevelComponents.hpp"

#undef ENEMY_BASIC_SPEED
#undef ENEMY_BASIC_HEALTH
#undef ENEMY_FAST_SPEED
#undef ENEMY_FAST_HEALTH
#undef ENEMY_TANK_SPEED
#undef ENEMY_TANK_HEALTH
#undef ENEMY_BOSS_SPEED
#undef ENEMY_BOSS_HEALTH

#include "NetworkUtils.hpp"
#include <iostream>

#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include "ecs/events/InputEvents.hpp"
#include "ecs/events/GameEvents.hpp"

namespace rtype::server {
using netutils::ByteOrder;
using protocol::EntityType;

GameSession::GameSession(uint32_t session_id, protocol::GameMode game_mode,
                         protocol::Difficulty difficulty, uint16_t map_id)
    : session_id_(session_id)
    , game_mode_(game_mode)
    , difficulty_(difficulty)
    , map_id_(map_id)
    , is_active_(true)
    , tick_count_(0)
    , current_scroll_(0.0)
    , scroll_speed_(config::GAME_SCROLL_SPEED)
    , session_start_time_(std::chrono::steady_clock::now())
{
    // std::cout << "[GameSession " << session_id_ << "] Created (mode: " << static_cast<int>(game_mode)
    //           << ", difficulty: " << static_cast<int>(difficulty) << ", map: " << map_id << ")\n";

    registry_.register_component<Position>();
    registry_.register_component<Velocity>();
    registry_.register_component<Health>();
    registry_.register_component<Controllable>();
    registry_.register_component<Enemy>();
    registry_.register_component<AI>();
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
    registry_.register_component<Weapon>();
    registry_.register_component<SpeedBoost>();
    registry_.register_component<Scrollable>();
    registry_.register_component<Attached>();
    registry_.register_component<Sprite>();
    registry_.register_component<TextEffect>();
    registry_.register_component<ShotAnimation>();
    registry_.register_component<CircleEffect>();
    registry_.register_component<BonusWeapon>();
    registry_.register_component<BonusLifetime>();
    registry_.register_component<Camera>();

    // Register Level System components
    registry_.register_component<game::LevelController>();
    registry_.register_component<game::PlayerLevel>();
    // CheckpointManager removed
    registry_.register_component<game::BossPhase>();
    registry_.register_component<game::PlayerLives>();
    registry_.register_component<game::ScrollState>();

    registry_.register_system<MovementSystem>();
    registry_.register_system<PhysiqueSystem>();
    registry_.register_system<AttachmentSystem>();
    registry_.register_system<CollisionSystem>();
    registry_.register_system<HealthSystem>();
    registry_.register_system<ShootingSystem>();
    registry_.register_system<BonusSystem>();
    registry_.register_system<BonusWeaponSystem>();
    registry_.register_system<ScoreSystem>();
    registry_.register_system<game::LevelUpSystem>();

    registry_.get_system<HealthSystem>().init(registry_);
    registry_.get_system<ShootingSystem>().init(registry_);
    registry_.get_system<BonusSystem>().init(registry_);
    registry_.get_system<BonusWeaponSystem>().init(registry_);
    registry_.get_system<ScoreSystem>().init(registry_);
    registry_.get_system<game::LevelUpSystem>().init(registry_);

    registry_.register_system<ServerNetworkSystem>(session_id_, config::SNAPSHOT_INTERVAL);
    network_system_ = &registry_.get_system<ServerNetworkSystem>();
    network_system_->init(registry_);
    network_system_->set_player_entities(&player_entities_);
    network_system_->set_listener(this);

    // Set up level-up callback for network broadcasting (after network_system_ is initialized)
    registry_.get_system<game::LevelUpSystem>().set_level_up_callback(
        [this](Entity entity, uint8_t new_level, uint8_t new_skin_id) {
            // Find player_id from entity
            uint32_t player_id = 0;
            for (const auto& [pid, ent] : player_entities_) {
                if (ent == entity) {
                    player_id = pid;
                    break;
                }
            }
            if (player_id == 0) {
                std::cerr << "[GameSession] Cannot find player_id for leveled-up entity " << entity << "\n";
                return;
            }

            // Get current score
            uint32_t current_score = 0;
            auto& scores = registry_.get_components<Score>();
            if (scores.has_entity(entity)) {
                current_score = static_cast<uint32_t>(scores[entity].value);
            }

            // Queue level-up for broadcast
            uint8_t ship_type = game::get_ship_type_for_level(new_level);
            uint8_t weapon_type = static_cast<uint8_t>(game::get_weapon_type_for_level(new_level));
            network_system_->queue_player_level_up(player_id, entity, new_level, ship_type, weapon_type, new_skin_id, current_score);

            // Also update the entity spawn for new players joining (so they see correct skin)
            auto& positions = registry_.get_components<Position>();
            auto& healths = registry_.get_components<Health>();
            if (positions.has_entity(entity)) {
                float x = positions[entity].x;
                float y = positions[entity].y;
                uint16_t health = healths.has_entity(entity) ? static_cast<uint16_t>(healths[entity].current) : 100;
                uint8_t subtype = static_cast<uint8_t>(((player_id & 0x0F) << 4) | (new_skin_id & 0x0F));
                network_system_->queue_entity_spawn(entity, protocol::EntityType::PLAYER, x, y, health, subtype);
            }

            std::cout << "[GameSession] Player " << player_id << " leveled up to " << static_cast<int>(new_level)
                      << " (skin_id=" << static_cast<int>(new_skin_id) << ")\n";
        }
    );

    registry_.register_system<DestroySystem>();

    // Register Level System systems
    registry_.register_system<game::LevelSystem>();
    registry_.register_system<game::BossSystem>();
    registry_.register_system<game::CheckpointSystem>();

    // Wire CheckpointSystem callbacks
    if (registry_.has_system<game::CheckpointSystem>()) {
        auto& checkpoint_system = registry_.get_system<game::CheckpointSystem>();



        // Callback 2A: Broadcast respawn info (Legacy - handled by spawn_player now)
        checkpoint_system.set_network_callback(
            [this](uint32_t player_id, float x, float y, float invuln, uint8_t lives) {
                // Keep for compatibility if needed, but logic moved to respawn_player_at
            }
        );

        // Callback 2B: Request player spawn
        checkpoint_system.set_spawn_player_callback(
            [this](uint32_t player_id, float x, float y, float invuln, uint8_t lives) {
                return respawn_player_at(player_id, x, y, invuln, lives);
            }
        );



        // Callback 4: Game Over (All players dead)
        checkpoint_system.set_game_over_callback(
            [this]() {
                std::cout << "[GameSession] 💀 GAME OVER - All players exhausted lives\n";

                // Transition LevelSystem to GAME_OVER state
                // This will be picked up by GameSession::update() to end the session
                if (registry_.has_system<game::LevelSystem>()) {
                    auto& level_controllers = registry_.get_components<game::LevelController>();
                    if (level_controllers.size() > 0) {
                        game::LevelController& lc = level_controllers.get_data_at(0);
                        registry_.get_system<game::LevelSystem>().transition_to_game_over(lc);
                    }
                }
            }
        );
    }

    registry_.get_event_bus().subscribe<ecs::EntityDeathEvent>(
        [this](const ecs::EntityDeathEvent& event) {
            if (!event.isPlayer)
                return;
            for (auto& [player_id, player] : players_) {
                if (player.entity == event.entity && player.is_alive) {
                    player.is_alive = false;

                    // Destroy companion turret (BonusWeapon) if player has one
                    auto& bonusWeapons = registry_.get_components<BonusWeapon>();
                    if (bonusWeapons.has_entity(event.entity)) {
                        const BonusWeapon& bonusWeapon = bonusWeapons[event.entity];
                        if (bonusWeapon.weaponEntity != static_cast<size_t>(-1)) {
                            Entity companionEntity = static_cast<Entity>(bonusWeapon.weaponEntity);
                            registry_.add_component(companionEntity, ToDestroy{});
                            if (network_system_)
                                network_system_->queue_entity_destroy(companionEntity);
                        }
                    }

                    if (network_system_)
                        network_system_->queue_entity_destroy(event.entity);

                    // Notify CheckpointSystem to handle respawn or game over
                    if (registry_.has_system<game::CheckpointSystem>()) {
                        auto& checkpoint_system = registry_.get_system<game::CheckpointSystem>();
                        checkpoint_system.on_player_death(registry_, event.entity, player_id);
                    }
                    break;
                }
            }
        });

    // Subscribe to bonus spawn events to sync with clients
    registry_.get_event_bus().subscribe<ecs::BonusSpawnEvent>(
        [this](const ecs::BonusSpawnEvent& event) {
            // Create bonus entity for network sync
            Entity bonus = registry_.spawn_entity();
            registry_.add_component(bonus, Position{event.x, event.y});
            registry_.add_component(bonus, Velocity{-config::GAME_SCROLL_SPEED, 0.0f});
            registry_.add_component(bonus, Collider{config::BONUS_SIZE, config::BONUS_SIZE});

            BonusType type = static_cast<BonusType>(event.bonusType);
            registry_.add_component(bonus, Bonus{type, config::BONUS_SIZE / 2.0f});
            registry_.add_component(bonus, NoFriction{});
            registry_.add_component(bonus, Scrollable{1.0f, false, true}); // Scroll and destroy when off-screen
            registry_.add_component(bonus, BonusLifetime{15.0f}); // 15 seconds max lifetime

            // Determine EntityType for network
            EntityType entityType = EntityType::BONUS_HEALTH;
            if (type == BonusType::SHIELD) {
                entityType = EntityType::BONUS_SHIELD;
            } else if (type == BonusType::SPEED) {
                entityType = EntityType::BONUS_SPEED;
            } else if (type == BonusType::BONUS_WEAPON) {
                entityType = EntityType::BONUS_HEALTH; // Use health type for now, client will handle color
            }

            // std::cout << "[GameSession " << session_id_ << "] Spawned bonus at ("
            //           << event.x << ", " << event.y << ") type=" << event.bonusType << "\n";

            if (network_system_)
                network_system_->queue_entity_spawn(bonus, entityType, event.x, event.y, 0, static_cast<uint8_t>(event.bonusType));
        });

    // Subscribe to companion spawn events to create BonusWeapon component on server
    registry_.get_event_bus().subscribe<ecs::CompanionSpawnEvent>(
        [this](const ecs::CompanionSpawnEvent& event) {
            auto& bonusWeapons = registry_.get_components<BonusWeapon>();
            auto& positions = registry_.get_components<Position>();

            // Check if player already has a companion
            if (bonusWeapons.has_entity(event.player)) {
                std::cout << "[GameSession " << session_id_ << "] Player " << event.player
                          << " already has companion, ignoring spawn\n";
                return;
            }

            if (!positions.has_entity(event.player)) {
                std::cerr << "[GameSession " << session_id_ << "] Player has no position, cannot spawn companion\n";
                return;
            }

            const Position& playerPos = positions[event.player];

            // Create companion entity (server-side, no sprite needed)
            Entity companionEntity = registry_.spawn_entity();

            // Position offset relative to player
            float bonusOffsetX = 120.0f;
            float bonusOffsetY = -70.0f;

            registry_.add_component(companionEntity, Position{
                playerPos.x + bonusOffsetX,
                playerPos.y + bonusOffsetY
            });

            registry_.add_component(companionEntity, Attached{
                event.player,
                bonusOffsetX,
                bonusOffsetY,
                4.0f  // Smooth follow factor
            });

            // Mark player as having a bonus weapon (stores companion entity reference)
            registry_.add_component(event.player, BonusWeapon{companionEntity, 0.0f, true});

            std::cout << "[GameSession " << session_id_ << "] Created server-side companion entity "
                      << companionEntity << " for player " << event.player << "\n";
        });

    // === INITIALIZE LEVEL SYSTEM ===
    // Map the map_id (0-based index from UI) directly to level_id
    // map_id 0 = test level, 1 = level 1, 2 = level 2, 3 = level 3, 4 = instant boss
    uint8_t starting_level = static_cast<uint8_t>(map_id);
    // Note: map_id comes from UI selection, can be 0-99 for debug levels

    if (!level_manager_.load_level(starting_level)) {
        std::cerr << "[GameSession " << session_id_ << "] Failed to load level " << static_cast<int>(starting_level) << "\n";
    } else {
        // std::cout << "[GameSession " << session_id_ << "] Loaded level: " << level_manager_.get_level_name() << "\n";
    }

    // Extract all waves from level phases and load into WaveManager
    std::vector<Wave> all_waves;
    for (const auto& phase : level_manager_.get_phases()) {
        for (const auto& wave : phase.waves) {
            all_waves.push_back(wave);
        }
    }
    wave_manager_.load_from_phases(all_waves);
    wave_manager_.set_listener(this);

    // Create LevelController entity
    Entity level_controller_entity = registry_.spawn_entity();
    game::LevelController lc;
    lc.current_level = starting_level;
    lc.state = game::LevelState::LEVEL_START;
    lc.state_timer = 0.0f;
    lc.current_phase_index = 0;
    lc.current_wave_in_phase = 0;
    lc.boss_spawned = false;
    lc.boss_entity = engine::INVALID_HANDLE; // Checkpoint Manager removed - Dynamic Respawn used
    registry_.add_component(level_controller_entity, lc);

    // Create ScrollState component for checkpoint system
    game::ScrollState scroll_state;
    scroll_state.current_scroll = 0.0f;
    registry_.add_component(level_controller_entity, scroll_state);

    // Create Camera entity for scroll management via ECS
    // Camera position.x = current scroll offset, updated by MovementSystem
    Entity camera_entity = registry_.spawn_entity();
    registry_.add_component(camera_entity, Position{0.0f, 0.0f});
    registry_.add_component(camera_entity, Velocity{scroll_speed_, 0.0f});  // Scroll speed as velocity
    registry_.add_component(camera_entity, Camera{scroll_speed_});
    registry_.add_component(camera_entity, NoFriction{});  // Camera doesn't slow down

    // std::cout << "[GameSession " << session_id_ << "] Level system initialized with "
    //           << cm.checkpoints.size() << " checkpoints\n";

    // Load map segments for tile-based walls
    load_map_segments(map_id);
}

void GameSession::add_player(uint32_t player_id, const std::string& player_name, uint8_t skin_id)
{
    if (players_.find(player_id) != players_.end()) {
        std::cerr << "[GameSession " << session_id_ << "] Player " << player_id << " already in session\n";
        return;
    }
    GamePlayer player(player_id, player_name, skin_id);
    spawn_player_entity(player);
    players_[player_id] = player;
    player_entities_[player_id] = player.entity;

    // Initialize PlayerLives for level system
    Entity player_lives_entity = registry_.spawn_entity();
    game::PlayerLives pl;
    pl.player_id = player_id;
    pl.lives_remaining = config::PLAYER_LIVES;  // Lives from config
    pl.respawn_pending = false;
    pl.respawn_timer = 0.0f;
    // pl.checkpoint_index removed
    registry_.add_component(player_lives_entity, pl);

    // std::cout << "[GameSession " << session_id_ << "] Player " << player_id << " (" << player_name
    //           << ") added with skin " << static_cast<int>(skin_id) << " (entity ID: " << player.entity << ", lives: 3)\n";
    if (network_system_) {
        float spawn_y = config::PLAYER_SPAWN_Y_BASE + ((players_.size() - 1) * config::PLAYER_SPAWN_Y_OFFSET);
        // Get actual skin_id from PlayerLevel component (which was just created in spawn_player_entity)
        uint8_t actual_skin_id = skin_id;  // fallback
        auto& player_levels = registry_.get_components<game::PlayerLevel>();
        if (player_levels.has_entity(player.entity)) {
            const auto& pl = player_levels[player.entity];
            actual_skin_id = game::compute_skin_id(pl.current_level, pl.color_id);
        }
        // Encode both player_id and actual_skin_id in subtype: high 4 bits = player_id, low 4 bits = skin_id
        uint8_t subtype = static_cast<uint8_t>(((player_id & 0x0F) << 4) | (actual_skin_id & 0x0F));
        network_system_->queue_entity_spawn(
            player.entity,
            EntityType::PLAYER,
            config::PLAYER_SPAWN_X,
            spawn_y,
            config::PLAYER_MAX_HEALTH,
            subtype
        );
    }
}

void GameSession::remove_player(uint32_t player_id)
{
    auto it = players_.find(player_id);

    if (it == players_.end())
        return;
    Entity player_entity = it->second.entity;
    players_.erase(it);
    player_entities_.erase(player_id);
    registry_.kill_entity(player_entity);
    // std::cout << "[GameSession " << session_id_ << "] Player " << player_id << " removed\n";
    if (network_system_)
        network_system_->queue_entity_destroy(player_entity);
    check_game_over();
}

void GameSession::handle_input(uint32_t player_id, const protocol::ClientInputPayload& input)
{
    if (network_system_)
        network_system_->queue_input(player_id, input);
}

void GameSession::update(float delta_time)
{
    if (!is_active_)
        return;

    if (is_paused_) {
        if (network_system_)
            network_system_->update(registry_, delta_time);
        return;
    }

    tick_count_++;

    // Run ECS systems first - this includes MovementSystem which updates Camera position
    // CollisionSystem reads scroll from Camera entity directly (pure ECS)
    registry_.get_system<ShootingSystem>().update(registry_, delta_time);
    registry_.get_system<BonusSystem>().update(registry_, delta_time);
    registry_.get_system<BonusWeaponSystem>().update(registry_, delta_time);
    registry_.run_systems(delta_time);

    // Read scroll from Camera entity (updated by MovementSystem)
    // Camera.position.x = current scroll offset
    auto& cameras = registry_.get_components<Camera>();
    auto& positions = registry_.get_components<Position>();
    if (cameras.size() > 0) {
        Entity camera_entity = cameras.get_entity_at(0);
        if (positions.has_entity(camera_entity)) {
            current_scroll_ = static_cast<double>(positions[camera_entity].x);
        }
    }

    // Synchronize scroll position with network system for client synchronization
    if (network_system_)
        network_system_->set_scroll_x(current_scroll_);

    // Update ScrollState component for checkpoint system
    auto& scroll_states = registry_.get_components<game::ScrollState>();
    if (scroll_states.size() > 0) {
        scroll_states.get_data_at(0).current_scroll = current_scroll_;
    }

    // Spawn walls from map tiles as they come into view
    spawn_walls_in_view();

    wave_manager_.update(delta_time, current_scroll_);

    // Update checkpoint system (handles respawn timers)
    if (registry_.has_system<game::CheckpointSystem>()) {
        registry_.get_system<game::CheckpointSystem>().update(registry_, delta_time);
    }

    check_offscreen_enemies();

    // === LEVEL SYSTEM VICTORY/DEFEAT CHECKS ===
    auto& level_controllers = registry_.get_components<game::LevelController>();
    if (level_controllers.size() > 0) {
        game::LevelController& lc = level_controllers.get_data_at(0);

        // Update wave completion status for LevelSystem
        lc.all_waves_triggered = wave_manager_.all_waves_complete();

        // Check if boss should be spawned
        if (lc.state == game::LevelState::BOSS_FIGHT && !lc.boss_spawned) {
            // Spawn boss
            const BossConfig& boss_config = level_manager_.get_boss_config();

            Entity boss_entity = registry_.spawn_entity();

            // Position
            registry_.add_component(boss_entity, Position{boss_config.spawn_position_x, boss_config.spawn_position_y});

            // Velocity
            registry_.add_component(boss_entity, Velocity{0.0f, 0.0f});

            // Enemy component with bonus drop for boss
            Enemy enemy;
            // Boss can drop a BONUS_WEAPON with 100% chance
            enemy.bonusDrop.enabled = true;
            enemy.bonusDrop.bonusType = BonusType::BONUS_WEAPON;
            enemy.bonusDrop.dropChance = 1.0f;
            registry_.add_component(boss_entity, enemy);

            // Health (500 HP for boss)
            Health health;
            health.max = 500;
            health.current = 500;
            registry_.add_component(boss_entity, health);

            // Collider (large boss)
            registry_.add_component(boss_entity, Collider{100.0f, 80.0f});

            // Sprite (texture handled by client)
            Sprite sprite;
            sprite.width = 100.0f;
            sprite.height = 80.0f;
            sprite.layer = 4;
            registry_.add_component(boss_entity, sprite);

            // BossPhase component
            game::BossPhase boss_phase;
            boss_phase.current_phase = 0;
            boss_phase.total_phases = boss_config.total_phases;
            boss_phase.phase_health_thresholds = {1.0f, 0.66f, 0.33f};
            boss_phase.phase_timer = 0.0f;
            boss_phase.attack_cooldown = 2.0f;
            boss_phase.attack_pattern_index = 0;

            // Safety check: ensure phases vector is not empty
            if (boss_config.phases.empty()) {
                std::cerr << "[GameSession] ERROR: Boss has no phases configured! Cannot spawn boss.\n";
                registry_.kill_entity(boss_entity);
                return;
            }

            boss_phase.movement_pattern = boss_config.phases[0].movement_pattern;
            boss_phase.movement_speed_multiplier = boss_config.phases[0].movement_speed_multiplier;
            boss_phase.phase_configs = boss_config.phases;
            registry_.add_component(boss_entity, boss_phase);

            // Update level controller
            lc.boss_spawned = true;
            lc.boss_entity = boss_entity;

            // std::cout << "[GameSession] Boss spawned: " << boss_config.boss_name << "\n";

            // Notify network
            if (network_system_) {
                network_system_->queue_entity_spawn(
                    boss_entity,
                    protocol::EntityType::ENEMY_BOSS,
                    boss_config.spawn_position_x,
                    boss_config.spawn_position_y,
                    500,  // Boss HP
                    0
                );
            }
        }

        // Level complete - load next level or declare victory
        if (lc.state == game::LevelState::LEVEL_COMPLETE) {
            if (lc.current_level >= 3) {
                // Final victory: Level 3 complete
                // std::cout << "[GameSession " << session_id_ << "] FINAL VICTORY - All 3 levels complete!\n";
                is_active_.store(false, std::memory_order_release);
                if (listener_)
                    listener_->on_game_over(session_id_, get_player_ids(), true);  // Victory
                return;
            } else {
                // Load next level
                uint8_t next_level = lc.current_level + 1;
                // std::cout << "[GameSession " << session_id_ << "] Loading level " << static_cast<int>(next_level) << "...\n";

                // Load next level configuration
                if (!level_manager_.load_level(next_level)) {
                    std::cerr << "[GameSession " << session_id_ << "] Failed to load level " << static_cast<int>(next_level) << "\n";
                    return;
                }

                // Clear all enemies and projectiles
                auto& enemies = registry_.get_components<Enemy>();
                std::vector<Entity> enemies_to_kill;
                for (size_t i = 0; i < enemies.size(); ++i) {
                    enemies_to_kill.push_back(enemies.get_entity_at(i));
                }
                for (Entity e : enemies_to_kill) {
                    registry_.kill_entity(e);
                }

                auto& projectiles = registry_.get_components<Projectile>();
                std::vector<Entity> projectiles_to_kill;
                for (size_t i = 0; i < projectiles.size(); ++i) {
                    projectiles_to_kill.push_back(projectiles.get_entity_at(i));
                }
                for (Entity p : projectiles_to_kill) {
                    registry_.kill_entity(p);
                }

                // Extract waves from new level and load into WaveManager
                std::vector<Wave> all_waves;
                for (const auto& phase : level_manager_.get_phases()) {
                    for (const auto& wave : phase.waves) {
                        all_waves.push_back(wave);
                    }
                }
                wave_manager_.load_from_phases(all_waves);

                // Update level controller
                lc.current_level = next_level;
                lc.state = game::LevelState::LEVEL_START;
                lc.state_timer = 0.0f;
                lc.current_phase_index = 0;
                lc.current_wave_in_phase = 0;
                lc.boss_spawned = false;
                lc.boss_entity = engine::INVALID_HANDLE;
                lc.all_waves_triggered = false;

                // Update checkpoint manager with new level checkpoints
                // CheckpointManager removed - Dynamic respawn implementation used instead

                // std::cout << "[GameSession " << session_id_ << "] Level " << static_cast<int>(next_level)
                //           << " loaded: " << level_manager_.get_level_name() << "\n";
            }
        }

        // Defeat: All players out of lives
        if (lc.state == game::LevelState::GAME_OVER) {
            // std::cout << "[GameSession " << session_id_ << "] GAME OVER - All players dead!\n";
            is_active_.store(false, std::memory_order_release);
            if (listener_)
                listener_->on_game_over(session_id_, get_player_ids(), false);  // Defeat
            return;
        }
    }

    check_game_over();
}

std::vector<uint32_t> GameSession::get_player_ids() const
{
    std::vector<uint32_t> ids;
    ids.reserve(players_.size());
    for (const auto& [player_id, player] : players_)
        ids.push_back(player_id);
    return ids;
}

void GameSession::spawn_player_entity(GamePlayer& player)
{
    Entity entity = registry_.spawn_entity();
    player.entity = entity;
    float spawn_x = config::PLAYER_SPAWN_X;
    float spawn_y = config::PLAYER_SPAWN_Y_BASE + (players_.size() * config::PLAYER_SPAWN_Y_OFFSET);

    // Extract color_id from player's skin selection
    // skin_id from client is now just color (0-2), for backward compatibility also handle old format (0-14)
    uint8_t color_id = (player.skin_id < 3) ? player.skin_id : game::get_color_from_skin_id(player.skin_id);

    // Create PlayerLevel component - all players start at level 1
    game::PlayerLevel player_level;
    player_level.current_level = 1;
    player_level.color_id = color_id;
    player_level.level_up_pending = false;
    player_level.level_up_timer = 0.0f;
    registry_.add_component(entity, player_level);

    // Calculate actual skin_id based on level (level 1 = SCOUT ship type = 0)
    uint8_t actual_skin_id = game::compute_skin_id(player_level.current_level, color_id);

    // Get ship-specific hitbox dimensions based on the level-determined skin_id
    auto hitbox = rtype::game::get_hitbox_dimensions_from_skin_id(actual_skin_id);
    float center_x = spawn_x + hitbox.width / 2.0f;
    float center_y = spawn_y + hitbox.height / 2.0f;

    registry_.add_component(entity, Position{center_x, center_y});
    registry_.add_component(entity, Velocity{0.0f, 0.0f});
    registry_.add_component(entity, Health{static_cast<int>(config::PLAYER_MAX_HEALTH), static_cast<int>(config::PLAYER_MAX_HEALTH)});
    registry_.add_component(entity, Controllable{config::PLAYER_MOVEMENT_SPEED});
    registry_.add_component(entity, Collider{hitbox.width, hitbox.height});
    registry_.add_component(entity, Invulnerability{3.0f});
    registry_.add_component(entity, Score{0});

    // Create weapon based on player level (level 1 = BASIC weapon)
    WeaponType weapon_type = game::get_weapon_type_for_level(player_level.current_level);
    Weapon weapon = create_weapon(weapon_type, engine::INVALID_HANDLE);
    registry_.add_component(entity, weapon);

    std::cout << "[GameSession " << session_id_ << "] Spawned player entity " << entity
              << " (level " << static_cast<int>(player_level.current_level)
              << ", color " << static_cast<int>(color_id)
              << ", skin_id " << static_cast<int>(actual_skin_id) << ")\n";
}

void GameSession::on_wave_started(const Wave& wave)
{
    std::cout << "[GameSession " << session_id_ << "] Wave " << wave.wave_number << " started\n";
    if (!listener_)
        return;
    protocol::ServerWaveStartPayload payload;
    payload.wave_number = ByteOrder::host_to_net32(wave.wave_number);
    payload.total_waves = ByteOrder::host_to_net16(static_cast<uint16_t>(wave_manager_.get_total_waves()));
    payload.scroll_distance = wave.trigger.scroll_distance;
    uint16_t enemy_count = 0;
    for (const auto& spawn : wave.spawns) {
        if (spawn.type == "enemy")
            enemy_count += static_cast<uint16_t>(spawn.count);
    }
    payload.expected_enemies = ByteOrder::host_to_net16(enemy_count);
    payload.set_wave_name("Wave " + std::to_string(wave.wave_number));
    last_wave_start_payload_ = payload;
    has_wave_started_ = true;
    listener_->on_wave_start(session_id_, serialize(payload));
}

void GameSession::on_wave_completed(const Wave& wave)
{
    std::cout << "[GameSession " << session_id_ << "] Wave " << wave.wave_number << " completed\n";
    if (!listener_)
        return;
    protocol::ServerWaveCompletePayload payload;
    payload.wave_number = ByteOrder::host_to_net32(wave.wave_number);
    payload.completion_time = ByteOrder::host_to_net32(0);
    payload.enemies_killed = ByteOrder::host_to_net16(0);
    payload.bonus_points = ByteOrder::host_to_net16(0);
    payload.all_waves_complete = wave_manager_.all_waves_complete() ? 1 : 0;
    last_wave_complete_payload_ = payload;
    has_wave_complete_ = payload.all_waves_complete != 0;

    listener_->on_wave_complete(session_id_, serialize(payload));
}

void GameSession::on_spawn_enemy(const std::string& enemy_type, float x, float y, const BonusDropConfig& bonus_drop)
{
    Entity enemy = registry_.spawn_entity();
    float velocity_x = -config::ENEMY_BASIC_SPEED;
    uint16_t health = config::ENEMY_BASIC_HEALTH;
    float width = config::ENEMY_BASIC_WIDTH;
    float height = config::ENEMY_BASIC_HEIGHT;
    EntityType entity_type = EntityType::ENEMY_BASIC;
    protocol::EnemySubtype subtype = protocol::EnemySubtype::BASIC;
    EnemyType ai_type = EnemyType::Basic;

    if (enemy_type == "fast") {
        velocity_x = -config::ENEMY_FAST_SPEED;
        health = config::ENEMY_FAST_HEALTH;
        width = config::ENEMY_FAST_WIDTH;
        height = config::ENEMY_FAST_HEIGHT;
        entity_type = EntityType::ENEMY_FAST;
        subtype = protocol::EnemySubtype::FAST;
        ai_type = EnemyType::Fast;
    } else if (enemy_type == "tank") {
        velocity_x = -config::ENEMY_TANK_SPEED;
        health = config::ENEMY_TANK_HEALTH;
        width = config::ENEMY_TANK_WIDTH;
        height = config::ENEMY_TANK_HEIGHT;
        entity_type = EntityType::ENEMY_TANK;
        subtype = protocol::EnemySubtype::TANK;
        ai_type = EnemyType::Tank;
    } else if (enemy_type == "boss") {
        velocity_x = -config::ENEMY_BOSS_SPEED;
        health = config::ENEMY_BOSS_HEALTH;
        width = config::ENEMY_BOSS_WIDTH;
        height = config::ENEMY_BOSS_HEIGHT;
        entity_type = EntityType::ENEMY_BOSS;
        subtype = protocol::EnemySubtype::BOSS;
        ai_type = EnemyType::Boss;
    }
    float center_x = x + width / 2.0f;
    float center_y = y + height / 2.0f;
    registry_.add_component(enemy, Position{center_x, center_y});
    registry_.add_component(enemy, Velocity{velocity_x, 0.0f});
    registry_.add_component(enemy, Health{static_cast<int>(health), static_cast<int>(health)});

    // Add AI component for enemy type detection in snapshots
    AI ai;
    ai.type = ai_type;
    ai.moveSpeed = std::abs(velocity_x);
    registry_.add_component(enemy, ai);

    // Convert BonusDropConfig to BonusDrop component
    BonusDrop drop;
    drop.enabled = bonus_drop.enabled;
    drop.dropChance = bonus_drop.drop_chance;
    if (bonus_drop.bonus_type == "health") {
        drop.bonusType = BonusType::HEALTH;
    } else if (bonus_drop.bonus_type == "shield") {
        drop.bonusType = BonusType::SHIELD;
    } else if (bonus_drop.bonus_type == "speed") {
        drop.bonusType = BonusType::SPEED;
    } else if (bonus_drop.bonus_type == "bonus_weapon") {
        drop.bonusType = BonusType::BONUS_WEAPON;
    }

    registry_.add_component(enemy, Enemy{drop});
    registry_.add_component(enemy, NoFriction{});
    registry_.add_component(enemy, Collider{width, height});

    // std::cout << "[GameSession " << session_id_ << "] Spawned " << enemy_type << " enemy " << enemy
    //           << " at (" << x << ", " << y << ")";
    // if (bonus_drop.enabled) {
    //     std::cout << " with bonusDrop: " << bonus_drop.bonus_type;
    // }
    // std::cout << "\n";

    if (network_system_)
        network_system_->queue_entity_spawn(enemy, entity_type, center_x, center_y, health, static_cast<uint8_t>(subtype));
}

void GameSession::on_spawn_wall(float x, float y)
{
    Entity wall = registry_.spawn_entity();
    float center_x = x + config::WALL_WIDTH / 2.0f;
    float center_y = y + config::WALL_HEIGHT / 2.0f;

    registry_.add_component(wall, Position{center_x, center_y});
    registry_.add_component(wall, Velocity{-scroll_speed_, 0.0f});
    registry_.add_component(wall, Collider{config::WALL_WIDTH, config::WALL_HEIGHT});
    registry_.add_component(wall, Wall{});
    registry_.add_component(wall, NoFriction{});
    registry_.add_component(wall, Health{65535, 65535});
    // std::cout << "[GameSession " << session_id_ << "] Spawned wall " << wall << " at (" << x << ", " << y << ")\n";

    // DO NOT send walls to client - client loads walls from tilemap via ChunkManager
    // Server uses these walls for server-side collision validation only
    // if (network_system_)
    //     network_system_->queue_entity_spawn(wall, EntityType::WALL, center_x, center_y, 65535, 0);
}

void GameSession::on_spawn_powerup(const std::string& bonus_type, float x, float y)
{
    // Disabled: powerups only come from enemy drops now
    (void)bonus_type;
    (void)x;
    (void)y;
}

void GameSession::on_snapshot_ready(uint32_t session_id, const std::vector<uint8_t>& snapshot)
{
    if (listener_)
        listener_->on_state_snapshot(session_id, snapshot);
}

void GameSession::on_entity_spawned(uint32_t session_id, const std::vector<uint8_t>& spawn_data)
{
    if (listener_)
        listener_->on_entity_spawn(session_id, spawn_data);
}

void GameSession::on_entity_destroyed(uint32_t session_id, uint32_t entity_id)
{
    if (listener_)
        listener_->on_entity_destroy(session_id, entity_id);
}

void GameSession::on_projectile_spawned(uint32_t session_id, const std::vector<uint8_t>& projectile_data)
{
    if (listener_)
        listener_->on_projectile_spawn(session_id, projectile_data);
}

void GameSession::on_explosion_triggered(uint32_t session_id, const std::vector<uint8_t>& explosion_data)
{
    if (listener_)
        listener_->on_explosion(session_id, explosion_data);
}

void GameSession::on_score_updated(uint32_t session_id, const std::vector<uint8_t>& score_data)
{
    if (listener_)
        listener_->on_score_update(session_id, score_data);
}

void GameSession::on_powerup_collected(uint32_t session_id, const std::vector<uint8_t>& powerup_data)
{
    if (listener_)
        listener_->on_powerup_collected(session_id, powerup_data);
}

void GameSession::on_player_respawn(uint32_t session_id, const std::vector<uint8_t>& respawn_data)
{
    if (listener_)
        listener_->on_player_respawn(session_id, respawn_data);
}

void GameSession::on_player_level_up(uint32_t session_id, const std::vector<uint8_t>& level_up_data)
{
    if (listener_)
        listener_->on_player_level_up(session_id, level_up_data);
}

void GameSession::check_game_over()
{
    // Check PlayerLives components, not just is_alive flag
    if (!registry_.has_component_registered<game::PlayerLives>()) {
        return;  // No lives system, checkpoint system will handle game over
    }

    auto& player_lives_components = registry_.get_components<game::PlayerLives>();

    bool any_lives_remaining = false;
    for (size_t i = 0; i < player_lives_components.size(); ++i) {
        const game::PlayerLives& pl = player_lives_components.get_data_at(i);
        if (pl.lives_remaining > 0 || pl.respawn_pending) {
            any_lives_remaining = true;
            break;
        }
    }

    // Only trigger game over if NO players have lives left AND none are pending respawn
    // (CheckpointSystem will handle checkpoint respawn with 3 lives before this happens)
    if (!any_lives_remaining && player_lives_components.size() > 0) {
        // This should rarely trigger since CheckpointSystem respawns with 3 lives
        // But kept as safeguard
        is_active_.store(false, std::memory_order_release);
        // std::cout << "[GameSession " << session_id_ << "] Game over - all players out of lives!\n";
        if (listener_)
            listener_->on_game_over(session_id_, get_player_ids(), false);  // Defeat
    }
}

void GameSession::check_offscreen_enemies()
{
    auto& positions = registry_.get_components<Position>();
    auto& enemies = registry_.get_components<Enemy>();

    std::vector<Entity> entities_to_kill;
    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        Position& pos = positions.get_data_at(i);

        // Skip players
        bool is_player = false;
        for (const auto& [pid, player] : players_) {
            if (player.entity == entity) {
                is_player = true;
                break;
            }
        }
        if (is_player) continue;

        // Skip boss enemies (they should never be destroyed by going offscreen)
        // A boss is identified by having a BossPhase component
        auto& boss_phases = registry_.get_components<game::BossPhase>();
        if (boss_phases.has_entity(entity)) {
            continue;  // Don't destroy boss
        }

        // Destroy regular offscreen entities
        if (pos.x < config::ENTITY_OFFSCREEN_LEFT)
            entities_to_kill.push_back(entity);
    }
    for (Entity entity : entities_to_kill) {
        registry_.kill_entity(entity);
        if (network_system_)
            network_system_->queue_entity_destroy(entity);
    }
}

void GameSession::resync_client(uint32_t player_id, uint32_t tcp_client_id)
{
    if (!network_system_) {
        std::cerr << "[GameSession " << session_id_ << "] Cannot resync: no network system\n";
        return;
    }
    // std::cout << "[GameSession " << session_id_ << "] Resyncing client " << tcp_client_id
    //           << " (player " << player_id << ") with existing entities\n";
    auto& positions = registry_.get_components<Position>();
    auto& healths = registry_.get_components<Health>();
    auto& player_levels = registry_.get_components<game::PlayerLevel>();
    int entity_count = 0;
    for (const auto& [pid, player] : players_) {
        if (positions.has_entity(player.entity)) {
            const Position& pos = positions[player.entity];
            uint16_t health = 100;
            if (healths.has_entity(player.entity))
                health = healths[player.entity].current;
            // Calculate actual skin_id from PlayerLevel component (level + color)
            uint8_t actual_skin_id = player.skin_id;  // fallback to stored value
            if (player_levels.has_entity(player.entity)) {
                const auto& pl = player_levels[player.entity];
                actual_skin_id = game::compute_skin_id(pl.current_level, pl.color_id);
            }
            // Encode both player_id and actual_skin_id in subtype: high 4 bits = player_id, low 4 bits = skin_id
            uint8_t subtype = static_cast<uint8_t>(((pid & 0x0F) << 4) | (actual_skin_id & 0x0F));
            network_system_->queue_entity_spawn(
                player.entity, EntityType::PLAYER,
                pos.x, pos.y, health, subtype
            );
            entity_count++;
        }
    }
    auto& enemies = registry_.get_components<Enemy>();
    for (size_t i = 0; i < enemies.size(); ++i) {
        Entity entity = enemies.get_entity_at(i);
        if (positions.has_entity(entity)) {
            const Position& pos = positions[entity];
            uint16_t health = 100;
            if (healths.has_entity(entity))
                health = healths[entity].current;
            network_system_->queue_entity_spawn(
                entity, EntityType::ENEMY_BASIC,
                pos.x, pos.y, health, 0
            );
            entity_count++;
        }
    }
    auto& projectiles = registry_.get_components<Projectile>();
    for (size_t i = 0; i < projectiles.size(); ++i) {
        Entity entity = projectiles.get_entity_at(i);
        if (positions.has_entity(entity)) {
            const Position& pos = positions[entity];
            const Projectile& proj = projectiles.get_data_at(i);
            network_system_->queue_entity_spawn(
                entity,
                proj.faction == ProjectileFaction::Player ?
                    EntityType::PROJECTILE_PLAYER : EntityType::PROJECTILE_ENEMY,
                pos.x, pos.y, 0, 0
            );
            entity_count++;
        }
    }
    if (listener_ && has_wave_started_)
        listener_->on_wave_start(session_id_, serialize(last_wave_start_payload_));
    if (listener_ && has_wave_complete_)
        listener_->on_wave_complete(session_id_, serialize(last_wave_complete_payload_));
    // std::cout << "[GameSession " << session_id_ << "] Queued " << entity_count << " entity spawns for resync\n";
}

void GameSession::load_map_segments(uint16_t map_id)
{
    // Map ID to folder mapping
    // The gameplay logic (waves, boss) is defined in level JSON files
    std::string map_folder;
    switch (map_id) {
        case 0:   // Debug: Quick Test
        case 99:  // Debug: Instant Boss
            map_folder = "nebula_outpost";
            break;
        case 1:   // Level 1: Mars Assault
            map_folder = "mars_outpost";
            break;
        case 2:   // Level 2: Nebula Station
            map_folder = "nebula_outpost";
            break;
        case 3:   // Level 3: Uranus Station
            map_folder = "urasnus_outpost";
            break;
        case 4:   // Level 4: Jupiter Orbit
            map_folder = "jupiter_outpost";
            break;
        default:
            map_folder = "nebula_outpost";
            break;
    }

    try {
        map_config_ = rtype::MapConfigLoader::loadMapById(map_folder);
        scroll_speed_ = (map_config_.baseScrollSpeed > 0.0f)
            ? map_config_.baseScrollSpeed
            : config::GAME_SCROLL_SPEED;
        tile_size_ = map_config_.tileSize;

        std::string segments_dir = map_config_.basePath + "/segments";
        auto segment_paths = rtype::MapConfigLoader::getSegmentPaths(segments_dir);

        for (const auto& path : segment_paths) {
            auto segment = rtype::MapConfigLoader::loadSegment(path);
            map_segments_.push_back(segment);
        }

        // std::cout << "[GameSession " << session_id_ << "] Loaded " << map_segments_.size()
        //           << " map segments for tile walls (tileSize=" << tile_size_ << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "[GameSession " << session_id_ << "] Failed to load map segments: " << e.what() << "\n";
    }
}

void GameSession::spawn_walls_in_view()
{
    if (map_segments_.empty() || next_segment_to_spawn_ >= map_segments_.size())
        return;

    // Calculate world X position where next segment starts
    double segment_world_x = 0.0;
    for (size_t i = 0; i < next_segment_to_spawn_; ++i) {
        segment_world_x += static_cast<double>(map_segments_[i].width * tile_size_);
    }

    // Spawn walls from segments that are about to come into view
    // Use double for precision
    double spawn_threshold = current_scroll_ + 1920.0 + 500.0;  // screen width + buffer

    while (next_segment_to_spawn_ < map_segments_.size() && segment_world_x < spawn_threshold) {
        const auto& segment = map_segments_[next_segment_to_spawn_];
        double segment_width = static_cast<double>(segment.width * tile_size_);

        // Greedy merge: find rectangular groups of wall tiles
        int height = static_cast<int>(segment.tiles.size());
        int width = (height > 0) ? static_cast<int>(segment.tiles[0].size()) : 0;
        std::vector<std::vector<bool>> processed(height, std::vector<bool>(width, false));

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (processed[y][x] || segment.tiles[y][x] == 0)
                    continue;

                // Greedy merge: find largest rectangle of wall tiles
                int w = 1, h = 1;

                // Expand width
                while (x + w < width && segment.tiles[y][x + w] != 0 && !processed[y][x + w])
                    w++;

                // Expand height if all tiles in row match
                while (y + h < height) {
                    bool row_ok = true;
                    for (int dx = 0; dx < w; ++dx) {
                        if (segment.tiles[y + h][x + dx] == 0 || processed[y + h][x + dx]) {
                            row_ok = false;
                            break;
                        }
                    }
                    if (!row_ok)
                        break;
                    h++;
                }

                // Mark tiles as processed
                for (int dy = 0; dy < h; ++dy) {
                    for (int dx = 0; dx < w; ++dx) {
                        processed[y + dy][x + dx] = true;
                    }
                }

                // Spawn merged wall entity with FIXED world position
                // Walls are STATIC in world coordinates - they don't move!
                // The scroll is applied during collision detection by converting
                // player screen position to world position
                double tile_world_x = segment_world_x + static_cast<double>(x * tile_size_);
                float wall_width = static_cast<float>(w * tile_size_);
                float wall_height = static_cast<float>(h * tile_size_);
                float center_x = static_cast<float>(tile_world_x) + wall_width * 0.5f;
                float center_y = static_cast<float>(y * tile_size_) + wall_height * 0.5f;

                Entity wall = registry_.spawn_entity();
                registry_.add_component(wall, Position{center_x, center_y});
                // NO VELOCITY - walls are static in world coordinates!
                // This eliminates floating point drift that caused desync
                registry_.add_component(wall, Collider{wall_width, wall_height});
                registry_.add_component(wall, Wall{});
                registry_.add_component(wall, NoFriction{});
                registry_.add_component(wall, Health{65535, 65535});

                // DO NOT send walls to client - client loads walls from tilemap via ChunkManager
                // Server uses these walls for server-side collision validation only
            }
        }

        segment_world_x += segment_width;
        next_segment_to_spawn_++;
    }

    // Despawn walls that are now behind the camera (off-screen to the left)
    // This prevents memory from growing indefinitely as we scroll through the level
    despawn_walls_behind_camera();
}

void GameSession::despawn_walls_behind_camera()
{
    auto& walls = registry_.get_components<Wall>();
    auto& positions = registry_.get_components<Position>();
    auto& colliders = registry_.get_components<Collider>();

    // Despawn walls that are completely off-screen to the left
    // Buffer of 100 pixels to be safe
    float despawn_threshold = static_cast<float>(current_scroll_) - 100.0f;

    std::vector<Entity> walls_to_despawn;
    for (size_t i = 0; i < walls.size(); ++i) {
        Entity wall_entity = walls.get_entity_at(i);
        if (!positions.has_entity(wall_entity) || !colliders.has_entity(wall_entity))
            continue;

        const Position& pos = positions[wall_entity];
        const Collider& col = colliders[wall_entity];

        // Wall right edge position
        float wall_right = pos.x + col.width * 0.5f;

        if (wall_right < despawn_threshold) {
            walls_to_despawn.push_back(wall_entity);
        }
    }

    for (Entity wall : walls_to_despawn) {
        registry_.kill_entity(wall);
    }
}

// Wall collision is now handled by CollisionSystem with scroll-aware detection
// See CollisionSystem::update() which uses m_currentScroll set by GameSession


Entity GameSession::respawn_player_at(uint32_t player_id, float x, float y, float invuln_duration, uint8_t lives)
{
    // Find player data to get skin_id
    auto it = players_.find(player_id);
    if (it == players_.end()) {
        std::cerr << "[GameSession] Cannot respawn unknown player " << player_id << "\n";
        return engine::INVALID_HANDLE;
    }

    // Create new entity
    Entity entity = registry_.spawn_entity();

    // Update player tracking
    it->second.entity = entity;
    it->second.is_alive = true;
    it->second.lives = lives; // Sync lives from CheckpointSystem
    player_entities_[player_id] = entity;

    // Basic components
    registry_.add_component(entity, Position{x, y});
    registry_.add_component(entity, Velocity{0.0f, 0.0f});
    // Use rtype::shared::config for player stats
    registry_.add_component(entity, Health{static_cast<int>(rtype::shared::config::PLAYER_MAX_HEALTH), static_cast<int>(rtype::shared::config::PLAYER_MAX_HEALTH)});
    registry_.add_component(entity, Controllable{rtype::shared::config::PLAYER_MOVEMENT_SPEED});
    registry_.add_component(entity, Collider{rtype::shared::config::PLAYER_WIDTH, rtype::shared::config::PLAYER_HEIGHT});
    registry_.add_component(entity, Invulnerability{invuln_duration});

    // Score handling (reset score on respawn)
    registry_.add_component(entity, Score{0});

    // Extract color from player's skin_id (which is now just color 0-2)
    uint8_t color_id = (it->second.skin_id < 3) ? it->second.skin_id : game::get_color_from_skin_id(it->second.skin_id);

    // Create PlayerLevel component - respawn at level 1
    game::PlayerLevel player_level;
    player_level.current_level = 1;
    player_level.color_id = color_id;
    player_level.level_up_pending = false;
    player_level.level_up_timer = 0.0f;
    registry_.add_component(entity, player_level);

    // Calculate actual skin_id for level 1
    uint8_t actual_skin_id = game::compute_skin_id(1, color_id);

    // Reset to basic weapon (level 1 weapon)
    if (registry_.has_component_registered<Weapon>()) {
        Weapon weapon = create_weapon(WeaponType::BASIC, engine::INVALID_HANDLE);
        registry_.add_component(entity, weapon);
    }

    // Network spawn
    if (network_system_) {
        // Encode player_id and actual_skin_id in subtype
        uint8_t subtype = static_cast<uint8_t>(((player_id & 0x0F) << 4) | (actual_skin_id & 0x0F));

        network_system_->queue_entity_spawn(
            entity,
            EntityType::PLAYER,
            x,
            y,
            rtype::shared::config::PLAYER_MAX_HEALTH,
            subtype
        );

        // Notify client of respawn
        network_system_->queue_player_respawn(player_id, x, y, invuln_duration, it->second.lives);
    }

    return entity;
}

void GameSession::pause()
{
    is_paused_ = true;
    std::cout << "[GameSession " << session_id_ << "] Paused\n";
}

void GameSession::resume()
{
    is_paused_ = false;
    std::cout << "[GameSession " << session_id_ << "] Resumed\n";
}

void GameSession::clear_enemies()
{
    auto& enemy_components = registry_.get_components<Enemy>();
    size_t count = enemy_components.size();

    for (size_t i = 0; i < count; ++i) {
        Entity enemy_entity = enemy_components.get_entity_at(i);
        registry_.add_component(enemy_entity, ToDestroy{});
    }
    std::cout << "[GameSession " << session_id_ << "] Cleared " << count << " enemies\n";
}

} // namespace rtype::server
