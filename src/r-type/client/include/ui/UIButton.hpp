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
    void set_selected(bool selected) { selected_ = selected; }

    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    bool is_hovered() const { return hovered_; }
    bool is_enabled() const { return enabled_; }
    bool is_selected() const { return selected_; }

private:
    float x_, y_;
    float width_, height_;
    std::string text_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool enabled_ = true;
    bool selected_ = false;
    std::function<void()> on_click_;

    // Colors
    engine::Color normal_color_ = {70, 130, 180, 255};  // SteelBlue
    engine::Color hover_color_ = {100, 150, 210, 255};  // Lighter blue
    engine::Color pressed_color_ = {50, 100, 150, 255};  // Darker blue
    engine::Color disabled_color_ = {100, 100, 100, 255};  // Gray
    engine::Color selected_color_ = {50, 200, 100, 255};  // Green for selected
};

}  // namespace rtype
