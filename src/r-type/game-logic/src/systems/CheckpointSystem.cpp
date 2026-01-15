/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CheckpointSystem implementation
*/

#include "systems/CheckpointSystem.hpp"
#include "GameConfig.hpp"
#include <iostream>

namespace game {

CheckpointSystem::CheckpointSystem()
{
}

void CheckpointSystem::init(Registry& registry)
{
}

void CheckpointSystem::shutdown()
{
}

void CheckpointSystem::set_spawn_player_callback(std::function<Entity(uint32_t, float, float, float, uint8_t)> callback)
{
    on_spawn_player_callback_ = callback;
}

void CheckpointSystem::set_network_callback(std::function<void(uint32_t, float, float, float, uint8_t)> callback)
{
    on_broadcast_respawn_callback_ = callback;
}

void CheckpointSystem::set_game_over_callback(std::function<void()> callback)
{
    on_game_over_callback_ = callback;
}

void CheckpointSystem::update(Registry& registry, float dt)
{
    process_respawn_timers(registry, dt);
}

void CheckpointSystem::on_player_death(Registry& registry, Entity player_entity,
                                        uint32_t player_id)
{
    PlayerLives* player_lives = find_player_lives(registry, player_id);
    if (!player_lives) {
        return;
    }

    player_lives->lives_remaining--;

    if (player_lives->lives_remaining > 0) {
        player_lives->respawn_pending = true;
        player_lives->respawn_timer = 3.0f;
    } else {
        check_all_players_dead(registry);
    }
}

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
    float spawn_x = 300.0f;
    float spawn_y = 540.0f + ((player_id % 4) * 80.0f) - 120.0f;

    Entity new_entity = engine::INVALID_HANDLE;
    if (on_spawn_player_callback_) {
        new_entity = on_spawn_player_callback_(player_id, spawn_x, spawn_y, 3.0f, player_lives.lives_remaining);
    } else {
        std::cerr << "[CheckpointSystem] ERROR: Spawn callback not set! Cannot respawn player " << player_id << "\n";
        return;
    }

    player_lives.respawn_pending = false;
}

void CheckpointSystem::check_all_players_dead(Registry& registry)
{
    if (!registry.has_component_registered<PlayerLives>()) {
        return;
    }

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
        if (on_game_over_callback_) {
            on_game_over_callback_();
        }
    }
}

PlayerLives* CheckpointSystem::find_player_lives(Registry& registry, uint32_t player_id)
{
    if (!registry.has_component_registered<PlayerLives>()) {
        return nullptr;
    }

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
