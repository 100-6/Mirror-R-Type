/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LobbyManager - Manages game lobbies and matchmaking
*/

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>

#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"
#include "interfaces/ILobbyListener.hpp"

namespace rtype::server {

/**
 * @brief Represents a single game lobby
 */
struct Lobby {
    uint32_t lobby_id;
    protocol::GameMode game_mode;
    protocol::Difficulty difficulty;
    std::vector<uint32_t> player_ids;
    uint8_t max_players;
    bool countdown_active;
    std::chrono::steady_clock::time_point countdown_start;
    uint16_t map_id;

    std::string room_name;
    std::string password_hash;
    uint32_t host_player_id;
    bool is_custom_room;
    protocol::RoomStatus status;

    Lobby(uint32_t id, protocol::GameMode mode, protocol::Difficulty diff)
        : lobby_id(id)
        , game_mode(mode)
        , difficulty(diff)
        , max_players(get_max_players_for_mode(mode))
        , countdown_active(false)
        , map_id(0)
        , room_name("")
        , password_hash("")
        , host_player_id(0)
        , is_custom_room(false)
        , status(protocol::RoomStatus::WAITING) {}

    Lobby(uint32_t id, protocol::GameMode mode, protocol::Difficulty diff,
          const std::string& name, const std::string& pwd_hash,
          uint32_t host, uint16_t map, uint8_t max_plrs = 0)
        : lobby_id(id)
        , game_mode(mode)
        , difficulty(diff)
        , max_players(max_plrs > 0 ? max_plrs : get_max_players_for_mode(mode))
        , countdown_active(false)
        , map_id(map)
        , room_name(name)
        , password_hash(pwd_hash)
        , host_player_id(host)
        , is_custom_room(true)
        , status(protocol::RoomStatus::WAITING) {}

    bool is_full() const { return player_ids.size() >= max_players; }
    bool has_space() const { return player_ids.size() < max_players; }
    bool is_empty() const { return player_ids.empty(); }
    bool is_private() const { return !password_hash.empty(); }
    bool is_host(uint32_t player_id) const { return host_player_id == player_id; }

private:
    static uint8_t get_max_players_for_mode(protocol::GameMode mode) {
        switch (mode) {
            case protocol::GameMode::DUO: return 2;
            case protocol::GameMode::TRIO: return 3;
            case protocol::GameMode::SQUAD: return 4;
            default: return 4;
        }
    }
};

/**
 * @brief Manages all game lobbies and matchmaking
 *
 * Simple class that:
 * - Creates/destroys lobbies
 * - Adds/removes players from lobbies
 * - Handles countdown when lobby is full
 * - Notifies listener when events happen
 */
class LobbyManager {
public:
    LobbyManager();
    ~LobbyManager() = default;

    // === Configuration ===

    /**
     * @brief Set the listener for lobby events
     * @param listener Pointer to listener (must outlive LobbyManager)
     */
    void set_listener(ILobbyListener* listener) { listener_ = listener; }

    // === Player Actions ===

    /**
     * @brief Add a player to a lobby (finds or creates one)
     * @param player_id The player
     * @param game_mode Desired game mode
     * @param difficulty Desired difficulty
     * @return Lobby ID, or 0 if failed
     */
    uint32_t join_lobby(uint32_t player_id, protocol::GameMode game_mode, protocol::Difficulty difficulty);

    /**
     * @brief Remove a player from their lobby
     * @param player_id The player
     * @return true if player was removed
     */
    bool leave_lobby(uint32_t player_id);

    // === Queries ===

    uint32_t get_player_lobby(uint32_t player_id) const;
    std::vector<uint32_t> get_lobby_players(uint32_t lobby_id) const;
    const Lobby* get_lobby(uint32_t lobby_id) const;

    // === Update ===

    /**
     * @brief Update lobbies (check countdowns)
     * Call this every server tick
     */
    void update();

    /**
     * @brief Build lobby state payload for network
     */
    std::vector<uint8_t> build_lobby_state_payload(uint32_t lobby_id) const;

private:
    std::unordered_map<uint32_t, std::unique_ptr<Lobby>> lobbies_;
    std::unordered_map<uint32_t, uint32_t> player_to_lobby_;
    uint32_t next_lobby_id_;
    ILobbyListener* listener_ = nullptr;

    static constexpr int COUNTDOWN_DURATION_SECONDS = 5;

    uint32_t create_lobby(protocol::GameMode game_mode, protocol::Difficulty difficulty);
    uint32_t find_available_lobby(protocol::GameMode game_mode, protocol::Difficulty difficulty);
    void start_countdown(uint32_t lobby_id);
    void cancel_countdown(uint32_t lobby_id);
    void notify_lobby_state_changed(uint32_t lobby_id);
};

}
