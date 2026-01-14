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
 * @brief Modal dialog for selecting player skin
 *
 * Displays a 5x3 grid of ship previews (5 types x 3 colors = 15 skins).
 * Player can click a ship to select it, then confirm or cancel.
 */
class SkinSelectorDialog {
public:
    using ConfirmCallback = std::function<void(uint8_t skin_id)>;
    using CancelCallback = std::function<void()>;

    SkinSelectorDialog(int screen_width, int screen_height);

    void initialize();
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    /**
     * @brief Show the dialog with the current skin selected
     * @param current_skin_id Current skin ID (0-14)
     */
    void show(uint8_t current_skin_id);
    void hide();
    bool is_visible() const { return visible_; }

    void set_confirm_callback(ConfirmCallback callback) { on_confirm_ = callback; }
    void set_cancel_callback(CancelCallback callback) { on_cancel_ = callback; }

    /**
     * @brief Set the SpaceshipManager for drawing ship previews
     */
    void set_spaceship_manager(SpaceshipManager* manager) { spaceship_manager_ = manager; }

private:
    int screen_width_;
    int screen_height_;
    bool visible_ = false;
    uint8_t selected_skin_ = 0;
    uint8_t hovered_skin_ = 255;  // 255 = none

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;

    SpaceshipManager* spaceship_manager_ = nullptr;

    ConfirmCallback on_confirm_;
    CancelCallback on_cancel_;

    // Grid constants
    static constexpr int GRID_COLS = 5;  // 5 ship types
    static constexpr int GRID_ROWS = 3;  // 3 colors
    static constexpr float CELL_SIZE = 80.0f;
    static constexpr float CELL_PADDING = 10.0f;

    // Click tracking (like UIButton)
    bool was_mouse_pressed_ = false;
};

}  // namespace rtype::client
