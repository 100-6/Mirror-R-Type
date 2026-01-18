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
    // Simple rectangle background
    engine::Color bg_color = focused_ ?
        engine::Color{30, 25, 45, 240} :
        engine::Color{22, 20, 35, 220};

    engine::Rectangle bg{x_, y_, width_, height_};
    graphics->draw_rectangle(bg, bg_color);

    // Simple border
    engine::Color border_color = focused_ ?
        engine::Color{120, 100, 180, 255} :
        engine::Color{70, 60, 100, 180};
    graphics->draw_rectangle_outline(bg, border_color, focused_ ? 2.0f : 1.0f);

    // Draw text or placeholder
    std::string display_text;
    engine::Color text_color;

    if (text_.empty() && !focused_) {
        display_text = placeholder_;
        text_color = {140, 130, 170, 180};
    } else {
        if (password_mode_) {
            display_text = std::string(text_.length(), '*');
        } else {
            display_text = text_;
        }
        text_color = {220, 215, 240, 255};
    }

    // Center text vertically
    float font_size = 18.0f;
    float text_y = y_ + (height_ - font_size) / 2.0f;
    engine::Vector2f text_pos{x_ + 10.0f, text_y};
    graphics->draw_text(display_text, text_pos, text_color, engine::INVALID_HANDLE, static_cast<int>(font_size));

    // Draw cursor if focused
    if (focused_ && cursor_blink_timer_ < 0.5f) {
        float cursor_x = x_ + 10.0f + display_text.length() * 9.0f;
        float cursor_margin = 6.0f;
        engine::Vector2f cursor_start{cursor_x, y_ + cursor_margin};
        engine::Vector2f cursor_end{cursor_x, y_ + height_ - cursor_margin};
        graphics->draw_line(cursor_start, cursor_end, {180, 160, 220, 255}, 2.0f);
    }
}

}  // namespace rtype
