/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RoomManager - Manages custom game rooms
*/

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>

#include "LobbyManager.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"
#include "interfaces/ILobbyListener.hpp"

namespace rtype::server {

/**
 * @brief Manages custom game rooms
 *
 * Handles:
 * - Creating/destroying custom rooms
 * - Joining/leaving rooms with password verification
 * - Listing public rooms
 * - Host transfer when host leaves
 * - Automatic cleanup of empty rooms
 * - Countdown when room is full
 */
class RoomManager {
public:
    RoomManager();
    ~RoomManager() = default;

    /**
     * @brief Set the listener for room events
     * @param listener Pointer to listener (must outlive RoomManager)
     */
    void set_listener(ILobbyListener* listener) { listener_ = listener; }

    /**
     * @brief Create a new custom room
     * @param host_player_id The player creating the room (becomes host)
     * @param room_name Optional room name (empty = "Room #ID")
     * @param password_hash SHA256 hash of password (empty = public)
     * @param game_mode DUO, TRIO, or SQUAD
     * @param difficulty EASY, NORMAL, or HARD
     * @param map_id Map identifier
     * @param max_players Maximum players (2-4, 0 = use default for mode)
     * @return Room ID, or 0 if creation failed
     */
    uint32_t create_room(uint32_t host_player_id,
                         const std::string& room_name,
                         const std::string& password_hash,
                         protocol::GameMode game_mode,
                         protocol::Difficulty difficulty,
                         uint16_t map_id,
                         uint8_t max_players = 0);

    /**
     * @brief Join an existing room
     * @param player_id The player joining
     * @param room_id Target room
     * @param password_hash SHA256 hash of password (if private)
     * @return true if joined successfully
     */
    bool join_room(uint32_t player_id, uint32_t room_id,
                   const std::string& password_hash);

    /**
     * @brief Leave current room
     * @param player_id The player leaving
     * @return true if player was removed
     */
    bool leave_room(uint32_t player_id);

    /**
     * @brief Get list of all public rooms
     * @return Vector of RoomInfo for all public (non-private) rooms
     */
    std::vector<protocol::RoomInfo> get_public_rooms() const;

    /**
     * @brief Manually start a game (host only)
     * @param room_id Target room
     * @param requester_id Player requesting start (must be host)
     * @return true if game started successfully
     */
    bool start_game(uint32_t room_id, uint32_t requester_id);

    /**
     * @brief Get room by ID
     * @param room_id Room identifier
     * @return Pointer to room, or nullptr if not found
     */
    const Lobby* get_room(uint32_t room_id) const;

    /**
     * @brief Get player's current room
     * @param player_id Player identifier
     * @return Room ID, or 0 if player is not in any room
     */
    uint32_t get_player_room(uint32_t player_id) const;

    /**
     * @brief Get all players in a room
     * @param room_id Room identifier
     * @return Vector of player IDs
     */
    std::vector<uint32_t> get_room_players(uint32_t room_id) const;

    /**
     * @brief Update rooms (check countdowns, cleanup empty rooms)
     * Call this every server tick
     */
    void update();

    /**
     * @brief Build room state payload for network
     */
    std::vector<uint8_t> build_room_state_payload(uint32_t room_id) const;

private:
    std::unordered_map<uint32_t, std::unique_ptr<Lobby>> rooms_;
    std::unordered_map<uint32_t, uint32_t> player_to_room_;
    uint32_t next_room_id_;
    ILobbyListener* listener_ = nullptr;

    static constexpr int COUNTDOWN_DURATION_SECONDS = 5;

    void cleanup_empty_rooms();
    void transfer_host(uint32_t room_id, uint32_t leaving_player_id);
    void start_countdown(uint32_t room_id);
    void cancel_countdown(uint32_t room_id);
    void notify_room_state_changed(uint32_t room_id);
    std::string generate_room_name(uint32_t room_id) const;
};

}
