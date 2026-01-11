#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "ui/UIButton.hpp"
#include "ui/UITextField.hpp"
#include "protocol/Payloads.hpp"
#include "screens/CreateRoomScreen.hpp"
#include <memory>
#include <vector>

namespace rtype::client::createroom {

/**
 * @brief Handles all update logic for CreateRoomScreen
 */
class Updater {
public:
    /**
     * @brief Update room info step (text fields)
     */
    static void update_room_info_step(
        std::vector<std::unique_ptr<UITextField>>& fields,
        engine::IGraphicsPlugin* graphics,
        engine::IInputPlugin* input
    );



    /**
     * @brief Update difficulty step (circular click detection)
     */
    static void update_difficulty_step(
        protocol::Difficulty& current_difficulty,
        float screen_width,
        engine::IInputPlugin* input
    );

    /**
     * @brief Update game mode step (circular click detection)
     */
    static void update_game_mode_step(
        protocol::GameMode& current_mode,
        float screen_width,
        engine::IInputPlugin* input
    );

    /**
     * @brief Update navigation buttons
     */
    static void update_navigation_buttons(
        std::vector<std::unique_ptr<UIButton>>& nav_buttons,
        engine::IGraphicsPlugin* graphics,
        engine::IInputPlugin* input
    );

    /**
     * @brief Check if any text field is focused
     */
    static bool is_any_field_focused(
        const std::vector<std::unique_ptr<UITextField>>& fields
    );
};

}  // namespace rtype::client::createroom
