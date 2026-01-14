#pragma once

#include <functional>
#include <vector>
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"

namespace bagario {

/**
 * @brief A color picker grid UI component
 */
class UIColorPicker {
public:
    using ColorCallback = std::function<void(engine::Color)>;

    UIColorPicker(float x, float y, float cell_size = 40.0f);

    void set_selected_color(engine::Color color);
    void set_on_color_change(ColorCallback callback);

    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    engine::Color get_selected_color() const { return selected_color_; }

private:
    float x_, y_;
    float cell_size_;
    int selected_index_ = 0;
    engine::Color selected_color_;
    ColorCallback on_color_change_;

    // Color palette (20 vibrant colors in 4x5 grid)
    static constexpr int COLS = 5;
    static constexpr int ROWS = 4;
    std::vector<engine::Color> palette_;

    void init_palette();
    int get_color_index_at(float mx, float my) const;
};

}  // namespace bagario
