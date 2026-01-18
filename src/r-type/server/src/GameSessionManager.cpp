/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameSessionManager implementation
*/

#include "GameSessionManager.hpp"
#include "ThreadingConfig.hpp"
#include <iostream>

namespace rtype::server {

GameSessionManager::GameSessionManager(size_t thread_pool_size)
    : thread_pool_(std::make_unique<SessionThreadPool>(thread_pool_size))
    , listener_(nullptr)
{
    std::cout << "[GameSessionManager] Initialized with " << thread_pool_size << " worker threads\n";
}

GameSession* GameSessionManager::create_session(uint32_t session_id, protocol::GameMode game_mode,
                                                protocol::Difficulty difficulty, uint32_t level_seed) {
    std::unique_lock lock(sessions_mutex_);
    auto session = std::make_unique<GameSession>(session_id, game_mode, difficulty, level_seed);
    auto* session_ptr = session.get();

    if (listener_)
        session_ptr->set_listener(listener_);
    sessions_[session_id] = std::move(session);
    std::cout << "[GameSessionManager] Created session " << session_id << "\n";
    return session_ptr;
}

GameSession* GameSessionManager::get_session(uint32_t session_id) {
    std::shared_lock lock(sessions_mutex_);
    auto it = sessions_.find(session_id);

    if (it != sessions_.end())
        return it->second.get();
    return nullptr;
}

void GameSessionManager::update_all(float delta_time) {
    std::vector<SessionTask> tasks;
    {
        std::shared_lock lock(sessions_mutex_);
        tasks.reserve(sessions_.size());
        for (auto& [session_id, session] : sessions_)
            if (session->is_active_threadsafe())
                tasks.push_back({session.get(), delta_time});
    }
    if (tasks.empty())
        return;
    thread_pool_->schedule_batch(tasks);
    thread_pool_->wait_for_completion();
}

void GameSessionManager::cleanup_inactive_sessions() {
    std::vector<uint32_t> sessions_to_remove;
    {
        std::shared_lock lock(sessions_mutex_);
        for (auto& [session_id, session] : sessions_)
            if (!session->is_active_threadsafe())
                sessions_to_remove.push_back(session_id);
    }
    if (!sessions_to_remove.empty()) {
        std::unique_lock lock(sessions_mutex_);
        for (uint32_t session_id : sessions_to_remove) {
            std::cout << "[GameSessionManager] Removing inactive session " << session_id << "\n";
            sessions_.erase(session_id);
        }
    }
}

void GameSessionManager::remove_session(uint32_t session_id) {
    std::unique_lock lock(sessions_mutex_);
    sessions_.erase(session_id);
    std::cout << "[GameSessionManager] Removed session " << session_id << "\n";
}

std::vector<uint32_t> GameSessionManager::get_active_session_ids() const {
    std::shared_lock lock(sessions_mutex_);
    std::vector<uint32_t> session_ids;
    session_ids.reserve(sessions_.size());
    for (const auto& [session_id, session] : sessions_)
        if (session->is_active_threadsafe())
            session_ids.push_back(session_id);
    return session_ids;
}

}
