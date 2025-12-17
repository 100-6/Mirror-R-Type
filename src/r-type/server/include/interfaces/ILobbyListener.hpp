/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ILobbyListener - Interface for lobby events
*/

#pragma once

#include <cstdint>
#include <vector>

namespace rtype::server {

/**
 * @brief Interface for receiving lobby events
 *
 * Implement this interface to receive notifications when:
 * - Lobby state changes (player joins/leaves)
 * - Countdown ticks
 * - Game is ready to start
 */
class ILobbyListener {
public:
    virtual ~ILobbyListener() = default;

    /**
     * @brief Called when lobby state changes (player joins/leaves)
     * @param lobby_id The lobby that changed
     * @param payload Serialized lobby state data
     */
    virtual void on_lobby_state_changed(uint32_t lobby_id, const std::vector<uint8_t>& payload) = 0;

    /**
     * @brief Called every second during countdown
     * @param lobby_id The lobby in countdown
     * @param seconds_remaining Seconds left before game starts
     */
    virtual void on_countdown_tick(uint32_t lobby_id, uint8_t seconds_remaining) = 0;

    /**
     * @brief Called when countdown reaches zero and game should start
     * @param lobby_id The lobby that's starting
     * @param player_ids List of players in the game
     */
    virtual void on_game_start(uint32_t lobby_id, const std::vector<uint32_t>& player_ids) = 0;
};

}
