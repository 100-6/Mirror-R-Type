/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InterpolationSystem - Implementation
*/

#include "systems/InterpolationSystem.hpp"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace rtype::client {

InterpolationSystem::InterpolationSystem()
{
    // std::cout << "[InterpolationSystem] Initialized\n";
}

void InterpolationSystem::on_snapshot_received(
    uint32_t timestamp,
    const std::vector<protocol::EntityState>& entities,
    uint32_t local_player_id)
{
    for (const auto& entity_state : entities) {
        uint32_t entity_id = ntohl(entity_state.entity_id);

        // Skip local player (uses prediction instead)
        if (entity_id == local_player_id)
            continue;

        // Create snapshot state
        SnapshotState state;
        state.timestamp = timestamp;
        state.position_x = entity_state.position_x;
        state.position_y = entity_state.position_y;
        state.velocity_x = static_cast<float>(ntohs(entity_state.velocity_x)) / 10.0f;
        state.velocity_y = static_cast<float>(ntohs(entity_state.velocity_y)) / 10.0f;

        // Add to buffer
        auto& history = entity_history_[entity_id];
        history.push_back(state);

        // Keep only last 3 snapshots
        if (history.size() > MAX_SNAPSHOTS) {
            history.pop_front();
        }
    }
}

bool InterpolationSystem::get_interpolated_position(
    uint32_t entity_id,
    uint32_t current_time,
    float& out_x,
    float& out_y)
{
    auto it = entity_history_.find(entity_id);
    if (it == entity_history_.end())
        return false;

    auto& history = it->second;

    // Need at least 2 snapshots to interpolate
    if (history.size() < 2)
        return false;

    // Render time = current time - interpolation delay
    uint32_t render_time = (current_time > INTERPOLATION_DELAY_TICKS)
                            ? (current_time - INTERPOLATION_DELAY_TICKS)
                            : 0;

    // Find the two snapshots that bracket render_time
    SnapshotState* from = nullptr;
    SnapshotState* to = nullptr;

    for (size_t i = 0; i < history.size() - 1; ++i) {
        if (history[i].timestamp <= render_time &&
            history[i + 1].timestamp >= render_time) {
            from = &history[i];
            to = &history[i + 1];
            break;
        }
    }

    // If we found a bracket, interpolate
    if (from && to) {
        // Calculate interpolation factor
        uint32_t time_diff = to->timestamp - from->timestamp;
        if (time_diff == 0) {
            out_x = from->position_x;
            out_y = from->position_y;
            return true;
        }

        float t = static_cast<float>(render_time - from->timestamp) /
                  static_cast<float>(time_diff);
        t = std::clamp(t, 0.0f, 1.0f);

        // Interpolate position
        SnapshotState interpolated = interpolate(*from, *to, t);
        out_x = interpolated.position_x;
        out_y = interpolated.position_y;
        return true;
    }

    // If no bracket found, use the most recent snapshot
    if (!history.empty()) {
        out_x = history.back().position_x;
        out_y = history.back().position_y;
        return true;
    }

    return false;
}

void InterpolationSystem::clear()
{
    entity_history_.clear();
}

SnapshotState InterpolationSystem::interpolate(
    const SnapshotState& from,
    const SnapshotState& to,
    float t)
{
    SnapshotState result;
    result.timestamp = from.timestamp +
                       static_cast<uint32_t>((to.timestamp - from.timestamp) * t);
    result.position_x = from.position_x + (to.position_x - from.position_x) * t;
    result.position_y = from.position_y + (to.position_y - from.position_y) * t;
    result.velocity_x = from.velocity_x + (to.velocity_x - from.velocity_x) * t;
    result.velocity_y = from.velocity_y + (to.velocity_y - from.velocity_y) * t;
    return result;
}

}  // namespace rtype::client
