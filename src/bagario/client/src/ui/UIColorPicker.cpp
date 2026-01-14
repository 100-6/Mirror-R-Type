#include "ui/UIColorPicker.hpp"

namespace bagario {

UIColorPicker::UIColorPicker(float x, float y, float cell_size)
    : x_(x), y_(y), cell_size_(cell_size) {
    init_palette();
    if (!palette_.empty()) {
        selected_color_ = palette_[0];
    }
}

void UIColorPicker::init_palette() {
    // Vibrant color palette (20 colors, 4x5 grid)
    palette_ = {
        // Row 1: Reds and pinks
        {244, 67, 54, 255},   // Red
        {233, 30, 99, 255},   // Pink
        {156, 39, 176, 255},  // Purple
        {103, 58, 183, 255},  // Deep Purple
        {63, 81, 181, 255},   // Indigo

        // Row 2: Blues and cyans
        {33, 150, 243, 255},  // Blue
        {3, 169, 244, 255},   // Light Blue
        {0, 188, 212, 255},   // Cyan
        {0, 150, 136, 255},   // Teal
        {76, 175, 80, 255},   // Green

        // Row 3: Greens and yellows
        {139, 195, 74, 255},  // Light Green
        {205, 220, 57, 255},  // Lime
        {255, 235, 59, 255},  // Yellow
        {255, 193, 7, 255},   // Amber
        {255, 152, 0, 255},   // Orange

        // Row 4: Oranges, browns, grays
        {255, 87, 34, 255},   // Deep Orange
        {121, 85, 72, 255},   // Brown
        {158, 158, 158, 255}, // Gray
        {96, 125, 139, 255},  // Blue Gray
        {255, 255, 255, 255}  // White
    };
}

void UIColorPicker::set_selected_color(engine::Color color) {
    selected_color_ = color;
    // Find matching index
    for (size_t i = 0; i < palette_.size(); ++i) {
        if (palette_[i].r == color.r && palette_[i].g == color.g && 
            palette_[i].b == color.b && palette_[i].a == color.a) {
            selected_index_ = static_cast<int>(i);
            break;
        }
    }
}

void UIColorPicker::set_on_color_change(ColorCallback callback) {
    on_color_change_ = callback;
}

int UIColorPicker::get_color_index_at(float mx, float my) const {
    float rel_x = mx - x_;
    float rel_y = my - y_;

    if (rel_x < 0 || rel_y < 0) return -1;

    int col = static_cast<int>(rel_x / cell_size_);
    int row = static_cast<int>(rel_y / cell_size_);

    if (col >= COLS || row >= ROWS) return -1;

    int index = row * COLS + col;
    if (index >= 0 && index < static_cast<int>(palette_.size())) {
        return index;
    }
    return -1;
}

void UIColorPicker::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    auto mouse_pos = input->get_mouse_position();
    float mx = mouse_pos.x;
    float my = mouse_pos.y;

    if (input->is_mouse_button_just_pressed(engine::MouseButton::Left)) {
        int idx = get_color_index_at(mx, my);
        if (idx >= 0) {
            selected_index_ = idx;
            selected_color_ = palette_[idx];
            if (on_color_change_) {
                on_color_change_(selected_color_);
            }
        }
    }
}

void UIColorPicker::draw(engine::IGraphicsPlugin* graphics) {
    float padding = 3.0f;

    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            int idx = row * COLS + col;
            if (idx >= static_cast<int>(palette_.size())) break;

            float cx = x_ + col * cell_size_ + cell_size_ / 2.0f;
            float cy = y_ + row * cell_size_ + cell_size_ / 2.0f;
            float radius = (cell_size_ - padding * 2) / 2.0f;

            // Draw color circle
            graphics->draw_circle({cx, cy}, radius, palette_[idx]);

            // Draw selection indicator
            if (idx == selected_index_) {
                // Outer white ring
                graphics->draw_circle({cx, cy}, radius + 3.0f, engine::Color{255, 255, 255, 200});
                // Re-draw the color on top
                graphics->draw_circle({cx, cy}, radius - 1.0f, palette_[idx]);
            }
        }
    }
}

}  // namespace bagario
