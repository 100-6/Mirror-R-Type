#include "ui/UIButton.hpp"

namespace rtype {

UIButton::UIButton(float x, float y, float width, float height, const std::string& text)
    : x_(x), y_(y), width_(width), height_(height), text_(text) {
}

void UIButton::set_position(float x, float y) {
    x_ = x;
    y_ = y;
}

void UIButton::set_text(const std::string& text) {
    text_ = text;
}

void UIButton::set_on_click(std::function<void()> callback) {
    on_click_ = callback;
}

void UIButton::set_enabled(bool enabled) {
    enabled_ = enabled;
}

void UIButton::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (!enabled_) {
        hovered_ = false;
        pressed_ = false;
        return;
    }

    // Get mouse position
    engine::Vector2f mouse_pos = input->get_mouse_position();

    // Check if mouse is over button
    hovered_ = mouse_pos.x >= x_ && mouse_pos.x <= x_ + width_ &&
               mouse_pos.y >= y_ && mouse_pos.y <= y_ + height_;

    // Check for click - use is_mouse_button_pressed for immediate response
    if (hovered_ && input->is_mouse_button_pressed(engine::MouseButton::Left)) {
        if (!pressed_) {
            pressed_ = true;
            // Trigger callback immediately on press
            if (on_click_) {
                on_click_();
            }
        }
    } else if (!input->is_mouse_button_pressed(engine::MouseButton::Left)) {
        pressed_ = false;
    }
}

void UIButton::draw(engine::IGraphicsPlugin* graphics) {
    // Choose color based on state
    engine::Color color;
    if (!enabled_) {
        color = disabled_color_;
    } else if (pressed_) {
        color = pressed_color_;
    } else if (hovered_) {
        color = hover_color_;
    } else {
        color = normal_color_;
    }

    // Draw button background
    engine::Rectangle rect{x_, y_, width_, height_};
    graphics->draw_rectangle(rect, color);

    // Draw button border
    engine::Color border_color = {255, 255, 255, 255};
    if (!enabled_) {
        border_color = {150, 150, 150, 255};
    }
    graphics->draw_rectangle_outline(rect, border_color, 2.0f);

    // Draw text centered (approximate centering - 10 pixels per char, 20 pixels font size)
    int text_width_approx = text_.length() * 10;
    int text_height_approx = 20;

    engine::Vector2f text_pos{
        x_ + (width_ - text_width_approx) / 2.0f,
        y_ + (height_ - text_height_approx) / 2.0f
    };

    engine::Color text_color = {255, 255, 255, 255};
    if (!enabled_) {
        text_color = {150, 150, 150, 255};
    }

    graphics->draw_text(text_, text_pos, text_color, engine::INVALID_HANDLE, 20);
}

}  // namespace rtype
