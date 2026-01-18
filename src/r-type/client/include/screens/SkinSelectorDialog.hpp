/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SkinSelectorDialog - Modal for selecting player skin
*/

#pragma once

#include "ui/UIButton.hpp"
#include "ui/UILabel.hpp"
#include "SpaceshipManager.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <memory>
#include <functional>
#include <vector>

namespace rtype::client {

/**
 * @brief Modal dialog for selecting player color
 *
 * Displays a 3x1 grid showing the current ship type in 3 colors.
 * Ship type is determined by player level (level up system).
 * Player can click a color to select it, then confirm or cancel.
 */
class SkinSelectorDialog {
public:
    using ConfirmCallback = std::function<void(uint8_t color_id)>;
    using CancelCallback = std::function<void()>;

    SkinSelectorDialog(int screen_width, int screen_height);

    void initialize();
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    /**
     * @brief Show the dialog with the current color selected
     * @param current_color_id Current color ID (0-2: GREEN, RED, BLUE)
     */
    void show(uint8_t current_color_id);
    void hide();
    bool is_visible() const { return visible_; }

    void set_confirm_callback(ConfirmCallback callback) { on_confirm_ = callback; }
    void set_cancel_callback(CancelCallback callback) { on_cancel_ = callback; }

    /**
     * @brief Set the SpaceshipManager for drawing ship previews
     */
    void set_spaceship_manager(SpaceshipManager* manager) { spaceship_manager_ = manager; }

    /**
     * @brief Set the current player level (determines ship type to display)
     * @param level Player level (1-5)
     */
    void set_current_level(uint8_t level) { current_level_ = level; }

private:
    int screen_width_;
    int screen_height_;
    bool visible_ = false;
    uint8_t selected_color_ = 0;  // 0=GREEN, 1=RED, 2=BLUE
    uint8_t hovered_color_ = 255;  // 255 = none
    uint8_t current_level_ = 1;    // Player level (determines ship type)

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;

    SpaceshipManager* spaceship_manager_ = nullptr;

    ConfirmCallback on_confirm_;
    CancelCallback on_cancel_;

    // Grid constants - 3 colors in a row
    static constexpr int GRID_COLS = 3;  // 3 colors (GREEN, RED, BLUE)
    static constexpr int GRID_ROWS = 1;  // Single row
    static constexpr float CELL_SIZE = 100.0f;  // Larger cells for better visibility
    static constexpr float CELL_PADDING = 20.0f;

    // Click tracking (like UIButton)
    bool was_mouse_pressed_ = false;
};

}  // namespace rtype::client
