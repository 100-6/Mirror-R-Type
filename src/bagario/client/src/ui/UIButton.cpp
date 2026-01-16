#include "ui/UIButton.hpp"

namespace bagario {

UIButton::UIButton(float x, float y, float width, float height, const std::string& text)
    : x_(x), y_(y), width_(width), height_(height), text_(text) {
}

void UIButton::set_position(float x, float y) {
    x_ = x;
    y_ = y;
}

void UIButton::set_size(float width, float height) {
    width_ = width;
    height_ = height;
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

    engine::Vector2f mouse_pos = input->get_mouse_position();

    hovered_ = mouse_pos.x >= x_ && mouse_pos.x <= x_ + width_ &&
               mouse_pos.y >= y_ && mouse_pos.y <= y_ + height_;

    if (hovered_ && input->is_mouse_button_pressed(engine::MouseButton::Left)) {
        if (!pressed_) {
            pressed_ = true;
            if (on_click_) {
                on_click_();
            }
        }
    } else if (!input->is_mouse_button_pressed(engine::MouseButton::Left)) {
        pressed_ = false;
    }
}

void UIButton::draw(engine::IGraphicsPlugin* graphics) {
    float corner_radius = 25.0f;

    // Choose colors based on state - Agar.io style vibrant colors
    engine::Color base_color;
    engine::Color highlight_color;
    float glow_strength = 0.0f;

    if (!enabled_) {
        base_color = disabled_color_;
        highlight_color = disabled_color_;
    } else if (pressed_) {
        base_color = pressed_color_;
        highlight_color = {66, 165, 70, 255};
        glow_strength = 0.3f;
    } else if (selected_) {
        base_color = selected_color_;
        highlight_color = {66, 165, 245, 255};
        glow_strength = 1.0f;
    } else if (hovered_) {
        base_color = hover_color_;
        highlight_color = {129, 199, 132, 250};
        glow_strength = 0.6f;
    } else {
        base_color = normal_color_;
        highlight_color = {102, 187, 106, 240};
        glow_strength = 0.0f;
    }

    // Draw shadow for depth
    if (enabled_ && !pressed_) {
        float shadow_offset = 5.0f;
        engine::Color shadow{0, 0, 0, 100};

        // Shadow rectangles
        engine::Rectangle shadow_h{x_ + shadow_offset + corner_radius, y_ + shadow_offset, width_ - corner_radius * 2, height_};
        graphics->draw_rectangle(shadow_h, shadow);
        engine::Rectangle shadow_v{x_ + shadow_offset, y_ + shadow_offset + corner_radius, width_, height_ - corner_radius * 2};
        graphics->draw_rectangle(shadow_v, shadow);

        // Shadow corners
        graphics->draw_circle({x_ + shadow_offset + corner_radius, y_ + shadow_offset + corner_radius}, corner_radius, shadow);
        graphics->draw_circle({x_ + width_ + shadow_offset - corner_radius, y_ + shadow_offset + corner_radius}, corner_radius, shadow);
        graphics->draw_circle({x_ + shadow_offset + corner_radius, y_ + height_ + shadow_offset - corner_radius}, corner_radius, shadow);
        graphics->draw_circle({x_ + width_ + shadow_offset - corner_radius, y_ + height_ + shadow_offset - corner_radius}, corner_radius, shadow);
    }

    // Draw glow effect when hovered
    if (glow_strength > 0.0f) {
        for (int i = 3; i > 0; i--) {
            float expand = 4.0f * i * glow_strength;
            engine::Color glow = glow_color_;
            glow.a = static_cast<unsigned char>((60 / i) * glow_strength);

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

    // Draw main button background with rounded corners
    engine::Rectangle bg_h{x_ + corner_radius, y_, width_ - corner_radius * 2, height_};
    graphics->draw_rectangle(bg_h, base_color);
    engine::Rectangle bg_v{x_, y_ + corner_radius, width_, height_ - corner_radius * 2};
    graphics->draw_rectangle(bg_v, base_color);

    // Corner circles
    graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, corner_radius, base_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, corner_radius, base_color);
    graphics->draw_circle({x_ + corner_radius, y_ + height_ - corner_radius}, corner_radius, base_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + height_ - corner_radius}, corner_radius, base_color);

    // Draw inner gradient on top
    engine::Rectangle top_gradient_h{x_ + corner_radius, y_, width_ - corner_radius * 2, height_ / 2.5f};
    graphics->draw_rectangle(top_gradient_h, highlight_color);
    engine::Rectangle top_gradient_v{x_, y_ + corner_radius, width_, height_ / 2.5f - corner_radius};
    graphics->draw_rectangle(top_gradient_v, highlight_color);

    graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, corner_radius, highlight_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, corner_radius, highlight_color);

    // Draw border
    engine::Color border_color = hovered_ ? engine::Color{255, 255, 255, 200} : engine::Color{255, 255, 255, 120};
    float border_width = 2.5f;

    engine::Rectangle border_top{x_ + corner_radius, y_, width_ - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_top, border_color);
    engine::Rectangle border_bottom{x_ + corner_radius, y_ + height_ - border_width, width_ - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_bottom, border_color);
    engine::Rectangle border_left{x_, y_ + corner_radius, border_width, height_ - corner_radius * 2};
    graphics->draw_rectangle(border_left, border_color);
    engine::Rectangle border_right{x_ + width_ - border_width, y_ + corner_radius, border_width, height_ - corner_radius * 2};
    graphics->draw_rectangle(border_right, border_color);

    // Draw text
    int font_size = 36;
    float available_width = width_ - 40.0f;
    float text_width_approx = text_.length() * (font_size * 0.6f);

    while (text_width_approx > available_width && font_size > 16) {
        font_size -= 2;
        text_width_approx = text_.length() * (font_size * 0.6f);
    }

    float estimated_text_width = text_.length() * (font_size * 0.6f);
    engine::Vector2f text_pos{
        x_ + (width_ - estimated_text_width) / 2.0f,
        y_ + (height_ - font_size) / 2.0f
    };

    // Text shadow
    if (enabled_) {
        engine::Vector2f shadow_pos{text_pos.x + 2, text_pos.y + 2};
        graphics->draw_text(text_, shadow_pos, {0, 0, 0, 100}, engine::INVALID_HANDLE, font_size);
    }

    engine::Color text_color = enabled_ ? engine::Color{255, 255, 255, 255} : engine::Color{180, 180, 180, 255};
    graphics->draw_text(text_, text_pos, text_color, engine::INVALID_HANDLE, font_size);
}

}  // namespace bagario
