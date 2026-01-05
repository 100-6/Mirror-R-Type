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
    void set_text(const std::string& text);
    void set_on_click(std::function<void()> callback);
    void set_enabled(bool enabled);

    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    bool is_hovered() const { return hovered_; }
    bool is_enabled() const { return enabled_; }

private:
    float x_, y_;
    float width_, height_;
    std::string text_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool enabled_ = true;
    std::function<void()> on_click_;

    // Colors
    engine::Color normal_color_ = {70, 130, 180, 255};  // SteelBlue
    engine::Color hover_color_ = {100, 150, 210, 255};  // Lighter blue
    engine::Color pressed_color_ = {50, 100, 150, 255};  // Darker blue
    engine::Color disabled_color_ = {100, 100, 100, 255};  // Gray
};

}  // namespace rtype
