/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** IGameSessionListener - Interface for game session events
*/

#pragma once

#include <cstdint>
#include <vector>

namespace rtype::server {

/**
 * @brief Interface for receiving game session events
 *
 * Implement this interface to receive notifications when:
 * - State snapshot is ready to send
 * - Entity spawns/dies
 * - Projectile spawns
 * - Wave starts/completes
 * - Game ends
 */
class IGameSessionListener {
public:
    virtual ~IGameSessionListener() = default;

    /**
     * @brief Called when a state snapshot is ready (20 Hz)
     * @param session_id The game session
     * @param snapshot Serialized snapshot data
     */
    virtual void on_state_snapshot(uint32_t session_id, const std::vector<uint8_t>& snapshot) = 0;

    /**
     * @brief Called when an entity spawns (player, enemy, wall, bonus)
     * @param session_id The game session
     * @param spawn_data Serialized spawn data
     */
    virtual void on_entity_spawn(uint32_t session_id, const std::vector<uint8_t>& spawn_data) = 0;

    /**
     * @brief Called when an entity is destroyed
     * @param session_id The game session
     * @param entity_id The destroyed entity
     */
    virtual void on_entity_destroy(uint32_t session_id, uint32_t entity_id) = 0;

    /**
     * @brief Called when a projectile spawns
     * @param session_id The game session
     * @param projectile_data Serialized projectile data
     */
    virtual void on_projectile_spawn(uint32_t session_id, const std::vector<uint8_t>& projectile_data) = 0;

    /**
     * @brief Called when an explosion effect must be broadcast
     * @param session_id The game session
     * @param explosion_data Serialized explosion payload
     */
    virtual void on_explosion(uint32_t session_id, const std::vector<uint8_t>& explosion_data) = 0;

    /**
     * @brief Called when a wave starts
     * @param session_id The game session
     * @param wave_data Serialized wave data
     */
    virtual void on_wave_start(uint32_t session_id, const std::vector<uint8_t>& wave_data) = 0;

    /**
     * @brief Called when a wave is completed
     * @param session_id The game session
     * @param wave_data Serialized wave completion data
     */
    virtual void on_wave_complete(uint32_t session_id, const std::vector<uint8_t>& wave_data) = 0;

    /**
     * @brief Called when game ends (victory or defeat)
     * @param session_id The game session
     * @param player_ids Players in the session
     * @param is_victory True if players won, false if all players died
     */
    virtual void on_game_over(uint32_t session_id, const std::vector<uint32_t>& player_ids, bool is_victory) = 0;

    /**
     * @brief Called when score is updated
     * @param session_id The game session
     * @param score_data Serialized score data
     */
    virtual void on_score_update(uint32_t session_id, const std::vector<uint8_t>& score_data) = 0;

    /**
     * @brief Called when a powerup is collected
     * @param session_id The game session
     * @param powerup_data Serialized powerup data
     */
    virtual void on_powerup_collected(uint32_t session_id, const std::vector<uint8_t>& powerup_data) = 0;

    /**
     * @brief Called when a player respawns
     * @param session_id The game session
     * @param respawn_data Serialized respawn data
     */
    virtual void on_player_respawn(uint32_t session_id, const std::vector<uint8_t>& respawn_data) = 0;

    /**
     * @brief Called when a player levels up
     * @param session_id The game session
     * @param level_up_data Serialized level-up data
     */
    virtual void on_player_level_up(uint32_t session_id, const std::vector<uint8_t>& level_up_data) = 0;

    /**
     /**
      * @brief Called when a level transition occurs
      * @param session_id Session identifier
      * @param transition_data Serialized transition data
      */
    virtual void on_level_transition(uint32_t session_id, const std::vector<uint8_t>& transition_data) = 0;

    /**
     * @brief Called when leaderboard should be sent (before game over)
     * @param session_id The game session
     * @param leaderboard_data Serialized leaderboard data
     */
    virtual void on_leaderboard(uint32_t session_id, const std::vector<uint8_t>& leaderboard_data) = 0;
};

} // namespace rtype::server
