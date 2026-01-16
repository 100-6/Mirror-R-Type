#pragma once

#include <string>
#include <functional>
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"

namespace bagario {

class UIButton {
public:
    UIButton(float x, float y, float width, float height, const std::string& text);

    void set_position(float x, float y);
    void set_size(float width, float height);
    void set_text(const std::string& text);
    void set_on_click(std::function<void()> callback);
    void set_enabled(bool enabled);
    void set_selected(bool selected) { selected_ = selected; }

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
    std::function<void()> on_click_;

    // Agar.io-style vibrant colors
    engine::Color normal_color_ = {76, 175, 80, 240};      // Green
    engine::Color hover_color_ = {102, 187, 106, 250};     // Light green
    engine::Color pressed_color_ = {56, 142, 60, 255};     // Dark green
    engine::Color disabled_color_ = {100, 100, 100, 180};
    engine::Color selected_color_ = {33, 150, 243, 245};   // Blue
    engine::Color glow_color_ = {129, 199, 132, 200};      // Light green glow
};

}
