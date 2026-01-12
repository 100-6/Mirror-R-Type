/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CheckpointSystem - Manages checkpoint activation and player respawning
*/

#ifndef CHECKPOINT_SYSTEM_HPP_
#define CHECKPOINT_SYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/LevelComponents.hpp"
#include "components/GameComponents.hpp"

namespace game {

/**
 * @brief System that manages checkpoint activation and player respawning
 *
 * The CheckpointSystem is responsible for:
 * - Activating checkpoints when scroll distance is reached
 * - Listening to EntityDeathEvent for player deaths
 * - Managing PlayerLives component (decrement lives, start respawn timer)
 * - Respawning players at active checkpoint after timer expires
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

private:
    // === Checkpoint Management ===

    /**
     * @brief Activate checkpoint at given index
     */
    void activate_checkpoint(Registry& registry, CheckpointManager& cm, uint32_t index);

    // === Respawn Management ===

    /**
     * @brief Process respawn timers for all players
     */
    void process_respawn_timers(Registry& registry, float dt);

    /**
     * @brief Respawn player at active checkpoint
     */
    void respawn_player(Registry& registry, uint32_t player_id, PlayerLives& player_lives);

    /**
     * @brief Spawn player entity at checkpoint position
     * @return New player entity
     */
    Entity spawn_player_at_checkpoint(Registry& registry, uint32_t player_id,
                                               const Checkpoint& checkpoint);

    /**
     * @brief Check if all players are out of lives
     */
    void check_all_players_dead(Registry& registry);

    // === Helper Functions ===

    /**
     * @brief Find PlayerLives component for given player ID
     */
    PlayerLives* find_player_lives(Registry& registry, uint32_t player_id);

    /**
     * @brief Get current scroll position from game session
     * Note: This is a placeholder - will be properly implemented when integrated with GameSession
     */
    float get_current_scroll(Registry& registry);
};

} // namespace game

#endif // CHECKPOINT_SYSTEM_HPP_
