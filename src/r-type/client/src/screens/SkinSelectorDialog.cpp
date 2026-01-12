/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SkinSelectorDialog - Modal for selecting player skin
*/

#include "screens/SkinSelectorDialog.hpp"

namespace rtype::client {

SkinSelectorDialog::SkinSelectorDialog(int screen_width, int screen_height)
    : screen_width_(screen_width)
    , screen_height_(screen_height) {
}

void SkinSelectorDialog::initialize() {
    labels_.clear();
    buttons_.clear();

    float center_x = screen_width_ / 2.0f;
    float center_y = screen_height_ / 2.0f;

    // Title
    auto title = std::make_unique<UILabel>(center_x, center_y - 200, "Select Your Ship", 28);
    title->set_alignment(UILabel::Alignment::CENTER);
    title->set_color(engine::Color{220, 210, 255, 255});
    labels_.push_back(std::move(title));

    // Subtitle with color/type legend
    auto subtitle = std::make_unique<UILabel>(center_x, center_y - 165, "Colors: Green | Red | Blue   -   Types: Scout | Fighter | Cruiser | Bomber | Carrier", 14);
    subtitle->set_alignment(UILabel::Alignment::CENTER);
    subtitle->set_color(engine::Color{160, 150, 200, 200});
    labels_.push_back(std::move(subtitle));

    // Confirm button
    auto confirm_btn = std::make_unique<UIButton>(center_x - 185, center_y + 170, 170, 50, "Confirm");
    confirm_btn->set_on_click([this]() {
        if (on_confirm_) {
            on_confirm_(selected_skin_);
        }
        hide();
    });
    buttons_.push_back(std::move(confirm_btn));

    // Cancel button
    auto cancel_btn = std::make_unique<UIButton>(center_x + 15, center_y + 170, 170, 50, "Cancel");
    cancel_btn->set_on_click([this]() {
        if (on_cancel_) {
            on_cancel_();
        }
        hide();
    });
    buttons_.push_back(std::move(cancel_btn));
}

void SkinSelectorDialog::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (!visible_) return;

    // Get mouse position
    engine::Vector2f mouse_pos = input->get_mouse_position();
    float mouse_x = mouse_pos.x;
    float mouse_y = mouse_pos.y;
    bool mouse_pressed = input->is_mouse_button_pressed(engine::MouseButton::Left);

    // Detect click (mouse just pressed this frame)
    bool mouse_clicked = mouse_pressed && !was_mouse_pressed_;
    was_mouse_pressed_ = mouse_pressed;

    // Calculate grid position
    float grid_width = GRID_COLS * (CELL_SIZE + CELL_PADDING) - CELL_PADDING;
    float grid_height = GRID_ROWS * (CELL_SIZE + CELL_PADDING) - CELL_PADDING;
    float grid_x = (screen_width_ - grid_width) / 2.0f;
    float grid_y = (screen_height_ - grid_height) / 2.0f - 30.0f;

    // Check hover and click on grid cells
    hovered_skin_ = 255;  // Reset hover

    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            float cell_x = grid_x + col * (CELL_SIZE + CELL_PADDING);
            float cell_y = grid_y + row * (CELL_SIZE + CELL_PADDING);

            if (mouse_x >= cell_x && mouse_x < cell_x + CELL_SIZE &&
                mouse_y >= cell_y && mouse_y < cell_y + CELL_SIZE) {
                uint8_t skin_id = static_cast<uint8_t>(row * GRID_COLS + col);
                hovered_skin_ = skin_id;

                if (mouse_clicked) {
                    selected_skin_ = skin_id;
                }
            }
        }
    }

    // Update buttons
    for (auto& button : buttons_) {
        button->update(graphics, input);
    }
}

void SkinSelectorDialog::draw(engine::IGraphicsPlugin* graphics) {
    if (!visible_) return;

    // Draw semi-transparent overlay
    engine::Rectangle overlay{0, 0, static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
    graphics->draw_rectangle(overlay, engine::Color{0, 0, 0, 200});

    // Dialog dimensions
    float dialog_width = 580.0f;
    float dialog_height = 480.0f;
    float dialog_x = (screen_width_ - dialog_width) / 2.0f;
    float dialog_y = (screen_height_ - dialog_height) / 2.0f;
    float corner_radius = 22.0f;

    // Shadow layers
    for (int i = 0; i < 4; ++i) {
        float shadow_offset = 12.0f + i * 4.0f;
        float shadow_alpha = 40 - i * 8;
        engine::Rectangle shadow{dialog_x + shadow_offset, dialog_y + shadow_offset,
                                dialog_width, dialog_height};
        graphics->draw_rectangle(shadow, engine::Color{0, 0, 0, static_cast<uint8_t>(shadow_alpha)});
    }

    // Main dialog background
    engine::Rectangle dialog_main{dialog_x + corner_radius, dialog_y,
                                  dialog_width - corner_radius * 2, dialog_height};
    graphics->draw_rectangle(dialog_main, engine::Color{28, 25, 40, 255});

    engine::Rectangle dialog_left{dialog_x, dialog_y + corner_radius,
                                  corner_radius, dialog_height - corner_radius * 2};
    graphics->draw_rectangle(dialog_left, engine::Color{28, 25, 40, 255});

    engine::Rectangle dialog_right{dialog_x + dialog_width - corner_radius, dialog_y + corner_radius,
                                   corner_radius, dialog_height - corner_radius * 2};
    graphics->draw_rectangle(dialog_right, engine::Color{28, 25, 40, 255});

    // Rounded corners
    graphics->draw_circle({dialog_x + corner_radius, dialog_y + corner_radius},
                         corner_radius, engine::Color{28, 25, 40, 255});
    graphics->draw_circle({dialog_x + dialog_width - corner_radius, dialog_y + corner_radius},
                         corner_radius, engine::Color{28, 25, 40, 255});
    graphics->draw_circle({dialog_x + corner_radius, dialog_y + dialog_height - corner_radius},
                         corner_radius, engine::Color{28, 25, 40, 255});
    graphics->draw_circle({dialog_x + dialog_width - corner_radius, dialog_y + dialog_height - corner_radius},
                         corner_radius, engine::Color{28, 25, 40, 255});

    // Gradient overlay
    engine::Rectangle gradient_overlay{dialog_x + corner_radius, dialog_y,
                                      dialog_width - corner_radius * 2, dialog_height / 2};
    graphics->draw_rectangle(gradient_overlay, engine::Color{45, 40, 65, 60});

    // Border
    engine::Color border_color{120, 100, 200, 255};
    float border_width = 3.0f;

    engine::Rectangle border_top{dialog_x + corner_radius, dialog_y,
                                dialog_width - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_top, border_color);

    engine::Rectangle border_bottom{dialog_x + corner_radius, dialog_y + dialog_height - border_width,
                                   dialog_width - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_bottom, border_color);

    engine::Rectangle border_left{dialog_x, dialog_y + corner_radius,
                                 border_width, dialog_height - corner_radius * 2};
    graphics->draw_rectangle(border_left, border_color);

    engine::Rectangle border_right{dialog_x + dialog_width - border_width, dialog_y + corner_radius,
                                  border_width, dialog_height - corner_radius * 2};
    graphics->draw_rectangle(border_right, border_color);

    // Draw labels
    for (auto& label : labels_) {
        label->draw(graphics);
    }

    // Draw ship grid
    float grid_width = GRID_COLS * (CELL_SIZE + CELL_PADDING) - CELL_PADDING;
    float grid_height = GRID_ROWS * (CELL_SIZE + CELL_PADDING) - CELL_PADDING;
    float grid_x = (screen_width_ - grid_width) / 2.0f;
    float grid_y = (screen_height_ - grid_height) / 2.0f - 30.0f;

    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            uint8_t skin_id = static_cast<uint8_t>(row * GRID_COLS + col);
            float cell_x = grid_x + col * (CELL_SIZE + CELL_PADDING);
            float cell_y = grid_y + row * (CELL_SIZE + CELL_PADDING);

            // Cell background
            engine::Color cell_bg{40, 35, 55, 255};
            if (skin_id == selected_skin_) {
                cell_bg = engine::Color{80, 60, 140, 255};  // Selected - purple
            } else if (skin_id == hovered_skin_) {
                cell_bg = engine::Color{60, 50, 90, 255};   // Hover - lighter
            }

            engine::Rectangle cell{cell_x, cell_y, CELL_SIZE, CELL_SIZE};
            graphics->draw_rectangle(cell, cell_bg);

            // Selection border
            if (skin_id == selected_skin_) {
                engine::Color sel_border{180, 140, 255, 255};
                engine::Rectangle sel_top{cell_x, cell_y, CELL_SIZE, 3.0f};
                engine::Rectangle sel_bot{cell_x, cell_y + CELL_SIZE - 3.0f, CELL_SIZE, 3.0f};
                engine::Rectangle sel_lft{cell_x, cell_y, 3.0f, CELL_SIZE};
                engine::Rectangle sel_rgt{cell_x + CELL_SIZE - 3.0f, cell_y, 3.0f, CELL_SIZE};
                graphics->draw_rectangle(sel_top, sel_border);
                graphics->draw_rectangle(sel_bot, sel_border);
                graphics->draw_rectangle(sel_lft, sel_border);
                graphics->draw_rectangle(sel_rgt, sel_border);
            }

            // Draw ship sprite
            if (spaceship_manager_ && spaceship_manager_->is_loaded()) {
                ShipColor color = static_cast<ShipColor>(row);
                ShipType type = static_cast<ShipType>(col);
                engine::Sprite ship_sprite = spaceship_manager_->create_ship_sprite(color, type, 1.0f);

                // Center sprite in cell
                float sprite_x = cell_x + (CELL_SIZE - ship_sprite.size.x) / 2.0f;
                float sprite_y = cell_y + (CELL_SIZE - ship_sprite.size.y) / 2.0f;

                graphics->draw_sprite(ship_sprite, {sprite_x, sprite_y});
            }
        }
    }

    // Draw buttons
    for (auto& button : buttons_) {
        button->draw(graphics);
    }
}

void SkinSelectorDialog::show(uint8_t current_skin_id) {
    selected_skin_ = current_skin_id % 15;  // Ensure valid range
    visible_ = true;
    hovered_skin_ = 255;
    was_mouse_pressed_ = true;  // Prevent immediate click on show
}

void SkinSelectorDialog::hide() {
    visible_ = false;
    hovered_skin_ = 255;
}

}  // namespace rtype::client
