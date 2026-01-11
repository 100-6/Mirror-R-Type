#pragma once

#include <string>
#include <functional>
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"

namespace rtype {

class UIButton {
public:
    UIButton(float x, float y, float width, float height, const std::string& text);

    void set_position(float x, float y);
    void set_size(float width, float height);
    void set_text(const std::string& text);
    void set_on_click(std::function<void()> callback);
    void set_enabled(bool enabled);
    void set_selected(bool selected) { selected_ = selected; }
    void set_invisible(bool invisible);

    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    bool is_hovered() const { return hovered_; }
    bool is_enabled() const { return enabled_; }
    bool is_selected() const { return selected_; }
    float get_x() const { return x_; }
    float get_y() const { return y_; }
    float get_width() const { return width_; }
    float get_height() const { return height_; }

private:
    float x_, y_;
    float width_, height_;
    std::string text_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool enabled_ = true;
    bool selected_ = false;
    bool invisible_ = false;
    std::function<void()> on_click_;

    // Modern styling properties - enhanced for better visual impact
    float corner_radius_ = 15.0f;
    float shadow_offset_ = 5.0f;
    float glow_intensity_ = 0.0f;

    // Enhanced color scheme with more vibrant gradients
    engine::Color normal_color_ = {50, 60, 95, 240};  // Richer deep blue
    engine::Color normal_color_highlight_ = {70, 85, 135, 240};  // More pronounced gradient
    engine::Color hover_color_ = {90, 110, 170, 250};  // Vibrant hover blue
    engine::Color hover_color_highlight_ = {120, 145, 210, 250};  // Bright gradient end
    engine::Color pressed_color_ = {35, 45, 75, 255};  // Deeper pressed state
    engine::Color disabled_color_ = {55, 55, 65, 180};  // Muted gray
    engine::Color selected_color_ = {110, 60, 220, 245};  // Vibrant purple for selected
    engine::Color glow_color_ = {130, 170, 255, 200};  // Brighter blue glow
    engine::Color shadow_color_ = {0, 0, 0, 120};  // Darker shadow for depth
};

}  // namespace rtype
