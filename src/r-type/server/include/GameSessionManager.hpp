/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameSessionManager - Manages the lifecycle of game sessions
*/

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "GameSession.hpp"
#include "interfaces/IGameSessionListener.hpp"
#include "protocol/PacketTypes.hpp"

namespace rtype::server {

/**
 * @brief Manages the lifecycle of game sessions
 *
 * This class handles:
 * - Creating and destroying game sessions
 * - Updating active sessions
 * - Forwarding session events to listener
 */
class GameSessionManager {
public:
    GameSessionManager();

    /**
     * @brief Set the listener for all session events
     */
    void set_listener(IGameSessionListener* listener) { listener_ = listener; }

    /**
     * @brief Create a new game session
     * @param session_id Unique session identifier
     * @param game_mode Game mode
     * @param difficulty Difficulty level
     * @param level_seed Random seed for level generation
     * @return Pointer to the created session
     */
    GameSession* create_session(uint32_t session_id, protocol::GameMode game_mode,
                               protocol::Difficulty difficulty, uint32_t level_seed);

    /**
     * @brief Get a game session by ID
     * @param session_id Session identifier
     * @return Pointer to the session, or nullptr if not found
     */
    GameSession* get_session(uint32_t session_id);

    /**
     * @brief Update all active game sessions
     * @param delta_time Time elapsed since last update (in seconds)
     */
    void update_all(float delta_time);

    /**
     * @brief Remove inactive sessions
     */
    void cleanup_inactive_sessions();

    /**
     * @brief Remove a specific session
     * @param session_id Session identifier
     */
    void remove_session(uint32_t session_id);

    /**
     * @brief Get all active session IDs
     */
    std::vector<uint32_t> get_active_session_ids() const;

private:
    std::unordered_map<uint32_t, std::unique_ptr<GameSession>> sessions_;
    IGameSessionListener* listener_ = nullptr;
};

}
