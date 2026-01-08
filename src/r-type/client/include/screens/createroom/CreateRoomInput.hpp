#pragma once

#include "plugin_manager/IInputPlugin.hpp"
#include "protocol/Payloads.hpp"

namespace rtype::client::createroom {

/**
 * @brief Handles all input logic for CreateRoomScreen
 */
class InputHandler {
public:
    /**
     * @brief Handle clicks on circular difficulty images
     * @param input Input plugin
     * @param screen_width Screen width for positioning
     * @param current_difficulty Current selected difficulty (will be modified if clicked)
     * @return true if a difficulty was clicked
     */
    static bool handle_difficulty_click(
        engine::IInputPlugin* input,
        float screen_width,
        protocol::Difficulty& current_difficulty
    );

    /**
     * @brief Handle clicks on circular game mode images
     * @param input Input plugin
     * @param screen_width Screen width for positioning
     * @param current_mode Current selected game mode (will be modified if clicked)
     * @return true if a game mode was clicked
     */
    static bool handle_gamemode_click(
        engine::IInputPlugin* input,
        float screen_width,
        protocol::GameMode& current_mode
    );

private:
    /**
     * @brief Check if a point is inside a circle
     * @param mouse_x Mouse X position
     * @param mouse_y Mouse Y position
     * @param circle_x Circle center X
     * @param circle_y Circle center Y
     * @param radius Circle radius
     * @return true if point is inside circle
     */
    static bool is_point_in_circle(
        float mouse_x, float mouse_y,
        float circle_x, float circle_y,
        float radius
    );
};

}  // namespace rtype::client::createroom
