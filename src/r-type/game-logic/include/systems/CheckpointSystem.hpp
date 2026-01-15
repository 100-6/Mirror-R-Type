/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CheckpointSystem - Manages checkpoint activation and player respawning
*/

#ifndef CHECKPOINT_SYSTEM_HPP_
#define CHECKPOINT_SYSTEM_HPP_

#include <functional>
#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/LevelComponents.hpp"
#include "components/GameComponents.hpp"

namespace game {

/**
 * @brief System that manages checkpoint activation and player respawning
 *
 * The CheckpointSystem is responsible for:
 * - Listening to EntityDeathEvent for player deaths
 * - Managing PlayerLives component (decrement lives, start respawn timer)
 * - Respawning players at dynamic position near current scroll
 * - Removing power-ups on respawn (BASIC weapon only - punishing mechanic)
 * - Checking if all players are out of lives → GAME_OVER
 *
 * This system works on server-side only (authoritative).
 */
class CheckpointSystem : public ISystem {
public:
    CheckpointSystem();
    virtual ~CheckpointSystem() = default;

    /**
     * @brief Initialize the system
     * @param registry ECS registry
     */
    void init(Registry& registry) override;

    /**
     * @brief Shutdown the system
     */
    void shutdown() override;

    /**
     * @brief Update checkpoint activation and respawn timers
     * @param registry ECS registry
     * @param dt Delta time in seconds
     */
    void update(Registry& registry, float dt) override;

    // === Event Handlers ===

    /**
     * @brief Called when a player dies
     * @param registry ECS registry
     * @param player_entity Player entity that died
     * @param player_id Player ID
     */
    void on_player_death(Registry& registry, Entity player_entity, uint32_t player_id);

    // === Callback Setters ===

    /**
     * @brief Set callback to request player spawning from GameSession
     * @param callback Function called with (player_id, x, y, invuln_duration, lives) that returns new Entity
     */
    void set_spawn_player_callback(std::function<Entity(uint32_t, float, float, float, uint8_t)> callback);

    /**
     * @brief Set callback for broadcasting respawn packets to clients
     * @param callback Function called with (player_id, x, y, invuln_duration, lives) to send network packet
     */
    void set_network_callback(std::function<void(uint32_t player_id, float x, float y, float invuln_duration, uint8_t lives)> callback);

    /**
     * @brief Set callback for game over (when all players out of lives)
     * @param callback Function called to trigger game over
     */
    void set_game_over_callback(std::function<void()> callback);

private:
    void process_respawn_timers(Registry& registry, float dt);
    void respawn_player(Registry& registry, uint32_t player_id, PlayerLives& player_lives);
    void check_all_players_dead(Registry& registry);
    PlayerLives* find_player_lives(Registry& registry, uint32_t player_id);
    float get_current_scroll(Registry& registry);

    std::function<Entity(uint32_t, float, float, float, uint8_t)> on_spawn_player_callback_;
    std::function<void(uint32_t, float, float, float, uint8_t)> on_broadcast_respawn_callback_;
    std::function<void()> on_game_over_callback_;
};

} // namespace game

#endif // CHECKPOINT_SYSTEM_HPP_
