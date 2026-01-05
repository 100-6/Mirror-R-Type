/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RoomManager implementation
*/

#include "RoomManager.hpp"
#include <algorithm>
#include <cstring>

namespace rtype::server {

RoomManager::RoomManager()
    : next_room_id_(1000)
{
}

uint32_t RoomManager::create_room(uint32_t host_player_id,
                                   const std::string& room_name,
                                   const std::string& password_hash,
                                   protocol::GameMode game_mode,
                                   protocol::Difficulty difficulty,
                                   uint16_t map_id)
{
    uint32_t room_id = next_room_id_++;
    std::string final_name = room_name.empty() ? generate_room_name(room_id) : room_name;
    auto room = std::make_unique<Lobby>(
        room_id, game_mode, difficulty,
        final_name, password_hash, host_player_id, map_id
    );

    room->player_ids.push_back(host_player_id);
    rooms_[room_id] = std::move(room);
    player_to_room_[host_player_id] = room_id;
    notify_room_state_changed(room_id);
    return room_id;
}

bool RoomManager::join_room(uint32_t player_id, uint32_t room_id,
                             const std::string& password_hash)
{
    auto it = rooms_.find(room_id);

    if (it == rooms_.end()) {
        return false;
    }
    auto& room = it->second;
    if (room->is_full()) {
        return false;
    }
    if (room->status != protocol::RoomStatus::WAITING) {
        return false;
    }
    if (room->is_private() && room->password_hash != password_hash) {
        return false;
    }
    room->player_ids.push_back(player_id);
    player_to_room_[player_id] = room_id;
    if (room->countdown_active) {
        cancel_countdown(room_id);
    }
    if (room->is_full()) {
        start_countdown(room_id);
    }
    notify_room_state_changed(room_id);
    return true;
}

bool RoomManager::leave_room(uint32_t player_id)
{
    auto it = player_to_room_.find(player_id);

    if (it == player_to_room_.end()) {
        return false;
    }
    uint32_t room_id = it->second;
    auto room_it = rooms_.find(room_id);
    if (room_it == rooms_.end()) {
        return false;
    }
    auto& room = room_it->second;
    transfer_host(room_id, player_id);
    auto& players = room->player_ids;
    players.erase(std::remove(players.begin(), players.end(), player_id), players.end());
    player_to_room_.erase(player_id);
    if (room->countdown_active) {
        cancel_countdown(room_id);
    }
    if (!room->is_empty()) {
        notify_room_state_changed(room_id);
    }
    return true;
}

std::vector<protocol::RoomInfo> RoomManager::get_public_rooms() const
{
    std::vector<protocol::RoomInfo> result;

    for (const auto& [room_id, room] : rooms_) {
        if (room->status != protocol::RoomStatus::WAITING) {
            continue;
        }
        protocol::RoomInfo info;
        info.room_id = room_id;
        info.set_name(room->room_name);
        info.game_mode = room->game_mode;
        info.difficulty = room->difficulty;
        info.current_players = static_cast<uint8_t>(room->player_ids.size());
        info.max_players = room->max_players;
        info.map_id = room->map_id;
        info.status = room->status;
        info.is_private = room->is_private() ? 1 : 0;
        result.push_back(info);
    }
    return result;
}

bool RoomManager::start_game(uint32_t room_id, uint32_t requester_id)
{
    auto it = rooms_.find(room_id);

    if (it == rooms_.end()) {
        return false;
    }
    auto& room = it->second;
    if (!room->is_host(requester_id)) {
        return false;
    }
    if (room->is_empty()) {
        return false;
    }
    if (room->status != protocol::RoomStatus::WAITING) {
        return false;
    }
    start_countdown(room_id);
    return true;
}

const Lobby* RoomManager::get_room(uint32_t room_id) const
{
    auto it = rooms_.find(room_id);

    return (it != rooms_.end()) ? it->second.get() : nullptr;
}

uint32_t RoomManager::get_player_room(uint32_t player_id) const
{
    auto it = player_to_room_.find(player_id);

    return (it != player_to_room_.end()) ? it->second : 0;
}

std::vector<uint32_t> RoomManager::get_room_players(uint32_t room_id) const
{
    auto it = rooms_.find(room_id);

    if (it != rooms_.end()) {
        return it->second->player_ids;
    }
    return {};
}

void RoomManager::update()
{
    cleanup_empty_rooms();
    for (auto& [room_id, room] : rooms_) {
        if (room->countdown_active) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - room->countdown_start
            ).count();
            if (elapsed >= COUNTDOWN_DURATION_SECONDS) {
                room->countdown_active = false;
                room->status = protocol::RoomStatus::IN_PROGRESS;
                if (listener_) {
                    listener_->on_game_start(room_id, room->player_ids);
                }
            } else {
                uint8_t remaining = COUNTDOWN_DURATION_SECONDS - static_cast<uint8_t>(elapsed);
                if (listener_) {
                    listener_->on_countdown_tick(room_id, remaining);
                }
            }
        }
    }
}

std::vector<uint8_t> RoomManager::build_room_state_payload(uint32_t room_id) const
{
    return {};
}

void RoomManager::cleanup_empty_rooms()
{
    std::vector<uint32_t> to_remove;

    for (const auto& [room_id, room] : rooms_) {
        if (room->is_empty()) {
            to_remove.push_back(room_id);
        }
    }
    for (uint32_t room_id : to_remove) {
        rooms_.erase(room_id);
    }
}

void RoomManager::transfer_host(uint32_t room_id, uint32_t leaving_player_id)
{
    auto it = rooms_.find(room_id);

    if (it == rooms_.end()) {
        return;
    }
    auto& room = it->second;
    if (!room->is_host(leaving_player_id)) {
        return;
    }
    if (room->player_ids.size() <= 1) {
        return;
    }
    for (uint32_t player_id : room->player_ids) {
        if (player_id != leaving_player_id) {
            room->host_player_id = player_id;
            notify_room_state_changed(room_id);
            break;
        }
    }
}

void RoomManager::start_countdown(uint32_t room_id)
{
    auto it = rooms_.find(room_id);

    if (it == rooms_.end()) {
        return;
    }
    auto& room = it->second;
    room->countdown_active = true;
    room->countdown_start = std::chrono::steady_clock::now();
    if (listener_) {
        listener_->on_countdown_tick(room_id, COUNTDOWN_DURATION_SECONDS);
    }
}

void RoomManager::cancel_countdown(uint32_t room_id)
{
    auto it = rooms_.find(room_id);

    if (it == rooms_.end()) {
        return;
    }
    auto& room = it->second;
    if (room->countdown_active) {
        room->countdown_active = false;
    }
}

void RoomManager::notify_room_state_changed(uint32_t room_id)
{
    if (!listener_) {
        return;
    }
    auto payload = build_room_state_payload(room_id);
    if (!payload.empty()) {
        listener_->on_lobby_state_changed(room_id, payload);
    }
}

std::string RoomManager::generate_room_name(uint32_t room_id) const
{
    return "Room #" + std::to_string(room_id);
}

}
