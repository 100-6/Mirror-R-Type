#include "ui/UITextField.hpp"
#include <algorithm>

namespace rtype {

UITextField::UITextField(float x, float y, float width, float height, const std::string& placeholder)
    : x_(x), y_(y), width_(width), height_(height), placeholder_(placeholder) {
}

void UITextField::set_position(float x, float y) {
    x_ = x;
    y_ = y;
}

void UITextField::set_text(const std::string& text) {
    text_ = text;
    if (text_.length() > max_length_) {
        text_ = text_.substr(0, max_length_);
    }
    if (on_change_) {
        on_change_(text_);
    }
}

void UITextField::set_placeholder(const std::string& placeholder) {
    placeholder_ = placeholder;
}

void UITextField::set_max_length(size_t max_length) {
    max_length_ = max_length;
    if (text_.length() > max_length_) {
        text_ = text_.substr(0, max_length_);
    }
}

void UITextField::set_password_mode(bool password_mode) {
    password_mode_ = password_mode;
}

void UITextField::set_on_change(std::function<void(const std::string&)> callback) {
    on_change_ = callback;
}

void UITextField::set_focused(bool focused) {
    focused_ = focused;
    if (focused_) {
        cursor_blink_timer_ = 0.0f;
    }
}

void UITextField::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    // Check if clicked
    engine::Vector2f mouse_pos = input->get_mouse_position();

    bool is_over = mouse_pos.x >= x_ && mouse_pos.x <= x_ + width_ &&
                   mouse_pos.y >= y_ && mouse_pos.y <= y_ + height_;

    // Track mouse button state for click detection
    bool is_pressed = input->is_mouse_button_pressed(engine::MouseButton::Left);

    // Detect click (transition from not pressed to pressed)
    if (is_pressed && !was_mouse_pressed_) {
        focused_ = is_over;
        if (focused_) {
            cursor_blink_timer_ = 0.0f;
            last_key_ = engine::Key::Unknown;  // Reset key repeat
            key_repeat_timer_ = 0.0f;
        }
    }
    was_mouse_pressed_ = is_pressed;

    if (focused_) {
        handle_text_input(input);
        cursor_blink_timer_ += 0.016f;  // Assuming ~60 FPS
        if (cursor_blink_timer_ > 1.0f) {
            cursor_blink_timer_ = 0.0f;
        }
    }
}

void UITextField::handle_text_input(engine::IInputPlugin* input) {
    const float KEY_REPEAT_DELAY = 0.15f;  // 150ms delay between repeats

    // Handle backspace
    if (input->is_key_pressed(engine::Key::Backspace)) {
        if (last_key_ != engine::Key::Backspace || key_repeat_timer_ <= 0.0f) {
            if (!text_.empty()) {
                text_.pop_back();
                if (on_change_) {
                    on_change_(text_);
                }
            }
            last_key_ = engine::Key::Backspace;
            key_repeat_timer_ = KEY_REPEAT_DELAY;
        }
        key_repeat_timer_ -= 0.016f;  // Approx 60 FPS
        return;
    }

    // Handle alphanumeric keys
    static const engine::Key letter_keys[] = {
        engine::Key::A, engine::Key::B, engine::Key::C, engine::Key::D, engine::Key::E,
        engine::Key::F, engine::Key::G, engine::Key::H, engine::Key::I, engine::Key::J,
        engine::Key::K, engine::Key::L, engine::Key::M, engine::Key::N, engine::Key::O,
        engine::Key::P, engine::Key::Q, engine::Key::R, engine::Key::S, engine::Key::T,
        engine::Key::U, engine::Key::V, engine::Key::W, engine::Key::X, engine::Key::Y,
        engine::Key::Z
    };

    static const engine::Key digit_keys[] = {
        engine::Key::Num0, engine::Key::Num1, engine::Key::Num2, engine::Key::Num3,
        engine::Key::Num4, engine::Key::Num5, engine::Key::Num6, engine::Key::Num7,
        engine::Key::Num8, engine::Key::Num9
    };

    if (text_.length() < max_length_) {
        // Check letters
        for (size_t i = 0; i < 26; ++i) {
            if (input->is_key_pressed(letter_keys[i])) {
                if (last_key_ != letter_keys[i] || key_repeat_timer_ <= 0.0f) {
                    char ch = 'a' + i;
                    // Check for shift (uppercase)
                    if (input->is_key_pressed(engine::Key::LShift) ||
                        input->is_key_pressed(engine::Key::RShift)) {
                        ch = 'A' + i;
                    }
                    text_ += ch;
                    if (on_change_) {
                        on_change_(text_);
                    }
                    last_key_ = letter_keys[i];
                    key_repeat_timer_ = KEY_REPEAT_DELAY;
                }
                key_repeat_timer_ -= 0.016f;
                return;
            }
        }

        // Check digits
        for (size_t i = 0; i < 10; ++i) {
            if (input->is_key_pressed(digit_keys[i])) {
                if (last_key_ != digit_keys[i] || key_repeat_timer_ <= 0.0f) {
                    text_ += ('0' + i);
                    if (on_change_) {
                        on_change_(text_);
                    }
                    last_key_ = digit_keys[i];
                    key_repeat_timer_ = KEY_REPEAT_DELAY;
                }
                key_repeat_timer_ -= 0.016f;
                return;
            }
        }

        // Handle space
        if (input->is_key_pressed(engine::Key::Space)) {
            if (last_key_ != engine::Key::Space || key_repeat_timer_ <= 0.0f) {
                text_ += ' ';
                if (on_change_) {
                    on_change_(text_);
                }
                last_key_ = engine::Key::Space;
                key_repeat_timer_ = KEY_REPEAT_DELAY;
            }
            key_repeat_timer_ -= 0.016f;
            return;
        }

        // Handle hyphen/underscore
        if (input->is_key_pressed(engine::Key::Hyphen)) {
            if (last_key_ != engine::Key::Hyphen || key_repeat_timer_ <= 0.0f) {
                bool shift = input->is_key_pressed(engine::Key::LShift) ||
                            input->is_key_pressed(engine::Key::RShift);
                text_ += shift ? '_' : '-';
                if (on_change_) {
                    on_change_(text_);
                }
                last_key_ = engine::Key::Hyphen;
                key_repeat_timer_ = KEY_REPEAT_DELAY;
            }
            key_repeat_timer_ -= 0.016f;
            return;
        }

        // Handle period (for IP addresses)
        if (input->is_key_pressed(engine::Key::Period)) {
            if (last_key_ != engine::Key::Period || key_repeat_timer_ <= 0.0f) {
                text_ += '.';
                if (on_change_) {
                    on_change_(text_);
                }
                last_key_ = engine::Key::Period;
                key_repeat_timer_ = KEY_REPEAT_DELAY;
            }
            key_repeat_timer_ -= 0.016f;
            return;
        }

        if (input->is_key_pressed(engine::Key::Num8)) {
            if (last_key_ != engine::Key::Num8 || key_repeat_timer_ <= 0.0f) {
                bool shift = input->is_key_pressed(engine::Key::LShift) ||
                            input->is_key_pressed(engine::Key::RShift);
                if (shift) {
                    text_ += '_';
                    if (on_change_) {
                        on_change_(text_);
                    }
                    last_key_ = engine::Key::Num8;
                    key_repeat_timer_ = KEY_REPEAT_DELAY;
                }
            }
            key_repeat_timer_ -= 0.016f;
            if (input->is_key_pressed(engine::Key::LShift) || input->is_key_pressed(engine::Key::RShift))
                return;
        }
    }

    // Reset if no key is pressed
    last_key_ = engine::Key::Unknown;
    key_repeat_timer_ = 0.0f;
}

void UITextField::draw(engine::IGraphicsPlugin* graphics) {
    float corner_radius = 25.0f;  // More rounded corners for futuristic look

    // Draw deep shadow for depth (rounded, more pronounced)
    float shadow_offset = 6.0f;

    // Shadow main rectangles (horizontal and vertical, without corners)
    engine::Rectangle shadow_h{x_ + shadow_offset + corner_radius, y_ + shadow_offset, width_ - corner_radius * 2, height_};
    graphics->draw_rectangle(shadow_h, {0, 0, 0, 140});
    engine::Rectangle shadow_v{x_ + shadow_offset, y_ + shadow_offset + corner_radius, width_, height_ - corner_radius * 2};
    graphics->draw_rectangle(shadow_v, {0, 0, 0, 140});

    // Shadow corner circles only
    graphics->draw_circle({x_ + shadow_offset + corner_radius, y_ + shadow_offset + corner_radius}, corner_radius, {0, 0, 0, 140});
    graphics->draw_circle({x_ + width_ + shadow_offset - corner_radius, y_ + shadow_offset + corner_radius}, corner_radius, {0, 0, 0, 140});
    graphics->draw_circle({x_ + shadow_offset + corner_radius, y_ + height_ + shadow_offset - corner_radius}, corner_radius, {0, 0, 0, 140});
    graphics->draw_circle({x_ + width_ + shadow_offset - corner_radius, y_ + height_ + shadow_offset - corner_radius}, corner_radius, {0, 0, 0, 140});

    // Draw outer purple/blue glow if focused (R-Type style)
    if (focused_) {
        for (int i = 4; i > 0; i--) {
            float expand = 12.0f * i;
            engine::Color glow_color{140, 80, 255, static_cast<unsigned char>(40 / i)};  // Purple glow

            // Main glow rectangles
            engine::Rectangle glow_h{x_ - expand + corner_radius, y_ - expand, width_ - corner_radius * 2, height_ + expand * 2};
            graphics->draw_rectangle(glow_h, glow_color);
            engine::Rectangle glow_v{x_ - expand, y_ - expand + corner_radius, width_ + expand * 2, height_ - corner_radius * 2};
            graphics->draw_rectangle(glow_v, glow_color);

            // Glow corners
            float glow_radius = corner_radius + expand;
            graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, glow_radius, glow_color);
            graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, glow_radius, glow_color);
            graphics->draw_circle({x_ + corner_radius, y_ + height_ - corner_radius}, glow_radius, glow_color);
            graphics->draw_circle({x_ + width_ - corner_radius, y_ + height_ - corner_radius}, glow_radius, glow_color);
        }
    }

    // Draw background with purple tint
    engine::Color bg_color = focused_ ?
        engine::Color{30, 20, 50, 245} :      // Purple-tinted background when focused
        engine::Color{22, 18, 35, 235};       // Dark purple-tinted background

    // Main background rectangles (without corners)
    engine::Rectangle bg_h{x_ + corner_radius, y_, width_ - corner_radius * 2, height_};
    graphics->draw_rectangle(bg_h, bg_color);
    engine::Rectangle bg_v{x_, y_ + corner_radius, width_, height_ - corner_radius * 2};
    graphics->draw_rectangle(bg_v, bg_color);

    // Corner circles for rounded effect
    graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, corner_radius, bg_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, corner_radius, bg_color);
    graphics->draw_circle({x_ + corner_radius, y_ + height_ - corner_radius}, corner_radius, bg_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + height_ - corner_radius}, corner_radius, bg_color);

    // Draw inner purple gradient (top portion for sci-fi effect)
    engine::Color gradient_color = focused_ ?
        engine::Color{60, 40, 90, 80} :
        engine::Color{45, 30, 65, 70};

    engine::Rectangle inner_gradient_h{x_ + corner_radius, y_, width_ - corner_radius * 2, height_ / 2.5f};
    graphics->draw_rectangle(inner_gradient_h, gradient_color);
    engine::Rectangle inner_gradient_v{x_, y_ + corner_radius, width_, height_ / 2.5f - corner_radius};
    graphics->draw_rectangle(inner_gradient_v, gradient_color);
    // Top corners gradient
    graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, corner_radius, gradient_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, corner_radius, gradient_color);

    // Draw energy line accent at bottom (R-Type style)
    engine::Color accent_color = focused_ ?
        engine::Color{180, 120, 255, 200} :
        engine::Color{100, 70, 150, 150};
    float accent_height = 2.0f;
    engine::Rectangle bottom_accent{x_ + corner_radius, y_ + height_ - accent_height - 8, width_ - corner_radius * 2, accent_height};
    graphics->draw_rectangle(bottom_accent, accent_color);

    // Draw border with purple/blue tint
    engine::Color border_color = focused_ ?
        engine::Color{160, 100, 255, 255} :   // Bright purple border when focused
        engine::Color{90, 70, 140, 200};      // Muted purple border
    float border_width = focused_ ? 3.5f : 2.5f;

    // Border rectangles (top, bottom, left, right)
    engine::Rectangle border_top{x_ + corner_radius, y_, width_ - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_top, border_color);
    engine::Rectangle border_bottom{x_ + corner_radius, y_ + height_ - border_width, width_ - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_bottom, border_color);
    engine::Rectangle border_left{x_, y_ + corner_radius, border_width, height_ - corner_radius * 2};
    graphics->draw_rectangle(border_left, border_color);
    engine::Rectangle border_right{x_ + width_ - border_width, y_ + corner_radius, border_width, height_ - corner_radius * 2};
    graphics->draw_rectangle(border_right, border_color);

    // Draw text or placeholder
    std::string display_text;
    engine::Color text_color;

    if (text_.empty() && !focused_) {
        display_text = placeholder_;
        text_color = {160, 140, 200, 200};  // Purple-tinted placeholder
    } else {
        if (password_mode_) {
            display_text = std::string(text_.length(), '*');
        } else {
            display_text = text_;
        }
        text_color = {230, 220, 255, 255};  // Light purple-white for text
    }

    // Center text vertically with better padding
    float font_size = 30.0f;  // Larger font for better readability
    float text_y = y_ + (height_ - font_size) / 2.0f + 3.0f;
    engine::Vector2f text_pos{x_ + 30.0f, text_y};
    graphics->draw_text(display_text, text_pos, text_color, engine::INVALID_HANDLE, static_cast<int>(font_size));

    // Draw animated cursor if focused (purple/magenta)
    if (focused_ && cursor_blink_timer_ < 0.5f) {
        float cursor_x = x_ + 30.0f + display_text.length() * 15.0f;
        float cursor_margin = 14.0f;
        engine::Vector2f cursor_start{cursor_x, y_ + cursor_margin};
        engine::Vector2f cursor_end{cursor_x, y_ + height_ - cursor_margin};
        engine::Color cursor_color = {200, 150, 255, 255};  // Purple cursor
        graphics->draw_line(cursor_start, cursor_end, cursor_color, 3.5f);

        // Add purple glow to cursor
        graphics->draw_line(cursor_start, cursor_end, {180, 120, 255, 100}, 8.0f);
    }
}

}  // namespace rtype
