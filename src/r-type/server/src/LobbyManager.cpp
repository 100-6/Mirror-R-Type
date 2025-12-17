/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LobbyManager implementation
*/

#include "LobbyManager.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <arpa/inet.h>

namespace rtype::server {

LobbyManager::LobbyManager()
    : next_lobby_id_(1)
{
}

uint32_t LobbyManager::join_lobby(uint32_t player_id, protocol::GameMode game_mode, protocol::Difficulty difficulty)
{
    // Check if player already in a lobby
    if (player_to_lobby_.find(player_id) != player_to_lobby_.end()) {
        std::cerr << "[LobbyManager] Player " << player_id << " already in a lobby\n";
        return 0;
    }

    // Find existing lobby or create new one
    uint32_t lobby_id = find_available_lobby(game_mode, difficulty);
    if (lobby_id == 0) {
        lobby_id = create_lobby(game_mode, difficulty);
        if (lobby_id == 0) {
            std::cerr << "[LobbyManager] Failed to create lobby\n";
            return 0;
        }
    }

    // Add player to lobby
    auto& lobby = lobbies_[lobby_id];
    lobby->player_ids.push_back(player_id);
    player_to_lobby_[player_id] = lobby_id;

    std::cout << "[LobbyManager] Player " << player_id << " joined lobby " << lobby_id
              << " (" << lobby->player_ids.size() << "/" << static_cast<int>(lobby->max_players) << ")\n";

    notify_lobby_state_changed(lobby_id);

    // Start countdown if lobby is full
    if (lobby->is_full() && !lobby->countdown_active)
        start_countdown(lobby_id);

    return lobby_id;
}

bool LobbyManager::leave_lobby(uint32_t player_id)
{
    auto it = player_to_lobby_.find(player_id);
    if (it == player_to_lobby_.end())
        return false;

    uint32_t lobby_id = it->second;
    auto lobby_it = lobbies_.find(lobby_id);
    if (lobby_it == lobbies_.end()) {
        player_to_lobby_.erase(it);
        return false;
    }

    // Remove player from lobby
    auto& lobby = lobby_it->second;
    auto player_it = std::find(lobby->player_ids.begin(), lobby->player_ids.end(), player_id);
    if (player_it != lobby->player_ids.end())
        lobby->player_ids.erase(player_it);
    player_to_lobby_.erase(it);

    std::cout << "[LobbyManager] Player " << player_id << " left lobby " << lobby_id
              << " (" << lobby->player_ids.size() << "/" << static_cast<int>(lobby->max_players) << ")\n";

    // Cancel countdown if active
    if (lobby->countdown_active)
        cancel_countdown(lobby_id);

    // Remove empty lobby
    if (lobby->is_empty()) {
        std::cout << "[LobbyManager] Lobby " << lobby_id << " is now empty, removing\n";
        lobbies_.erase(lobby_it);
    } else {
        notify_lobby_state_changed(lobby_id);
    }

    return true;
}

uint32_t LobbyManager::get_player_lobby(uint32_t player_id) const
{
    auto it = player_to_lobby_.find(player_id);
    return (it != player_to_lobby_.end()) ? it->second : 0;
}

std::vector<uint32_t> LobbyManager::get_lobby_players(uint32_t lobby_id) const
{
    auto it = lobbies_.find(lobby_id);
    if (it != lobbies_.end())
        return it->second->player_ids;
    return {};
}

const Lobby* LobbyManager::get_lobby(uint32_t lobby_id) const
{
    auto it = lobbies_.find(lobby_id);
    if (it != lobbies_.end())
        return it->second.get();
    return nullptr;
}

void LobbyManager::update()
{
    auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> lobbies_to_start;

    for (auto& [lobby_id, lobby] : lobbies_) {
        if (!lobby->countdown_active)
            continue;

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lobby->countdown_start);
        int remaining = COUNTDOWN_DURATION_SECONDS - static_cast<int>(elapsed.count());

        if (remaining <= 0) {
            lobbies_to_start.push_back(lobby_id);
        } else if (listener_) {
            listener_->on_countdown_tick(lobby_id, static_cast<uint8_t>(remaining));
        }
    }

    // Start games for lobbies with completed countdown
    for (uint32_t lobby_id : lobbies_to_start) {
        auto& lobby = lobbies_[lobby_id];
        std::cout << "[LobbyManager] Countdown complete for lobby " << lobby_id << ", starting game\n";

        if (listener_)
            listener_->on_game_start(lobby_id, lobby->player_ids);

        // Remove players from mapping
        for (uint32_t player_id : lobby->player_ids)
            player_to_lobby_.erase(player_id);

        // Remove lobby
        lobbies_.erase(lobby_id);
    }
}

std::vector<uint8_t> LobbyManager::build_lobby_state_payload(uint32_t lobby_id) const
{
    auto it = lobbies_.find(lobby_id);
    if (it == lobbies_.end())
        return {};

    const auto& lobby = it->second;
    protocol::ServerLobbyStatePayload payload;
    payload.lobby_id = htonl(lobby_id);
    payload.game_mode = lobby->game_mode;
    payload.difficulty = lobby->difficulty;
    payload.current_player_count = static_cast<uint8_t>(lobby->player_ids.size());
    payload.required_player_count = lobby->max_players;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&payload);
    return std::vector<uint8_t>(bytes, bytes + sizeof(payload));
}

uint32_t LobbyManager::create_lobby(protocol::GameMode game_mode, protocol::Difficulty difficulty)
{
    uint32_t lobby_id = next_lobby_id_++;
    auto lobby = std::make_unique<Lobby>(lobby_id, game_mode, difficulty);

    std::cout << "[LobbyManager] Created lobby " << lobby_id << " (mode: "
              << static_cast<int>(game_mode) << ", difficulty: "
              << static_cast<int>(difficulty) << ")\n";

    lobbies_[lobby_id] = std::move(lobby);
    return lobby_id;
}

uint32_t LobbyManager::find_available_lobby(protocol::GameMode game_mode, protocol::Difficulty difficulty)
{
    for (const auto& [lobby_id, lobby] : lobbies_) {
        if (lobby->game_mode == game_mode &&
            lobby->difficulty == difficulty &&
            lobby->has_space() &&
            !lobby->countdown_active) {
            return lobby_id;
        }
    }
    return 0;
}

void LobbyManager::start_countdown(uint32_t lobby_id)
{
    auto it = lobbies_.find(lobby_id);
    if (it == lobbies_.end())
        return;

    auto& lobby = it->second;
    lobby->countdown_active = true;
    lobby->countdown_start = std::chrono::steady_clock::now();

    std::cout << "[LobbyManager] Starting " << COUNTDOWN_DURATION_SECONDS
              << "-second countdown for lobby " << lobby_id << "\n";

    notify_lobby_state_changed(lobby_id);
}

void LobbyManager::cancel_countdown(uint32_t lobby_id)
{
    auto it = lobbies_.find(lobby_id);
    if (it == lobbies_.end())
        return;

    auto& lobby = it->second;
    if (!lobby->countdown_active)
        return;

    lobby->countdown_active = false;
    std::cout << "[LobbyManager] Countdown cancelled for lobby " << lobby_id << "\n";

    notify_lobby_state_changed(lobby_id);
}

void LobbyManager::notify_lobby_state_changed(uint32_t lobby_id)
{
    if (!listener_)
        return;

    auto payload = build_lobby_state_payload(lobby_id);
    if (!payload.empty())
        listener_->on_lobby_state_changed(lobby_id, payload);
}

}
