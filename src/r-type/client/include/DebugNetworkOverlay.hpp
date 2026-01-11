/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** DebugNetworkOverlay - Debug visualization for lag compensation
*/

#pragma once

#include <string>
#include <sstream>
#include "plugin_manager/IGraphicsPlugin.hpp"

namespace rtype::client {

/**
 * @brief Debug overlay showing network and lag compensation metrics
 *
 * This overlay displays:
 * - RTT (Round-Trip Time)
 * - Prediction corrections per second
 * - Input buffer size
 * - Server/predicted position difference
 */
class DebugNetworkOverlay {
public:
    /**
     * @brief Construct a new DebugNetworkOverlay
     * @param enabled Initial state (default: disabled)
     */
    explicit DebugNetworkOverlay(bool enabled = false);

    /**
     * @brief Enable or disable the debug overlay
     * @param enabled Whether to show the overlay
     */
    void set_enabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Check if the overlay is enabled
     * @return true if enabled
     */
    bool is_enabled() const { return enabled_; }

    /**
     * @brief Render the debug overlay
     * @param graphics Graphics plugin for rendering
     */
    void render(engine::IGraphicsPlugin& graphics);

    /**
     * @brief Update network metrics
     * @param rtt_ms Round-trip time in milliseconds
     * @param corrections_per_second Number of position corrections in the last second
     * @param input_buffer_size Number of pending inputs in buffer
     */
    void update_metrics(float rtt_ms, uint32_t corrections_per_second, size_t input_buffer_size);

    /**
     * @brief Record a position correction (called when reconciliation happens)
     */
    void record_correction();

    /**
     * @brief Set server position (for visualization)
     * @param x X position
     * @param y Y position
     */
    void set_server_position(float x, float y);

    /**
     * @brief Set predicted position (for visualization)
     * @param x X position
     * @param y Y position
     */
    void set_predicted_position(float x, float y);

    /**
     * @brief Update per-second correction counter (call once per second)
     */
    void update_correction_counter();

private:
    bool enabled_;

    // Metrics
    float rtt_ms_;
    uint32_t corrections_per_second_;
    size_t input_buffer_size_;

    // Position tracking
    float server_x_, server_y_;
    float predicted_x_, predicted_y_;

    // Correction tracking
    uint32_t corrections_this_second_;
};

}  // namespace rtype::client