#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>
#include <functional>

#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"

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

    Lobby(uint32_t id, protocol::GameMode mode, protocol::Difficulty diff)
        : lobby_id(id)
        , game_mode(mode)
        , difficulty(diff)
        , max_players(get_max_players_for_mode(mode))
        , countdown_active(false)
        , map_id(0) {}

    bool is_full() const {
        return player_ids.size() >= max_players;
    }

    bool has_space() const {
        return player_ids.size() < max_players;
    }

    bool is_empty() const {
        return player_ids.empty();
    }

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
 */
class LobbyManager {
public:
    using LobbyStateCallback = std::function<void(uint32_t lobby_id, const std::vector<uint8_t>& payload)>;
    using CountdownCallback = std::function<void(uint32_t lobby_id, uint8_t seconds_remaining)>;
    using GameStartCallback = std::function<void(uint32_t lobby_id, const std::vector<uint32_t>& player_ids)>;

    LobbyManager();
    ~LobbyManager() = default;

    /**
     * @brief Find or create a lobby for a player
     * @param player_id The player requesting to join
     * @param game_mode Desired game mode
     * @param difficulty Desired difficulty
     * @return Lobby ID, or 0 if no lobby could be created
     */
    uint32_t join_lobby(uint32_t player_id, protocol::GameMode game_mode, protocol::Difficulty difficulty);

    /**
     * @brief Remove a player from their current lobby
     * @param player_id The player leaving
     * @return true if player was in a lobby and was removed
     */
    bool leave_lobby(uint32_t player_id);

    /**
     * @brief Get the lobby ID for a player
     * @param player_id The player to check
     * @return Lobby ID, or 0 if player is not in a lobby
     */
    uint32_t get_player_lobby(uint32_t player_id) const;

    /**
     * @brief Get all player IDs in a specific lobby
     * @param lobby_id The lobby to check
     * @return Vector of player IDs
     */
    std::vector<uint32_t> get_lobby_players(uint32_t lobby_id) const;

    /**
     * @brief Get lobby by ID
     * @param lobby_id The lobby to get
     * @return Pointer to lobby, or nullptr if not found
     */
    const Lobby* get_lobby(uint32_t lobby_id) const;

    /**
     * @brief Update lobby timers and check for countdown completion
     * Should be called from main server loop
     */
    void update();

    /**
     * @brief Build SERVER_LOBBY_STATE payload for a lobby
     * @param lobby_id The lobby to serialize
     * @return Serialized payload
     */
    std::vector<uint8_t> build_lobby_state_payload(uint32_t lobby_id) const;

    /**
     * @brief Set callback for when lobby state changes (player joins/leaves)
     */
    void set_lobby_state_callback(LobbyStateCallback callback) {
        lobby_state_callback_ = callback;
    }

    /**
     * @brief Set callback for countdown updates
     */
    void set_countdown_callback(CountdownCallback callback) {
        countdown_callback_ = callback;
    }

    /**
     * @brief Set callback for when countdown completes and game should start
     */
    void set_game_start_callback(GameStartCallback callback) {
        game_start_callback_ = callback;
    }

private:
    std::unordered_map<uint32_t, std::unique_ptr<Lobby>> lobbies_;
    std::unordered_map<uint32_t, uint32_t> player_to_lobby_;
    uint32_t next_lobby_id_;

    LobbyStateCallback lobby_state_callback_;
    CountdownCallback countdown_callback_;
    GameStartCallback game_start_callback_;

    static constexpr int COUNTDOWN_DURATION_SECONDS = 5;

    uint32_t create_lobby(protocol::GameMode game_mode, protocol::Difficulty difficulty);
    uint32_t find_available_lobby(protocol::GameMode game_mode, protocol::Difficulty difficulty);
    void start_countdown(uint32_t lobby_id);
    void cancel_countdown(uint32_t lobby_id);
    void notify_lobby_state_changed(uint32_t lobby_id);
};

}
