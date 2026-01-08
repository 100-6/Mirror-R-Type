#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include "ui/UIButton.hpp"
#include "ui/UILabel.hpp"
#include "ui/UITextField.hpp"
#include "screens/createroom/CreateRoomRenderer.hpp"
#include "protocol/Payloads.hpp"
#include "screens/CreateRoomScreen.hpp"
#include <memory>
#include <vector>

namespace rtype::client::createroom {

/**
 * @brief Handles all drawing logic for CreateRoomScreen steps
 */
class Drawer {
public:
    /**
     * @brief Draw room info step (labels and text fields)
     */
    static void draw_room_info_step(
        const std::vector<std::unique_ptr<UILabel>>& labels,
        const std::vector<std::unique_ptr<UITextField>>& fields,
        engine::IGraphicsPlugin* graphics
    );

    /**
     * @brief Draw map selection step (images + buttons)
     */
    static void draw_map_selection_step(
        const TexturePack& textures,
        const std::vector<std::unique_ptr<UIButton>>& map_buttons,
        rtype::client::MapId selected_map,
        float screen_width,
        engine::IGraphicsPlugin* graphics
    );

    /**
     * @brief Draw difficulty step (circular images only)
     */
    static void draw_difficulty_step(
        const TexturePack& textures,
        protocol::Difficulty selected_difficulty,
        float screen_width,
        engine::IGraphicsPlugin* graphics
    );

    /**
     * @brief Draw game mode step (circular images only)
     */
    static void draw_game_mode_step(
        const TexturePack& textures,
        protocol::GameMode selected_mode,
        float screen_width,
        engine::IGraphicsPlugin* graphics
    );

    /**
     * @brief Draw circular navigation buttons with arrows
     */
    static void draw_navigation_buttons(
        std::vector<std::unique_ptr<UIButton>>& nav_buttons,
        int current_step_index,
        int total_steps,
        engine::IGraphicsPlugin* graphics,
        int screen_width,
        int screen_height,
        float button_radius,
        float button_spacing,
        float button_y_offset,
        bool edit_mode
    );
};

}  // namespace rtype::client::createroom
