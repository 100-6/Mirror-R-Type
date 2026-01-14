/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** DebugNetworkOverlay - Implementation
*/

#include "DebugNetworkOverlay.hpp"
#include <cmath>

namespace rtype::client {

DebugNetworkOverlay::DebugNetworkOverlay(bool enabled)
    : enabled_(enabled)
    , rtt_ms_(0.0f)
    , corrections_per_second_(0)
    , input_buffer_size_(0)
    , server_x_(0.0f)
    , server_y_(0.0f)
    , predicted_x_(0.0f)
    , predicted_y_(0.0f)
    , corrections_this_second_(0)
{
}

void DebugNetworkOverlay::render(engine::IGraphicsPlugin& graphics)
{
    if (!enabled_)
        return;

    // Build metrics text
    std::ostringstream oss;
    oss << "=== Network Debug ===\n";
    oss << "RTT: " << static_cast<int>(rtt_ms_) << " ms\n";
    oss << "Corrections/s: " << corrections_per_second_ << "\n";
    oss << "Input Buffer: " << input_buffer_size_ << "\n";

    // Calculate prediction error
    float dx = predicted_x_ - server_x_;
    float dy = predicted_y_ - server_y_;
    float distance = std::sqrt(dx * dx + dy * dy);
    oss << "Pred Error: " << static_cast<int>(distance) << " px\n";

    // Draw text overlay (top-left corner)
    engine::Vector2f position = {10.0f, 10.0f};
    engine::Color color = {255, 255, 0, 255};
    graphics.draw_text(oss.str(), position, color);

    // Note: Visualizing positions requires additional drawing capabilities
    // that may not be available in the current IGraphicsPlugin interface.
    // This would require adding circle/line drawing methods to the plugin.
}

void DebugNetworkOverlay::update_metrics(float rtt_ms, uint32_t corrections_per_second, size_t input_buffer_size)
{
    rtt_ms_ = rtt_ms;
    corrections_per_second_ = corrections_per_second;
    input_buffer_size_ = input_buffer_size;
}

void DebugNetworkOverlay::record_correction()
{
    corrections_this_second_++;
}

void DebugNetworkOverlay::set_server_position(float x, float y)
{
    server_x_ = x;
    server_y_ = y;
}

void DebugNetworkOverlay::set_predicted_position(float x, float y)
{
    predicted_x_ = x;
    predicted_y_ = y;
}

void DebugNetworkOverlay::update_correction_counter()
{
    corrections_per_second_ = corrections_this_second_;
    corrections_this_second_ = 0;
}

}  // namespace rtype::client