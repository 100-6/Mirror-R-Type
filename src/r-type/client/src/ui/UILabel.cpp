#include "ui/UILabel.hpp"

namespace rtype {

UILabel::UILabel(float x, float y, const std::string& text, int font_size)
    : x_(x), y_(y), text_(text), font_size_(font_size) {
}

void UILabel::set_position(float x, float y) {
    x_ = x;
    y_ = y;
}

void UILabel::set_text(const std::string& text) {
    text_ = text;
}

void UILabel::set_font_size(int size) {
    font_size_ = size;
}

void UILabel::set_color(engine::Color color) {
    color_ = color;
}

void UILabel::set_alignment(Alignment alignment) {
    alignment_ = alignment;
}

void UILabel::draw(engine::IGraphicsPlugin* graphics) {
    engine::Vector2f pos{x_, y_};

    // Apply centering if needed (approximate - font_size * 0.6 pixels per char for better accuracy)
    if (alignment_ == Alignment::CENTER) {
        float char_width = font_size_ * 0.6f;  // Better approximation for large fonts
        float text_width_approx = text_.length() * char_width;
        pos.x = x_ - text_width_approx / 2.0f;
    } else if (alignment_ == Alignment::RIGHT) {
        float char_width = font_size_ * 0.6f;
        float text_width_approx = text_.length() * char_width;
        pos.x = x_ - text_width_approx;
    }

    graphics->draw_text(text_, pos, color_, engine::INVALID_HANDLE, font_size_);
}

}  // namespace rtype
