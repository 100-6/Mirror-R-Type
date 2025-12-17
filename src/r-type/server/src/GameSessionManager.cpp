/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameSessionManager implementation
*/

#include "GameSessionManager.hpp"
#include <iostream>

namespace rtype::server {

GameSessionManager::GameSessionManager() = default;

GameSession* GameSessionManager::create_session(uint32_t session_id, protocol::GameMode game_mode,
                                                protocol::Difficulty difficulty, uint32_t level_seed) {
    auto session = std::make_unique<GameSession>(session_id, game_mode, difficulty, level_seed);
    auto* session_ptr = session.get();

    // Pass our listener to the new session
    if (listener_) {
        session_ptr->set_listener(listener_);
    }

    sessions_[session_id] = std::move(session);
    std::cout << "[GameSessionManager] Created session " << session_id << "\n";

    return session_ptr;
}

GameSession* GameSessionManager::get_session(uint32_t session_id) {
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void GameSessionManager::update_all(float delta_time) {
    for (auto& [session_id, session] : sessions_) {
        session->update(delta_time);
    }
}

void GameSessionManager::cleanup_inactive_sessions() {
    std::vector<uint32_t> sessions_to_remove;

    for (auto& [session_id, session] : sessions_) {
        if (!session->is_active()) {
            sessions_to_remove.push_back(session_id);
        }
    }

    for (uint32_t session_id : sessions_to_remove) {
        std::cout << "[GameSessionManager] Removing inactive session " << session_id << "\n";
        sessions_.erase(session_id);
    }
}

void GameSessionManager::remove_session(uint32_t session_id) {
    sessions_.erase(session_id);
    std::cout << "[GameSessionManager] Removed session " << session_id << "\n";
}

std::vector<uint32_t> GameSessionManager::get_active_session_ids() const {
    std::vector<uint32_t> session_ids;
    session_ids.reserve(sessions_.size());

    for (const auto& [session_id, session] : sessions_) {
        if (session->is_active()) {
            session_ids.push_back(session_id);
        }
    }

    return session_ids;
}

}
