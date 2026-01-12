/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CheckpointSystem implementation
*/

#include "systems/CheckpointSystem.hpp"
#include <iostream>

namespace game {

CheckpointSystem::CheckpointSystem()
{
}

void CheckpointSystem::init(Registry& registry)
{
    // Nothing to initialize
}

void CheckpointSystem::shutdown()
{
    // Nothing to cleanup
}

void CheckpointSystem::update(Registry& registry, float dt)
{
    auto& checkpoint_managers = registry.get_components<CheckpointManager>();
    if (checkpoint_managers.size() == 0) {
        return;  // No checkpoint manager, nothing to do
    }

    CheckpointManager& cm = checkpoint_managers.get_data_at(0);
    float current_scroll = get_current_scroll(registry);

    // Check if any new checkpoint should activate
    for (size_t i = cm.active_checkpoint_index + 1; i < cm.checkpoints.size(); ++i) {
        Checkpoint& cp = cm.checkpoints[i];
        if (current_scroll >= cp.scroll_distance && !cp.activated) {
            activate_checkpoint(registry, cm, i);
            cm.active_checkpoint_index = i;
        }
    }

    // Process pending respawns
    process_respawn_timers(registry, dt);
}

void CheckpointSystem::on_player_death(Registry& registry, Entity player_entity,
                                        uint32_t player_id)
{
    PlayerLives* player_lives = find_player_lives(registry, player_id);
    if (!player_lives) {
        std::cerr << "[CheckpointSystem] No PlayerLives found for player " << player_id << "\n";
        return;
    }

    player_lives->lives_remaining--;

    std::cout << "[CheckpointSystem] Player " << player_id << " died. Lives remaining: "
              << static_cast<int>(player_lives->lives_remaining) << "\n";

    if (player_lives->lives_remaining > 0) {
        // Respawn at checkpoint
        auto& checkpoint_managers = registry.get_components<CheckpointManager>();
        if (checkpoint_managers.size() == 0) {
            std::cerr << "[CheckpointSystem] No checkpoint manager found!\n";
            return;
        }

        CheckpointManager& cm = checkpoint_managers.get_data_at(0);

        player_lives->respawn_pending = true;
        player_lives->respawn_timer = 3.0f;  // 3 second delay
        player_lives->checkpoint_index = cm.active_checkpoint_index;

        std::cout << "[CheckpointSystem] Player " << player_id << " will respawn in 3 seconds at checkpoint "
                  << cm.checkpoints[cm.active_checkpoint_index].checkpoint_id << "\n";

        // TODO: Notify network system to show death screen
    } else {
        // Game over for this player
        std::cout << "[CheckpointSystem] Player " << player_id << " has no lives remaining\n";

        // TODO: Notify network system that player is eliminated

        // Check if ALL players are out of lives
        check_all_players_dead(registry);
    }
}

// ============================================================================
// CHECKPOINT MANAGEMENT
// ============================================================================

void CheckpointSystem::activate_checkpoint(Registry& registry, CheckpointManager& cm, uint32_t index)
{
    if (index >= cm.checkpoints.size()) return;

    Checkpoint& cp = cm.checkpoints[index];
    cp.activated = true;

    std::cout << "[CheckpointSystem] Checkpoint activated: " << cp.checkpoint_id
              << " at scroll distance " << cp.scroll_distance << "\n";

    // TODO: Notify network system to show checkpoint activation message
}

// ============================================================================
// RESPAWN MANAGEMENT
// ============================================================================

void CheckpointSystem::process_respawn_timers(Registry& registry, float dt)
{
    auto& player_lives_components = registry.get_components<PlayerLives>();

    for (size_t i = 0; i < player_lives_components.size(); ++i) {
        PlayerLives& player_lives = player_lives_components.get_data_at(i);

        if (player_lives.respawn_pending) {
            player_lives.respawn_timer -= dt;

            if (player_lives.respawn_timer <= 0.0f) {
                // Respawn player
                respawn_player(registry, player_lives.player_id, player_lives);
            }
        }
    }
}

void CheckpointSystem::respawn_player(Registry& registry, uint32_t player_id, PlayerLives& player_lives)
{
    auto& checkpoint_managers = registry.get_components<CheckpointManager>();
    if (checkpoint_managers.size() == 0) {
        std::cerr << "[CheckpointSystem] No checkpoint manager found for respawn!\n";
        return;
    }

    CheckpointManager& cm = checkpoint_managers.get_data_at(0);
    if (player_lives.checkpoint_index >= cm.checkpoints.size()) {
        std::cerr << "[CheckpointSystem] Invalid checkpoint index!\n";
        return;
    }

    const Checkpoint& cp = cm.checkpoints[player_lives.checkpoint_index];

    // Create new player entity at checkpoint
    Entity new_entity = spawn_player_at_checkpoint(registry, player_id, cp);

    // CRITICAL: Remove power-ups (punishing respawn like R-Type)
    // Player spawns with only BASIC weapon, no shield, no speed boost
    auto& weapons = registry.get_components<Weapon>();
    if (weapons.has_entity(new_entity)) {
        weapons[new_entity].type = WeaponType::BASIC;
    }

    // Remove shield if present
    auto& shields = registry.get_components<Shield>();
    if (shields.has_entity(new_entity)) {
        registry.remove_component<Shield>(new_entity);
    }

    // Remove speed boost if present
    auto& speed_boosts = registry.get_components<SpeedBoost>();
    if (speed_boosts.has_entity(new_entity)) {
        registry.remove_component<SpeedBoost>(new_entity);
    }

    // Add invulnerability for 3 seconds
    Invulnerability invuln;
    invuln.time_remaining = 3.0f;
    registry.add_component(new_entity, invuln);

    // Reset respawn flag
    player_lives.respawn_pending = false;

    std::cout << "[CheckpointSystem] Player " << player_id << " respawned at checkpoint "
              << cp.checkpoint_id << " with " << static_cast<int>(player_lives.lives_remaining)
              << " lives remaining\n";

    // TODO: Notify network system to broadcast SERVER_PLAYER_RESPAWN packet
}

Entity CheckpointSystem::spawn_player_at_checkpoint(Registry& registry, uint32_t player_id,
                                                              const Checkpoint& checkpoint)
{
    Entity player = registry.spawn_entity();

    // Position at checkpoint
    registry.add_component(player, Position{checkpoint.respawn_x, checkpoint.respawn_y});

    // Velocity
    registry.add_component(player, Velocity{0.0f, 0.0f});

    // LocalPlayer tag
    registry.add_component(player, LocalPlayer{});

    // Controllable
    Controllable controllable;
    controllable.speed = 300.0f;
    registry.add_component(player, controllable);

    // Health (full HP)
    Health health;
    health.max = 100;
    health.current = 100;
    registry.add_component(player, health);

    // Weapon (BASIC only - power-ups removed)
    Weapon weapon;
    weapon.type = WeaponType::BASIC;
    registry.add_component(player, weapon);

    // Collider
    registry.add_component(player, Collider{40.0f, 30.0f});

    // Sprite (TODO: load proper player sprite)
    Sprite sprite;
    sprite.width = 40.0f;
    sprite.height = 30.0f;
    sprite.layer = 5;
    registry.add_component(player, sprite);

    // Score
    Score score;
    score.value = 0;
    registry.add_component(player, score);

    return player;
}

void CheckpointSystem::check_all_players_dead(Registry& registry)
{
    auto& player_lives_components = registry.get_components<PlayerLives>();

    bool all_dead = true;
    for (size_t i = 0; i < player_lives_components.size(); ++i) {
        const PlayerLives& player_lives = player_lives_components.get_data_at(i);
        if (player_lives.lives_remaining > 0) {
            all_dead = false;
            break;
        }
    }

    if (all_dead && player_lives_components.size() > 0) {
        std::cout << "[CheckpointSystem] All players are out of lives - GAME OVER!\n";

        // Set level controller to GAME_OVER state
        auto& level_controllers = registry.get_components<LevelController>();
        if (level_controllers.size() > 0) {
            LevelController& lc = level_controllers.get_data_at(0);
            lc.state = LevelState::GAME_OVER;
        }

        // TODO: Notify server to send GAME_OVER packet with DEFEAT result
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

PlayerLives* CheckpointSystem::find_player_lives(Registry& registry, uint32_t player_id)
{
    auto& player_lives_components = registry.get_components<PlayerLives>();

    for (size_t i = 0; i < player_lives_components.size(); ++i) {
        PlayerLives& player_lives = player_lives_components.get_data_at(i);
        if (player_lives.player_id == player_id) {
            return &player_lives;
        }
    }

    return nullptr;
}

float CheckpointSystem::get_current_scroll(Registry& registry)
{
    auto& scroll_states = registry.get_components<ScrollState>();
    if (scroll_states.size() > 0) {
        return scroll_states.get_data_at(0).current_scroll;
    }
    return 0.0f;
}

} // namespace game
