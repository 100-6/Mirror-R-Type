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

    if (alignment_ == Alignment::CENTER) {
        float text_width = graphics->measure_text(text_, font_size_);
        pos.x = x_ - text_width / 2.0f;
    } else if (alignment_ == Alignment::RIGHT) {
        float text_width = graphics->measure_text(text_, font_size_);
        pos.x = x_ - text_width;
    }

    graphics->draw_text(text_, pos, color_, engine::INVALID_HANDLE, font_size_);
}

}  // namespace rtype
