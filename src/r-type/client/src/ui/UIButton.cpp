#include "ui/UIButton.hpp"

namespace rtype {

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

void UIButton::set_invisible(bool invisible) {
    invisible_ = invisible;
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
    // If invisible, don't draw anything but keep button functional for clicks
    if (invisible_) {
        return;
    }

    float corner_radius = 18.0f;  // Rounded corners like stepper style

    // Choose colors based on state
    engine::Color base_color;
    engine::Color highlight_color;
    float current_glow = glow_intensity_;

    if (!enabled_) {
        base_color = disabled_color_;
        highlight_color = disabled_color_;
        current_glow = 0.0f;
    } else if (pressed_) {
        base_color = {80, 50, 120, 255};  // Purple pressed like stepper
        highlight_color = {100, 60, 160, 255};
        current_glow = 0.3f;
    } else if (selected_) {
        base_color = {100, 60, 160, 255};  // Purple selected like stepper
        highlight_color = {140, 90, 200, 255};
        current_glow = 1.0f;
    } else if (hovered_) {
        base_color = {90, 70, 150, 250};  // More violet hover
        highlight_color = {120, 90, 190, 250};
        current_glow = 0.6f;
    } else {
        base_color = {60, 50, 100, 240};  // More violet normal state
        highlight_color = {80, 70, 140, 240};
        current_glow = 0.0f;
    }

    // Draw shadow for depth (rounded shadow with circles)
    if (enabled_ && !pressed_) {
        float shadow_offset = 6.0f;

        // Shadow main rectangles (horizontal and vertical, without corners)
        engine::Rectangle shadow_h{x_ + shadow_offset + corner_radius, y_ + shadow_offset, width_ - corner_radius * 2, height_};
        graphics->draw_rectangle(shadow_h, {0, 0, 0, 140});
        engine::Rectangle shadow_v{x_ + shadow_offset, y_ + shadow_offset + corner_radius, width_, height_ - corner_radius * 2};
        graphics->draw_rectangle(shadow_v, {0, 0, 0, 140});

        // Shadow corner circles
        graphics->draw_circle({x_ + shadow_offset + corner_radius, y_ + shadow_offset + corner_radius}, corner_radius, {0, 0, 0, 140});
        graphics->draw_circle({x_ + width_ + shadow_offset - corner_radius, y_ + shadow_offset + corner_radius}, corner_radius, {0, 0, 0, 140});
        graphics->draw_circle({x_ + shadow_offset + corner_radius, y_ + height_ + shadow_offset - corner_radius}, corner_radius, {0, 0, 0, 140});
        graphics->draw_circle({x_ + width_ + shadow_offset - corner_radius, y_ + height_ + shadow_offset - corner_radius}, corner_radius, {0, 0, 0, 140});
    }

    // Draw outer glow effect when hovered/selected (R-Type style) - neon violet glow
    if (current_glow > 0.0f) {
        engine::Color glow_color = selected_ ?
            engine::Color{200, 100, 255, 255} :  // Bright violet neon when selected
            engine::Color{180, 120, 255, 255};   // Purple-violet neon when hovered

        // 4 layers for beautiful neon glow effect
        for (int i = 4; i > 0; i--) {
            float expand = 5.0f * i * current_glow;  // Smaller but more intense
            engine::Color layer_glow = glow_color;
            // Neon effect: more opaque in the center, fading outward
            layer_glow.a = static_cast<unsigned char>((80 / i) * current_glow);

            // Main glow rectangles
            engine::Rectangle glow_h{x_ - expand + corner_radius, y_ - expand, width_ - corner_radius * 2, height_ + expand * 2};
            graphics->draw_rectangle(glow_h, layer_glow);
            engine::Rectangle glow_v{x_ - expand, y_ - expand + corner_radius, width_ + expand * 2, height_ - corner_radius * 2};
            graphics->draw_rectangle(glow_v, layer_glow);

            // Glow corners
            float glow_radius = corner_radius + expand;
            graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, glow_radius, layer_glow);
            graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, glow_radius, layer_glow);
            graphics->draw_circle({x_ + corner_radius, y_ + height_ - corner_radius}, glow_radius, layer_glow);
            graphics->draw_circle({x_ + width_ - corner_radius, y_ + height_ - corner_radius}, glow_radius, layer_glow);
        }
    }

    // Draw main button background with rounded corners
    // Main background rectangles (without corners)
    engine::Rectangle bg_h{x_ + corner_radius, y_, width_ - corner_radius * 2, height_};
    graphics->draw_rectangle(bg_h, base_color);
    engine::Rectangle bg_v{x_, y_ + corner_radius, width_, height_ - corner_radius * 2};
    graphics->draw_rectangle(bg_v, base_color);

    // Corner circles for rounded effect
    graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, corner_radius, base_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, corner_radius, base_color);
    graphics->draw_circle({x_ + corner_radius, y_ + height_ - corner_radius}, corner_radius, base_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + height_ - corner_radius}, corner_radius, base_color);

    // Draw inner gradient on top (R-Type style)
    engine::Rectangle top_gradient_h{x_ + corner_radius, y_, width_ - corner_radius * 2, height_ / 2.5f};
    graphics->draw_rectangle(top_gradient_h, highlight_color);
    engine::Rectangle top_gradient_v{x_, y_ + corner_radius, width_, height_ / 2.5f - corner_radius};
    graphics->draw_rectangle(top_gradient_v, highlight_color);

    // Top corners gradient
    graphics->draw_circle({x_ + corner_radius, y_ + corner_radius}, corner_radius, highlight_color);
    graphics->draw_circle({x_ + width_ - corner_radius, y_ + corner_radius}, corner_radius, highlight_color);

    // Draw inner highlight on top edge for 3D effect
    if (enabled_ && !pressed_) {
        engine::Color inner_highlight_color = {255, 255, 255, 40};
        engine::Rectangle inner_highlight{x_ + corner_radius + 5, y_ + 5, width_ - corner_radius * 2 - 10, 2};
        graphics->draw_rectangle(inner_highlight, inner_highlight_color);
    }

    // Draw border with purple accent (R-Type style)
    engine::Color border_color;
    float border_width = 2.5f;

    if (!enabled_) {
        border_color = {90, 90, 100, 150};
        border_width = 1.5f;
    } else if (selected_) {
        border_color = {150, 100, 255, 255};  // Purple border like stepper
        border_width = 3.0f;
    } else if (hovered_) {
        border_color = {160, 120, 255, 220};  // More violet border on hover
        border_width = 2.8f;
    } else {
        border_color = {100, 80, 150, 200};  // More violet normal border
        border_width = 2.0f;
    }

    // Border rectangles (top, bottom, left, right)
    engine::Rectangle border_top{x_ + corner_radius, y_, width_ - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_top, border_color);
    engine::Rectangle border_bottom{x_ + corner_radius, y_ + height_ - border_width, width_ - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_bottom, border_color);
    engine::Rectangle border_left{x_, y_ + corner_radius, border_width, height_ - corner_radius * 2};
    graphics->draw_rectangle(border_left, border_color);
    engine::Rectangle border_right{x_ + width_ - border_width, y_ + corner_radius, border_width, height_ - corner_radius * 2};
    graphics->draw_rectangle(border_right, border_color);

    // Draw text with modern font size and positioning
    // Auto-adjust font size to fit within button width
    int font_size = 42;
    float padding = 20.0f; // Padding on each side

    // Reduce padding for longer text to maximize space
    if (text_.length() > 12) {
        padding = 10.0f;
    }

    float available_width = width_ - (padding * 2);

    // Better approximation for text width: ~0.7 of font_size per character
    float text_width_approx = text_.length() * (font_size * 0.7f);

    // Reduce font size if text is too wide
    while (text_width_approx > available_width && font_size > 18) {
        font_size -= 2;
        text_width_approx = text_.length() * (font_size * 0.7f);
    }

    int text_height_approx = font_size;

    // For horizontal centering based on text length
    // Use different coefficients for better centering based on text characteristics
    float char_width;
    if (text_ == "+" || text_ == "-") {
        // Single character buttons need special handling
        char_width = font_size * 0.5f;
    } else if (text_.find("Back to Menu") != std::string::npos) {
        // "Back to Menu" button needs better centering
        char_width = font_size * 0.58f;
    } else if (text_.find("Settings") != std::string::npos) {
        // "Settings" button needs better centering
        char_width = font_size * 0.58f;
    } else if (text_.find("Leave") != std::string::npos) {
        // "Leave" button
        char_width = font_size * 0.55f;
    } else if (text_.find("Start Game") != std::string::npos) {
        // "Start Game" button
        char_width = font_size * 0.6f;
    } else if (text_.find("BROWSE") != std::string::npos || text_.find("Browse") != std::string::npos) {
        // BROWSE buttons need special centering (shifted slightly left)
        char_width = font_size * 0.62f;
    } else if (text_.find("Nebula") != std::string::npos ||
               text_.find("Asteroid") != std::string::npos ||
               text_.find("Bydo") != std::string::npos) {
        // Map names need precise centering
        char_width = font_size * 0.6f;
    } else {
        // Other buttons use standard centering
        char_width = font_size * 0.7f;
    }
    float estimated_text_width = text_.length() * char_width;

    engine::Vector2f text_pos{
        x_ + (width_ - estimated_text_width) / 2.0f,
        y_ + (height_ - text_height_approx) / 2.0f
    };

    // Text color with R-Type purple tint
    engine::Color text_color;
    if (!enabled_) {
        text_color = {160, 160, 180, 255};
    } else if (selected_) {
        text_color = {230, 210, 255, 255};  // Purple-white for selected
    } else if (hovered_) {
        text_color = {255, 255, 255, 255};  // Bright white on hover
    } else {
        text_color = {220, 230, 245, 255};  // Slightly off-white
    }

    // Draw text shadow for depth
    if (enabled_) {
        engine::Vector2f shadow_pos{text_pos.x + 2, text_pos.y + 2};
        engine::Color text_shadow{0, 0, 0, 120};
        graphics->draw_text(text_, shadow_pos, text_shadow, engine::INVALID_HANDLE, font_size);
    }

    graphics->draw_text(text_, text_pos, text_color, engine::INVALID_HANDLE, font_size);
}

}  // namespace rtype
