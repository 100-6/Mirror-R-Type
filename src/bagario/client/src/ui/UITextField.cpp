#include "ui/UITextField.hpp"
#include <algorithm>

namespace bagario {

UITextField::UITextField(float x, float y, float width, float height, const std::string& placeholder)
    : x_(x), y_(y), width_(width), height_(height), placeholder_(placeholder) {
    last_key_time_ = std::chrono::steady_clock::now();
    last_cursor_blink_ = std::chrono::steady_clock::now();
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

void UITextField::set_on_change(std::function<void(const std::string&)> callback) {
    on_change_ = callback;
}

void UITextField::set_focused(bool focused) {
    focused_ = focused;
    if (focused_) {
        last_cursor_blink_ = std::chrono::steady_clock::now();
    }
}

void UITextField::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    engine::Vector2f mouse_pos = input->get_mouse_position();

    bool is_over = mouse_pos.x >= x_ && mouse_pos.x <= x_ + width_ &&
                   mouse_pos.y >= y_ && mouse_pos.y <= y_ + height_;

    bool is_pressed = input->is_mouse_button_pressed(engine::MouseButton::Left);

    if (is_pressed && !was_mouse_pressed_) {
        focused_ = is_over;
        if (focused_) {
            last_cursor_blink_ = std::chrono::steady_clock::now();
            last_key_ = engine::Key::Unknown;
            last_key_was_pressed_ = false;
        }
    }
    was_mouse_pressed_ = is_pressed;

    if (focused_) {
        handle_text_input(input);
    }
}

void UITextField::handle_text_input(engine::IInputPlugin* input) {
    using namespace std::chrono;

    const auto KEY_INITIAL_DELAY = milliseconds(400);  // Delay before repeat starts
    const auto KEY_REPEAT_DELAY = milliseconds(35);    // Delay between repeats

    auto now = steady_clock::now();
    auto time_since_last = duration_cast<milliseconds>(now - last_key_time_);

    // Helper lambda to process a key
    auto process_key = [&](engine::Key key, char ch) -> bool {
        bool is_pressed = input->is_key_pressed(key);

        if (!is_pressed) {
            return false;
        }

        // Check if this is a new key press (different key or key was released)
        bool is_new_press = (last_key_ != key) || !last_key_was_pressed_;

        if (is_new_press) {
            // New key press - add character immediately
            text_ += ch;
            if (on_change_) {
                on_change_(text_);
            }
            last_key_ = key;
            last_key_was_pressed_ = true;
            last_key_time_ = now;
            return true;
        }

        // Same key still held - check for repeat
        auto delay = (time_since_last < KEY_INITIAL_DELAY + KEY_REPEAT_DELAY)
                     ? KEY_INITIAL_DELAY : KEY_REPEAT_DELAY;

        if (time_since_last >= delay) {
            text_ += ch;
            if (on_change_) {
                on_change_(text_);
            }
            last_key_time_ = now;
        }
        return true;
    };

    // Handle backspace
    bool backspace_pressed = input->is_key_pressed(engine::Key::Backspace);
    if (backspace_pressed) {
        bool is_new_press = (last_key_ != engine::Key::Backspace) || !last_key_was_pressed_;

        if (is_new_press) {
            if (!text_.empty()) {
                text_.pop_back();
                if (on_change_) {
                    on_change_(text_);
                }
            }
            last_key_ = engine::Key::Backspace;
            last_key_was_pressed_ = true;
            last_key_time_ = now;
        } else {
            // Repeat
            auto delay = (time_since_last < KEY_INITIAL_DELAY + KEY_REPEAT_DELAY)
                         ? KEY_INITIAL_DELAY : KEY_REPEAT_DELAY;

            if (time_since_last >= delay && !text_.empty()) {
                text_.pop_back();
                if (on_change_) {
                    on_change_(text_);
                }
                last_key_time_ = now;
            }
        }
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
        // Letters
        for (size_t i = 0; i < 26; ++i) {
            if (input->is_key_pressed(letter_keys[i])) {
                char ch = 'a' + i;
                if (input->is_key_pressed(engine::Key::LShift) ||
                    input->is_key_pressed(engine::Key::RShift)) {
                    ch = 'A' + i;
                }
                if (process_key(letter_keys[i], ch)) {
                    return;
                }
            }
        }

        // Digits
        for (size_t i = 0; i < 10; ++i) {
            if (input->is_key_pressed(digit_keys[i])) {
                if (process_key(digit_keys[i], '0' + i)) {
                    return;
                }
            }
        }

        // Special characters
        struct SpecialKey {
            engine::Key key;
            char normal;
            char shifted;
        };
        static const SpecialKey special_keys[] = {
            {engine::Key::Slash, '/', '?'},
            {engine::Key::Period, '.', '>'},
            {engine::Key::Hyphen, '-', '_'},
            {engine::Key::Space, ' ', ' '},
        };

        for (const auto& sk : special_keys) {
            if (input->is_key_pressed(sk.key)) {
                char ch = sk.normal;
                if (input->is_key_pressed(engine::Key::LShift) ||
                    input->is_key_pressed(engine::Key::RShift)) {
                    ch = sk.shifted;
                }
                if (process_key(sk.key, ch)) {
                    return;
                }
            }
        }
    }

    // No relevant key pressed - mark as released
    last_key_was_pressed_ = false;
}

void UITextField::draw(engine::IGraphicsPlugin* graphics) {
    float corner_radius = 20.0f;

    // Shadow
    float shadow_offset = 4.0f;
    engine::Color shadow{0, 0, 0, 100};

    engine::Rectangle shadow_h{x_ + shadow_offset + corner_radius, y_ + shadow_offset, width_ - corner_radius * 2, height_};
    graphics->draw_rectangle(shadow_h, shadow);
    engine::Rectangle shadow_v{x_ + shadow_offset, y_ + shadow_offset + corner_radius, width_, height_ - corner_radius * 2};
    graphics->draw_rectangle(shadow_v, shadow);

    graphics->draw_circle({x_ + shadow_offset + corner_radius, y_ + shadow_offset + corner_radius}, corner_radius, shadow);
    graphics->draw_circle({x_ + width_ + shadow_offset - corner_radius, y_ + shadow_offset + corner_radius}, corner_radius, shadow);
    graphics->draw_circle({x_ + shadow_offset + corner_radius, y_ + height_ + shadow_offset - corner_radius}, corner_radius, shadow);
    graphics->draw_circle({x_ + width_ + shadow_offset - corner_radius, y_ + height_ + shadow_offset - corner_radius}, corner_radius, shadow);

    // Glow when focused
    if (focused_) {
        for (int i = 3; i > 0; i--) {
            float expand = 8.0f * i;
            engine::Color glow{76, 175, 80, static_cast<unsigned char>(30 / i)};

            engine::Rectangle glow_h{x_ - expand + corner_radius, y_ - expand, width_ - corner_radius * 2, height_ + expand * 2};
            graphics->draw_rectangle(glow_h, glow);
            engine::Rectangle glow_v{x_ - expand, y_ - expand + corner_radius, width_ + expand * 2, height_ - corner_radius * 2};
            graphics->draw_rectangle(glow_v, glow);

            float glow_radius = corner_radius + expand;
            graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, glow_radius, glow);
            graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, glow_radius, glow);
            graphics->draw_circle({x_ + corner_radius, y_ + height_ - corner_radius}, glow_radius, glow);
            graphics->draw_circle({x_ + width_ - corner_radius, y_ + height_ - corner_radius}, glow_radius, glow);
        }
    }

    // Background - dark with slight transparency
    engine::Color bg_color = focused_ ?
        engine::Color{35, 35, 45, 250} :
        engine::Color{25, 25, 35, 240};

    engine::Rectangle bg_h{x_ + corner_radius, y_, width_ - corner_radius * 2, height_};
    graphics->draw_rectangle(bg_h, bg_color);
    engine::Rectangle bg_v{x_, y_ + corner_radius, width_, height_ - corner_radius * 2};
    graphics->draw_rectangle(bg_v, bg_color);

    graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, corner_radius, bg_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, corner_radius, bg_color);
    graphics->draw_circle({x_ + corner_radius, y_ + height_ - corner_radius}, corner_radius, bg_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + height_ - corner_radius}, corner_radius, bg_color);

    // Border
    engine::Color border_color = focused_ ?
        engine::Color{76, 175, 80, 255} :
        engine::Color{100, 100, 120, 200};
    float border_width = focused_ ? 3.0f : 2.0f;

    engine::Rectangle border_top{x_ + corner_radius, y_, width_ - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_top, border_color);
    engine::Rectangle border_bottom{x_ + corner_radius, y_ + height_ - border_width, width_ - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_bottom, border_color);
    engine::Rectangle border_left{x_, y_ + corner_radius, border_width, height_ - corner_radius * 2};
    graphics->draw_rectangle(border_left, border_color);
    engine::Rectangle border_right{x_ + width_ - border_width, y_ + corner_radius, border_width, height_ - corner_radius * 2};
    graphics->draw_rectangle(border_right, border_color);

    // Text or placeholder
    std::string display_text;
    engine::Color text_color;

    if (text_.empty() && !focused_) {
        display_text = placeholder_;
        text_color = {150, 150, 160, 200};
    } else {
        display_text = text_;
        text_color = {255, 255, 255, 255};
    }

    float font_size = 28.0f;
    float text_y = y_ + (height_ - font_size) / 2.0f + 3.0f;
    engine::Vector2f text_pos{x_ + 25.0f, text_y};
    graphics->draw_text(display_text, text_pos, text_color, engine::INVALID_HANDLE, static_cast<int>(font_size));

    // Cursor (blink every 500ms)
    auto now = std::chrono::steady_clock::now();
    auto blink_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cursor_blink_).count();
    bool cursor_visible = (blink_elapsed % 1000) < 500;

    if (focused_ && cursor_visible) {
        float cursor_x = x_ + 25.0f + display_text.length() * 14.0f;
        float cursor_margin = 12.0f;
        engine::Vector2f cursor_start{cursor_x, y_ + cursor_margin};
        engine::Vector2f cursor_end{cursor_x, y_ + height_ - cursor_margin};
        graphics->draw_line(cursor_start, cursor_end, {76, 175, 80, 255}, 3.0f);
    }
}

}  // namespace bagario
