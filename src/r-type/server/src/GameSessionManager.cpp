#include "GameSessionManager.hpp"
#include <iostream>

namespace rtype::server {

GameSessionManager::GameSessionManager() = default;

GameSession* GameSessionManager::create_session(uint32_t session_id, protocol::GameMode game_mode,
                                                protocol::Difficulty difficulty, uint32_t level_seed) {
    auto session = std::make_unique<GameSession>(session_id, game_mode, difficulty, level_seed);
    auto* session_ptr = session.get();

    setup_session_callbacks(session_ptr);

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

void GameSessionManager::setup_session_callbacks(GameSession* session) {
    if (!session)
        return;

    if (on_state_snapshot_) {
        session->set_state_snapshot_callback(on_state_snapshot_);
    }

    if (on_entity_spawn_) {
        session->set_entity_spawn_callback(on_entity_spawn_);
    }

    if (on_entity_destroy_) {
        session->set_entity_destroy_callback(on_entity_destroy_);
    }

    if (on_projectile_spawn_) {
        session->set_projectile_spawn_callback(on_projectile_spawn_);
    }

    if (on_game_over_) {
        session->set_game_over_callback(on_game_over_);
    }

    if (on_wave_start_) {
        session->set_wave_start_network_callback(on_wave_start_);
    }

    if (on_wave_complete_) {
        session->set_wave_complete_network_callback(on_wave_complete_);
    }
}

}
