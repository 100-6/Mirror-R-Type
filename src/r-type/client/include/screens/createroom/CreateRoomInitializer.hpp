#pragma once

#include "ui/UIButton.hpp"
#include "ui/UILabel.hpp"
#include "ui/UITextField.hpp"
#include "protocol/Payloads.hpp"
#include "screens/CreateRoomScreen.hpp"
#include <memory>
#include <vector>

namespace rtype::client::createroom {

/**
 * @brief Handles all initialization logic for CreateRoomScreen
 */
class Initializer {
public:
    /**
     * @brief Initialize room info step (name and password fields)
     */
    static void init_room_info_step(
        std::vector<std::unique_ptr<UILabel>>& labels,
        std::vector<std::unique_ptr<UITextField>>& fields,
        float screen_width
    );



    /**
     * @brief Initialize difficulty step (no buttons, uses circular images)
     */
    static void init_difficulty_step();

    /**
     * @brief Initialize game mode step (no buttons, uses circular images)
     */
    static void init_game_mode_step();

    /**
     * @brief Initialize navigation buttons (Previous/Next/Create)
     */
    static void init_navigation_buttons(
        std::vector<std::unique_ptr<UIButton>>& nav_buttons,
        std::function<void()> on_previous,
        std::function<void()> on_next,
        float screen_width,
        float screen_height
    );
};

}  // namespace rtype::client::createroom
