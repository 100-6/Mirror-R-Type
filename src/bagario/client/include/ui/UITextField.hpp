#pragma once

#include <string>
#include <functional>
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"

namespace bagario {

class UITextField {
public:
    UITextField(float x, float y, float width, float height, const std::string& placeholder = "");

    void set_position(float x, float y);
    void set_text(const std::string& text);
    void set_placeholder(const std::string& placeholder);
    void set_max_length(size_t max_length);
    void set_on_change(std::function<void(const std::string&)> callback);
    void set_focused(bool focused);

    const std::string& get_text() const { return text_; }
    bool is_focused() const { return focused_; }

    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

private:
    float x_, y_;
    float width_, height_;
    std::string text_;
    std::string placeholder_;
    bool focused_ = false;
    size_t max_length_ = 16;
    float cursor_blink_timer_ = 0.0f;
    std::function<void(const std::string&)> on_change_;

    // Key repeat management
    engine::Key last_key_ = engine::Key::Unknown;
    float key_repeat_timer_ = 0.0f;
    bool was_mouse_pressed_ = false;

    void handle_text_input(engine::IInputPlugin* input);
};

}  // namespace bagario
