#include "screens/createroom/CreateRoomUpdater.hpp"
#include "screens/createroom/CreateRoomInput.hpp"

namespace rtype::client::createroom {

void Updater::update_room_info_step(
    std::vector<std::unique_ptr<UITextField>>& fields,
    engine::IGraphicsPlugin* graphics,
    engine::IInputPlugin* input
) {
    for (auto& field : fields) {
        field->update(graphics, input);
    }
}

void Updater::update_map_selection_step(
    std::vector<std::unique_ptr<UIButton>>& map_buttons,
    rtype::client::MapId current_map,
    engine::IGraphicsPlugin* graphics,
    engine::IInputPlugin* input
) {
    for (size_t i = 0; i < map_buttons.size(); ++i) {
        bool is_selected = (static_cast<uint16_t>(current_map) == i + 1);
        map_buttons[i]->set_selected(is_selected);
        map_buttons[i]->update(graphics, input);
    }
}

void Updater::update_difficulty_step(
    protocol::Difficulty& current_difficulty,
    float screen_width,
    engine::IInputPlugin* input
) {
    InputHandler::handle_difficulty_click(input, screen_width, current_difficulty);
}

void Updater::update_game_mode_step(
    protocol::GameMode& current_mode,
    float screen_width,
    engine::IInputPlugin* input
) {
    InputHandler::handle_gamemode_click(input, screen_width, current_mode);
}

void Updater::update_navigation_buttons(
    std::vector<std::unique_ptr<UIButton>>& nav_buttons,
    engine::IGraphicsPlugin* graphics,
    engine::IInputPlugin* input
) {
    for (auto& button : nav_buttons) {
        button->update(graphics, input);
    }
}

bool Updater::is_any_field_focused(
    const std::vector<std::unique_ptr<UITextField>>& fields
) {
    for (const auto& field : fields) {
        if (field->is_focused()) {
            return true;
        }
    }
    return false;
}

}  // namespace rtype::client::createroom
