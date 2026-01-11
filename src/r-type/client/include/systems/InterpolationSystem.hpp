/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InterpolationSystem - Entity interpolation for smooth remote entity movement
*/

#pragma once

#include <deque>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "protocol/Payloads.hpp"

namespace rtype::client {

/**
 * @brief Stores a snapshot state for interpolation
 */
struct SnapshotState {
    uint32_t timestamp;      // Server tick or client timestamp
    float position_x;
    float position_y;
    float velocity_x;
    float velocity_y;
};

/**
 * @brief System for interpolating entity positions between snapshots
 *
 * This system implements entity interpolation by:
 * 1. Buffering incoming snapshots (last 3 per entity)
 * 2. Rendering entities slightly in the past (interpolation delay)
 * 3. Smoothly interpolating between two snapshots
 *
 * This creates smooth movement for remote entities even with low snapshot rates.
 */
class InterpolationSystem {
public:
    /**
     * @brief Construct a new InterpolationSystem
     */
    InterpolationSystem();

    /**
     * @brief Called when a snapshot arrives from server
     * Stores the snapshot data for each entity
     * @param timestamp Server tick number
     * @param entities List of entity states from snapshot
     * @param local_player_id ID of local player (to skip interpolation)
     */
    void on_snapshot_received(uint32_t timestamp,
                               const std::vector<protocol::EntityState>& entities,
                               uint32_t local_player_id);

    /**
     * @brief Get interpolated position for an entity
     * @param entity_id Entity ID to interpolate
     * @param current_time Current client time (in server ticks)
     * @param out_x Output X position
     * @param out_y Output Y position
     * @return true if interpolation succeeded, false if not enough data
     */
    bool get_interpolated_position(uint32_t entity_id, uint32_t current_time,
                                     float& out_x, float& out_y);

    /**
     * @brief Clear all stored snapshot data
     */
    void clear();

private:
    // Buffer of snapshots per entity (stores last 3 snapshots)
    std::unordered_map<uint32_t, std::deque<SnapshotState>> entity_history_;

    // Interpolation delay (render entities 100ms in the past)
    static constexpr uint32_t INTERPOLATION_DELAY_TICKS = 2;  // ~100ms at 20Hz snapshots
    static constexpr size_t MAX_SNAPSHOTS = 3;

    /**
     * @brief Linearly interpolate between two snapshot states
     */
    SnapshotState interpolate(const SnapshotState& from, const SnapshotState& to, float t);
};

}  // namespace rtype::client