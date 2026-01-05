#pragma once

#include <string>
#include "plugin_manager/IGraphicsPlugin.hpp"

namespace rtype {

class UILabel {
public:
    enum class Alignment {
        LEFT,
        CENTER,
        RIGHT
    };

    UILabel(float x, float y, const std::string& text, int font_size = 20);

    void set_position(float x, float y);
    void set_text(const std::string& text);
    void set_font_size(int size);
    void set_color(engine::Color color);
    void set_alignment(Alignment alignment);

    void draw(engine::IGraphicsPlugin* graphics);

private:
    float x_, y_;
    std::string text_;
    int font_size_;
    engine::Color color_ = {255, 255, 255, 255};
    Alignment alignment_ = Alignment::LEFT;
};

}  // namespace rtype
